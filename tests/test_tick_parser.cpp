#include <gtest/gtest.h>
#include "sparkland/tick_parser.h"
#include "sparkland/types.h"
#include <simdjson.h>

using namespace sparkland;

class TickParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        product_ids = {"BTC-USD", "ETH-USD"};
        parser = std::make_unique<TickParser>(ring_buffer, product_ids);
    }

    TickRingBuffer ring_buffer;
    std::vector<std::string> product_ids;
    std::unique_ptr<TickParser> parser;
    
    // Helper function to create valid ticker JSON
    std::string createTickerJson(const std::string& product_id = "BTC-USD",
                                const std::string& price = "111135.56",
                                const std::string& best_bid = "111135.55",
                                const std::string& best_ask = "111135.57") {
        return R"({
            "type": "ticker",
            "sequence": 111484916886,
            "product_id": ")" + product_id + R"(",
            "price": ")" + price + R"(",
            "open_24h": "1689.62789966",
            "volume_24h": "1234.56",
            "low_24h": "109993",
            "high_24h": "111389.94",
            "volume_30d": "1158609.10516081",
            "best_bid": ")" + best_bid + R"(",
            "best_bid_size": "0.0450549",
            "best_ask": ")" + best_ask + R"(",
            "best_ask_size": "0.00222536",
            "side": "sell",
            "time": "2025-09-07T08:47:52.369411Z",
            "trade_id": 871379421,
            "last_size": "9.08999"
        })";
    }
};

TEST_F(TickParserTest, ParseValidTickerMessage) {
    std::string json_str = createTickerJson();
    simdjson::padded_string payload(json_str);
    
    EXPECT_TRUE(ring_buffer.empty());
    
    bool result = parser->parse_and_push(payload);
    EXPECT_TRUE(result);
    EXPECT_FALSE(ring_buffer.empty());
    EXPECT_EQ(ring_buffer.size(), 1);
}

TEST_F(TickParserTest, NonTickerMessageIgnored) {
    std::string non_ticker_json = R"({
        "type": "subscriptions",
        "channels": [
            {
                "name": "ticker",
                "product_ids": ["BTC-USD", "ETH-USD"]
            }
        ]
    })";
    
    simdjson::padded_string payload(non_ticker_json);
    
    bool result = parser->parse_and_push(payload);
    // Should return true but not add to buffer
    EXPECT_TRUE(result);
    EXPECT_TRUE(ring_buffer.empty());
}

TEST_F(TickParserTest, ParsedDataIntegrity) {
    std::string json_str = createTickerJson("ETH-USD", "4305.0", "4304.98", "4305.1");
    simdjson::padded_string payload(json_str);
    
    parser->parse_and_push(payload);
    
    Tick* tick = ring_buffer.acquire_filled_slot();
    ASSERT_NE(tick, nullptr);
    
    // Verify parsed data
    EXPECT_STREQ(tick->type, "ticker");
    EXPECT_STREQ(tick->product_id, "ETH-USD");
    EXPECT_STREQ(tick->side, "sell");
    EXPECT_EQ(tick->sequence, 111484916886);
    EXPECT_EQ(tick->trade_id, 871379421);
    EXPECT_DOUBLE_EQ(tick->price, 4305.0);
    EXPECT_DOUBLE_EQ(tick->open_24h, 1689.62789966);
    EXPECT_DOUBLE_EQ(tick->volume_24h, 1234.56);
    EXPECT_DOUBLE_EQ(tick->best_bid, 4304.98);
    EXPECT_DOUBLE_EQ(tick->best_ask, 4305.1);
    EXPECT_DOUBLE_EQ(tick->best_bid_size, 0.0450549);
    EXPECT_DOUBLE_EQ(tick->best_ask_size, 0.00222536);
    
    // Verify calculated mid price
    double expected_mid = (4304.98 + 4305.1) / 2.0;
    EXPECT_DOUBLE_EQ(tick->mid_price, expected_mid);
    
    ring_buffer.release_slot();
}

