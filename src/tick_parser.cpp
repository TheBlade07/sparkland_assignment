#include "sparkland/tick_parser.h"

namespace sparkland {

TickParser::TickParser(TickRingBuffer& ringBuffer)
    : m_ring_buffer(ringBuffer) {
    
    m_ema_store.emplace("ETH-USD", EMA(5));
}

bool TickParser::parse_and_push(simdjson::padded_string_view payload) {
    try {
        auto doc = m_parser.iterate(payload);
        
        // Consider only ticker messages
        auto type_field = doc["type"];
        if (type_field.error()) return true;
        auto type_str = type_field.get_string().value();
        if (std::strncmp(type_str.data(), "ticker", 6) != 0) return true;
        
        // Acquire next free slot
        Tick* slot = m_ring_buffer.acquire_free_slot();
        if (!slot) {
            // Buffer full
            return false;
        }

        //Parse tick
        std::memcpy(slot->type, type_str.data(), type_str.size());
        slot->type[type_str.size()] = '\0';
        
        auto product_str = doc["product_id"].get_string().value();
        std::memcpy(slot->product_id, product_str.data(), product_str.size());
        slot->product_id[product_str.size()] = '\0';
        
        auto side_str = doc["side"].get_string().value();
        std::memcpy(slot->side, side_str.data(), side_str.size());
        slot->side[side_str.size()] = '\0';
        
        auto time_str = doc["time"].get_string().value();

        // Parsing iso time string for EMA
        // Optimization: Use system time instead of exchange time if clocks are in sync
        auto tick_time = parse_iso8601(time_str, time_str.size());

        std::memcpy(slot->time, time_str.data(), time_str.size());
        slot->time[time_str.size()] = '\0';
        // std::cout<<time_str<<" " << slot->time<<std::endl;

        slot->sequence = doc["sequence"].get_uint64();
        slot->trade_id = doc["trade_id"].get_uint64();

        slot->price = std::stod(std::string(doc["price"].get_string().value()));
        slot->open_24h = std::stod(std::string(doc["open_24h"].get_string().value()));
        slot->volume_24h = std::stod(std::string(doc["volume_24h"].get_string().value()));
        slot->low_24h = std::stod(std::string(doc["low_24h"].get_string().value()));
        slot->high_24h = std::stod(std::string(doc["high_24h"].get_string().value()));
        slot->volume_30d = std::stod(std::string(doc["volume_30d"].get_string().value()));
        slot->best_bid = std::stod(std::string(doc["best_bid"].get_string().value()));
        slot->best_bid_size = std::stod(std::string(doc["best_bid_size"].get_string().value()));
        slot->best_ask = std::stod(std::string(doc["best_ask"].get_string().value()));
        slot->best_ask_size = std::stod(std::string(doc["best_ask_size"].get_string().value()));
        slot->last_size = std::stod(std::string(doc["last_size"].get_string().value()));
        slot->mid_price = (slot->best_bid + slot->best_ask) / 2;
       
        auto& product_ema = m_ema_store.at(slot->product_id);
        product_ema.update(slot->price, slot->mid_price, tick_time);
        slot->price_ema = product_ema.price_ema();
        slot->mid_price_ema = product_ema.mid_ema();

        // Make it available for logging
        m_ring_buffer.publish_slot();
        return true;

    } catch (const simdjson::simdjson_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace sparkland
