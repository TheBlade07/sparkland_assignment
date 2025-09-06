#ifndef EMA_H
#define EMA_H

#include <chrono>
#include <iostream>

namespace sparkland {

class EMA {
public:
    EMA(double time_period) : m_time_period(time_period), m_initialized(false) {}

    void update(double price, double mid_price, std::chrono::steady_clock::time_point tick_time) {
        if (!m_initialized) {
            m_price_ema = price;
            m_mid_ema   = mid_price;
            m_last_update = tick_time;
            m_initialized = true;
            return;
        }

        double dt = std::chrono::duration<double>(tick_time - m_last_update).count();
        if (dt < 0) dt = 0;

        double alpha = (dt == 0) ? MIN_ALPHA : 1.0 - std::exp(-dt / m_time_period);
        m_price_ema = alpha * price + (1 - alpha) * m_price_ema;
        m_mid_ema   = alpha * mid_price + (1 - alpha) * m_mid_ema;
        m_last_update = tick_time;
    }

    double price_ema() const { return m_price_ema; }
    double mid_ema()   const { return m_mid_ema; }
    bool initialized() const { return m_initialized; }

private:
    double m_time_period;
    double m_price_ema = 0.0;
    double m_mid_ema   = 0.0;
    bool m_initialized;
    std::chrono::steady_clock::time_point m_last_update;
    static constexpr double MIN_ALPHA = 0.001;
};

}

#endif