TEST_F(TickParserTest, InvalidJsonHandling) {
    std::string invalid_json = R"({
        type : "ticker",
        "product_id": "BTC-USD",
        "price": NaN,
    })"; // type is not in string so not parsable
    
    simdjson::padded_string payload(invalid_json);
    
    bool result = parser->parse_and_push(payload);
    EXPECT_FALSE(result);  // Should return false for invalid JSON
    EXPECT_TRUE(ring_buffer.empty());
}

TEST_F(TickParserTest, EMACalculation) {
    std::string json_str1 = createTickerJson("BTC-USD", "110000.0", "110000.0", "110000.5");
    std::string json_str2 = createTickerJson("BTC-USD", "110001.5", "110001.0", "110001.8");
    
    simdjson::padded_string payload1(json_str1);
    simdjson::padded_string payload2(json_str2);
    
    // First tick
    parser->parse_and_push(payload1);
    Tick* tick1 = ring_buffer.acquire_filled_slot();
    
    // First EMA should equal the initial values
    EXPECT_DOUBLE_EQ(tick1->price_ema, 110000.0);
    EXPECT_DOUBLE_EQ(tick1->mid_price_ema, 110000.25);  // (49999 + 50001) / 2
    
    ring_buffer.release_slot();
    
    // Second tick (small delay will be applied due to steady_clock)
    parser->parse_and_push(payload2);
    Tick* tick2 = ring_buffer.acquire_filled_slot();
    
    // EMA should be between old and new values
    EXPECT_GT(tick2->price_ema, 110000.0);
    EXPECT_LT(tick2->price_ema, 110001.5);
    
    EXPECT_GT(tick2->mid_price_ema, 110000.25);
    EXPECT_LT(tick2->mid_price_ema, 110001.4);
    
    ring_buffer.release_slot();
}


TEST_F(TickParserTest, MissingFieldsHandling) {
    std::string minimal_json = R"({
        "type": "ticker",
        "product_id": "BTC-USD",
        "price": "110000.0",
        "best_bid": "110000.0",
        "best_ask": "110000.1"
    })";  // Missing many optional fields
    
    simdjson::padded_string payload(minimal_json);
    
    bool result = parser->parse_and_push(payload);
    EXPECT_TRUE(result);
    
    Tick* tick = ring_buffer.acquire_filled_slot();
    ASSERT_NE(tick, nullptr);
    
    // Essential fields should be present
    EXPECT_STREQ(tick->type, "ticker");
    EXPECT_STREQ(tick->product_id, "BTC-USD");
    EXPECT_DOUBLE_EQ(tick->price, 110000.0);
    EXPECT_DOUBLE_EQ(tick->best_bid, 110000.0);
    EXPECT_DOUBLE_EQ(tick->best_ask, 110000.1);
    
    // Missing fields should have default values
    EXPECT_EQ(tick->sequence, 0);
    EXPECT_EQ(tick->trade_id, 0);
    EXPECT_DOUBLE_EQ(tick->open_24h, 0.0);
    EXPECT_STREQ(tick->side, "");
    
    ring_buffer.release_slot();
}

TEST_F(TickParserTest, BufferFullHandling) {
    // Fill buffer to capacity
    std::string json_str = createTickerJson();
    simdjson::padded_string payload(json_str);
    
    // Fill the ring buffer
    const size_t capacity = ring_buffer.capacity();
    for (size_t i = 0; i < capacity; ++i) {
        bool result = parser->parse_and_push(payload);
        EXPECT_TRUE(result) << "Failed to push at index " << i;
    }
    
    EXPECT_TRUE(ring_buffer.full());
    
    // Try to add one more - should fail
    bool result = parser->parse_and_push(payload);
    EXPECT_FALSE(result);  // Should return false when buffer is full
}