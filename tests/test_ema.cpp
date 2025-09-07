#include <gtest/gtest.h>
#include "sparkland/ema.h"
#include <chrono>
#include <thread>

using namespace sparkland;
using namespace std::chrono;

class EMATest : public ::testing::Test {
protected:
    void SetUp() override {
        start_time = steady_clock::now();
    }

    steady_clock::time_point start_time;
};

TEST_F(EMATest, InitialState) {
    EMA ema(5.0);
    
    // Check initial values
    EXPECT_FALSE(ema.initialized());
    EXPECT_DOUBLE_EQ(ema.price_ema(), 0.0);
    EXPECT_DOUBLE_EQ(ema.mid_ema(), 0.0);
}

TEST_F(EMATest, FirstUpdate) {
    EMA ema(5.0);
    
    double price = 100.0;
    double mid_price = 99.5;
    auto tick_time = start_time;
    
    ema.update(price, mid_price, tick_time);
    
    EXPECT_TRUE(ema.initialized());
    // First update should set EMA to the initial prices
    EXPECT_DOUBLE_EQ(ema.price_ema(), price);
    EXPECT_DOUBLE_EQ(ema.mid_ema(), mid_price);
}

TEST_F(EMATest, SubsequentUpdates) {
    EMA ema(5.0);  // 5 second time period
    
    // First update
    ema.update(100.0, 99.5, start_time);
    
    // Second update after 1 second
    auto second_time = start_time + seconds(1);
    ema.update(110.0, 109.5, second_time);
    
    // EMA should be between old and new values
    double price_ema = ema.price_ema();
    double mid_ema = ema.mid_ema();
    
    EXPECT_GT(price_ema, 100.0);  // Should be higher than initial
    EXPECT_LT(price_ema, 110.0);  // But lower than new value
    
    EXPECT_GT(mid_ema, 99.5);     // Should be higher than initial
    EXPECT_LT(mid_ema, 109.5);    // But lower than new value
}

TEST_F(EMATest, ZeroTimeDelta) {
    EMA ema(5.0);
    
    ema.update(100.0, 99.5, start_time);
    double initial_price = ema.price_ema();
    double initial_mid = ema.mid_ema();
    
    // Update with same timestamp (zero delta)
    ema.update(200.0, 199.5, start_time);
    
    // With zero time delta, alpha should be minimal
    // So values should change very little
    EXPECT_NEAR(ema.price_ema(), initial_price, 1.0);
    EXPECT_NEAR(ema.mid_ema(), initial_mid, 1.0);

    // But ema should change from previous value
    EXPECT_NE(ema.price_ema(), initial_price);
    EXPECT_NE(ema.mid_ema(), initial_mid);
}

TEST_F(EMATest, LargeTimeDelta) {
    EMA ema(1.0);  // 1 second time period
    
    ema.update(100.0, 99.5, start_time);
    
    // Update after 10 seconds (much larger than time period)
    auto later_time = start_time + seconds(10);
    ema.update(200.0, 199.5, later_time);
    
    // With large time delta, alpha should be close to 1
    // So EMA should be very close to new values
    EXPECT_NEAR(ema.price_ema(), 200.0, 1.0);
    EXPECT_NEAR(ema.mid_ema(), 199.5, 1.0);
}

TEST_F(EMATest, NegativeTimeDelta) {
    EMA ema(5.0);
    
    auto later_time = start_time + seconds(5);
    ema.update(100.0, 99.5, later_time);
    
    // Update with earlier timestamp (negative delta)
    ema.update(200.0, 199.5, start_time);
    
    // Should handle negative delta gracefully (treat as zero)
    // Values should change minimally
    EXPECT_NEAR(ema.price_ema(), 100.0, 1.0);
    EXPECT_NEAR(ema.mid_ema(), 99.5, 1.0);
}

TEST_F(EMATest, EMACorrectness) {
    EMA ema(5.0);  // 5 second time period
    
    // First update
    ema.update(100.0, 95.0, start_time);
    EXPECT_DOUBLE_EQ(ema.price_ema(), 100.0);
    EXPECT_DOUBLE_EQ(ema.mid_ema(), 95.0);
    
    // Second update after exactly 1 second
    auto second_time = start_time + seconds(1);
    ema.update(102.0, 100.5, second_time);
    
    // Calculate expected alpha: 1 - exp(-dt/time_period) = 1 - exp(-1/5)
    double dt = 1.0;
    double time_period = 5.0;
    double expected_alpha = 1.0 - std::exp(-dt / time_period);
    
    // Expected EMA = alpha * new_value + (1 - alpha) * old_ema
    double expected_price_ema = expected_alpha * 102.0 + (1.0 - expected_alpha) * 100.0;
    double expected_mid_ema = expected_alpha * 100.5 + (1.0 - expected_alpha) * 95.0;
    
    EXPECT_NEAR(ema.price_ema(), expected_price_ema, 1e-10);
    EXPECT_NEAR(ema.mid_ema(), expected_mid_ema, 1e-10);
}