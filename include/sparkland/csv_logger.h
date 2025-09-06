#ifndef CSV_LOGGER_H
#define CSV_LOGGER_H

#include <fstream>
#include <atomic>
#include <thread>
#include <string>
#include <memory>
#include "sparkland/tick.h"
#include "sparkland/types.h"

namespace sparkland {

class CSVLogger {
public:
    CSVLogger(TickRingBuffer& ring_buffer, const std::string& filename);
    ~CSVLogger();

    // Delete copy/move operations
    CSVLogger(const CSVLogger&) = delete;
    CSVLogger& operator=(const CSVLogger&) = delete;
    CSVLogger(CSVLogger&&) = delete;
    CSVLogger& operator=(CSVLogger&&) = delete;

    void start();
    void stop();

private:
    void run();

    TickRingBuffer& m_ring_buffer;
    std::ofstream m_file;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};

}

#endif
