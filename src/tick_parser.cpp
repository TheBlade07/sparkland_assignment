#include "sparkland/tick_parser.h"

namespace sparkland {

TickParser::TickParser(TickRingBuffer& ringBuffer, const std::vector<std::string>& product_ids)
    : m_ring_buffer(ringBuffer) {
    
    for(auto products: product_ids)
        m_ema_store.emplace(products, EMA(5));
}

auto copy_field = [](auto &doc, const char* field_name, char* dest) {
    auto val = doc[field_name];
    if (!val.error()) {
        auto str = val.value().get_string().value();
        std::memcpy(dest, str.data(), str.size());
        dest[str.size()] = '\0';
    } else {
        dest[0] = '\0'; // field missing → empty string
    }
};

bool TickParser::parse_and_push(simdjson::padded_string_view payload) {
    try {
        auto doc = m_parser.iterate(payload);
        
        // Consider only ticker messages
        auto type_field = doc["type"];
        if (type_field.error()) return true;
        auto type_str = type_field.get_string().value();
        if (std::strncmp(type_str.data(), "ticker", 6) != 0) return true;

        // Parsing iso time string for EMA
        // Optimization: Use system time instead of exchange time since parsing iso string is very slow
        // auto tick_time = parse_iso8601(time_str, time_str.size());
        auto tick_time = std::chrono::steady_clock::now();
        
        // Acquire next free slot
        Tick* slot = m_ring_buffer.acquire_free_slot();
        if (!slot) {
            // Buffer full
            return false;
        }

        //Parse tick
        std::memcpy(slot->type, type_str.data(), type_str.size());
        slot->type[type_str.size()] = '\0';
        
        copy_field(doc, "product_id", slot->product_id);
        copy_field(doc, "side", slot->side);
        copy_field(doc, "time", slot->time);
        
        auto seq_res = doc["sequence"];
        if (!seq_res.error()) {
            slot->sequence = seq_res.get_uint64();
        } else {
            slot->sequence = 0;
        }

        auto trade_id = doc["trade_id"];
        if (!trade_id.error()) {
            slot->trade_id = trade_id.get_uint64();
        } else {
            slot->trade_id = 0;
        }

        slot->price = !doc["price"].error() ? std::stod(std::string(doc["price"].get_string().value())) : 0.0;
        slot->open_24h = !doc["open_24h"].error() ? std::stod(std::string(doc["open_24h"].get_string().value())) : 0.0;
        slot->volume_24h = !doc["volume_24h"].error() ? std::stod(std::string(doc["volume_24h"].get_string().value())) : 0.0;
        slot->low_24h = !doc["low_24h"].error() ? std::stod(std::string(doc["low_24h"].get_string().value())) : 0.0;
        slot->high_24h = !doc["high_24h"].error() ? std::stod(std::string(doc["high_24h"].get_string().value())) : 0.0;
        slot->volume_30d = !doc["volume_30d"].error() ? std::stod(std::string(doc["volume_30d"].get_string().value())) : 0.0;
        slot->best_bid = !doc["best_bid"].error() ? std::stod(std::string(doc["best_bid"].get_string().value())) : 0.0;
        slot->best_bid_size = !doc["best_bid_size"].error() ? std::stod(std::string(doc["best_bid_size"].get_string().value())) : 0.0;
        slot->best_ask = !doc["best_ask"].error() ? std::stod(std::string(doc["best_ask"].get_string().value())) : 0.0;
        slot->best_ask_size = !doc["best_ask_size"].error() ? std::stod(std::string(doc["best_ask_size"].get_string().value())) : 0.0;
        slot->last_size = !doc["last_size"].error() ? std::stod(std::string(doc["last_size"].get_string().value())) : 0.0;
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
