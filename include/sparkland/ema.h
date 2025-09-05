#ifndef EMA_H
#define EMA_H

namespace sparkland {

class EMA {
public:
    EMA(double alpha) : m_alpha(alpha), m_initialized(false) {}

    double update(double price) {
        if (!m_initialized) {
            m_value = price;
            m_initialized = true;
        } else {
            m_value = m_alpha * price + (1.0 - m_alpha) * m_value;
        }
        return m_value;
    }

    double value() const { return m_value; }

private:
    double m_alpha;
    double m_value;
    bool m_initialized;
};

}

#endif
