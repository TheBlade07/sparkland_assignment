#ifndef TICK_PARSER_H
#define TICK_PARSER_H

#include <string>
#include "sparkland/ring_buffer.h"
#include "sparkland/tick.h"
#include "simdjson.h"

namespace sparkland {

class TickParser {
public:
    TickParser(RingBuffer<Tick, 1024>& ringBuffer);
    
    // Parse incoming JSON packet into next available Tick slot
    // Returns true if successfully parsed & pushed, false if buffer full or parse error
    bool parse_and_push(simdjson::padded_string_view payload);

private:
    RingBuffer<Tick, 1024>& m_ring_buffer;
    simdjson::ondemand::parser m_parser;
};

}

#endif
