#include "sparkland/csv_logger.h"
#include <iostream>
#include <limits>
#include <iomanip>

namespace sparkland {

CSVLogger::CSVLogger(TickRingBuffer& ring_buffer, const std::string& filename)
    : m_ring_buffer(ring_buffer), m_file(filename, std::ios::out | std::ios::trunc)
{
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + filename);
    }

    // Write header row
    m_file << "type,sequence,product_id,price,open_24h,volume_24h,low_24h,high_24h,"
           << "volume_30d,best_bid,best_bid_size,best_ask,best_ask_size,side,time,trade_id,last_size,price_ema,mid_price_ema\n";
}

CSVLogger::~CSVLogger() {
    stop();
    if (m_file.is_open()) {
        m_file.close();
    }
}

void CSVLogger::start() {
    m_running = true;
    m_thread = std::thread(&CSVLogger::run, this);
}

void CSVLogger::stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void CSVLogger::run() {
    m_file << std::setprecision(std::numeric_limits<double>::digits10 + 1);
    while (m_running || !m_ring_buffer.empty()) {
        Tick* tick = m_ring_buffer.acquire_filled_slot();
        if (tick) {
            // Write Tick to CSV
            m_file << tick->type << ","
                   << tick->sequence << ","
                   << tick->product_id << ","
                   << tick->price << ","
                   << tick->open_24h << ","
                   << tick->volume_24h << ","
                   << tick->low_24h << ","
                   << tick->high_24h << ","
                   << tick->volume_30d << ","
                   << tick->best_bid << ","
                   << tick->best_bid_size << ","
                   << tick->best_ask << ","
                   << tick->best_ask_size << ","
                   << tick->side << ","
                   << tick->time << ","
                   << tick->trade_id << ","
                   << tick->last_size << ","
                   << tick->price_ema << ","
                   << tick->mid_price_ema
                   << "\n";
            
            // Return slot to free state
            m_ring_buffer.release_slot();
        } else {
            // Avoid busy spinning — short sleep
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

}
