#ifndef TICK_PARSER_H
#define TICK_PARSER_H

#include <string>
#include "sparkland/types.h"
#include "simdjson.h"

namespace sparkland {

class TickParser {
public:
    TickParser(TickRingBuffer& ringBuffer);
    
    // Parse incoming JSON packet into next available Tick slot
    // Returns true if successfully parsed & pushed, false if buffer full or parse error
    bool parse_and_push(simdjson::padded_string_view payload);

private:
    TickRingBuffer& m_ring_buffer;
    simdjson::ondemand::parser m_parser;
};

}

#endif
