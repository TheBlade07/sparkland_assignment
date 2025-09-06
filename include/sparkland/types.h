#ifndef TYPES_H
#define TYPES_H

#include "ring_buffer.h"
#include "tick.h"

namespace sparkland {

constexpr size_t TICK_BUFFER_CAPACITY = 1024;
using TickRingBuffer = RingBuffer<Tick, TICK_BUFFER_CAPACITY>;

}

#endif
