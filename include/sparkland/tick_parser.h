#ifndef TICK_PARSER_H
#define TICK_PARSER_H

#include <string>
#include <iostream>
#include <cstring>
#include <chrono>
#include <unordered_map>
#include <simdjson.h>
#include "sparkland/types.h"
#include "sparkland/ema.h"

namespace sparkland {

class TickParser {
public:
    TickParser(TickRingBuffer& ringBuffer, const std::vector<std::string>& product_ids);

    // Delete copy/move operations since ring_buffer reference can cause issue
    TickParser(const TickParser&) = delete;
    TickParser& operator=(const TickParser&) = delete;
    TickParser(TickParser&&) = delete;
    TickParser& operator=(TickParser&&) = delete;
    
    // Explicit destructor
    ~TickParser() = default;
    
    // Parse incoming JSON packet into next available Tick slot
    // Returns true if successfully parsed & pushed, false if buffer full or parse error
    bool parse_and_push(simdjson::padded_string_view payload);

private:
    TickRingBuffer& m_ring_buffer;
    simdjson::ondemand::parser m_parser;
    std::unordered_map<std::string, EMA> m_ema_store;

    inline std::chrono::system_clock::time_point parse_iso8601(std::string_view str, size_t len) {
        // Expected format: YYYY-MM-DDTHH:MM:SS.ssssssZ
        // Example: 2022-10-19T23:28:22.061769Z

        int year = (str[0]-'0')*1000 + (str[1]-'0')*100 + (str[2]-'0')*10 + (str[3]-'0');
        int month = (str[5]-'0')*10 + (str[6]-'0');
        int day = (str[8]-'0')*10 + (str[9]-'0');
        int hour = (str[11]-'0')*10 + (str[12]-'0');
        int minute = (str[14]-'0')*10 + (str[15]-'0');
        int second = (str[17]-'0')*10 + (str[18]-'0');
    
        // Convert to time_t (UTC)
        std::tm t{};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        t.tm_sec = second;
    
        std::time_t time_sec = timegm(&t);
    
        auto tp = std::chrono::system_clock::from_time_t(time_sec);
    
        // Fractional seconds
        if (len > 20 && str[19] == '.') {
            int micros = 0;
            int factor = 100000;
            for (size_t i = 20; i < len && std::isdigit(str[i]) && factor > 0; ++i) {
                micros += (str[i]-'0') * factor;
                factor /= 10;
            }
            tp += std::chrono::microseconds(micros);
        }
    
        return tp;
    }
};

}

#endif
