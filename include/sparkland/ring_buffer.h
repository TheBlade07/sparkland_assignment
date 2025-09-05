#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <array>
#include <cstddef>

namespace sparkland {

template <typename T, size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 1, "Capacity must be greater than 1");

private:
    alignas(64) std::atomic<size_t> m_head{0};  // Pop index
    alignas(64) std::atomic<size_t> m_tail{0};  // Push index
    std::array<T*, Capacity> m_buffer;

public:
    RingBuffer() = default;
    ~RingBuffer() {
        // Clean up any remaining items
        T* item = nullptr;
        while (pop(item)) {
            delete item;
        }
    }

    // Delete copy/move operations
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer(RingBuffer&&) = delete;
    RingBuffer& operator=(RingBuffer&&) = delete;

    bool push(T* item) {
        size_t current_tail = m_tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;

        // Check if buffer is full
        if (next_tail == m_head.load(std::memory_order_acquire)) {
            return false;
        }

        m_buffer[current_tail] = item;
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T*& item) {
        size_t current_head = m_head.load(std::memory_order_relaxed);

        // Check if buffer is empty
        if (current_head == m_tail.load(std::memory_order_acquire)) {
            return false;
        }

        item = m_buffer[current_head];
        size_t next_head = (current_head + 1) % Capacity;
        m_head.store(next_head, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t next_tail = (m_tail.load(std::memory_order_relaxed) + 1) % Capacity;
        return next_tail == m_head.load(std::memory_order_acquire);
    }

    size_t size() const {
        size_t head = m_head.load(std::memory_order_acquire);
        size_t tail = m_tail.load(std::memory_order_acquire);

        if (tail >= head) {
            return tail - head;
        } else {
            return Capacity - head + tail;
        }
    }

    size_t capacity() const {
        return Capacity - 1; // One slot always unused
    }
};

}

#endif