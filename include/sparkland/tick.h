#ifndef TICK_H
#define TICK_H
#include <string>

namespace sparkland {

struct Tick {
    char type[16];        // e.g., "ticker"
    char product_id[16];  // e.g., "ETH-USD"
    char side[8];         // "buy" / "sell"
    char time[32];        // timestamp

    // Prices & volumes
    double price;
    double open_24h;
    double volume_24h;
    double low_24h;
    double high_24h;
    double volume_30d;
    double best_bid;
    double best_bid_size;
    double best_ask;
    double best_ask_size;
    double last_size;

    // Trade identifiers
    uint64_t sequence;
    uint64_t trade_id;

    // Custom fields
    double mid_price;
    double price_ema;
    double mid_price_ema;
};

}

#endif