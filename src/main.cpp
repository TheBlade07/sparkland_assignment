#include "sparkland/coinbase_client.h"
#include "sparkland/types.h"
#include "sparkland/tick_parser.h"
#include "sparkland/csv_logger.h"


std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main() {

    // pre allocated a buffer to use
    sparkland::TickRingBuffer ring_buffer;

    // Create tick parser
    sparkland::TickParser parser(ring_buffer);
    sparkland::CSVLogger logger(ring_buffer, "ticks.csv");

    logger.start();

    // create a client to connect with coinbase
    sparkland::CoinbaseClient client("wss://ws-feed.exchange.coinbase.com", "ETH-USD");

    client.set_message_handler([&parser](simdjson::padded_string_view payload){
        if (!parser.parse_and_push(payload)) {
            std::cerr << "Failed to push tick into ring buffer\n";
        }
    });

    // Handle Ctrl+C clean exit
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    client.start();

    std::cout << "Running... Press Ctrl+C to stop." << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    client.stop();
    logger.stop();
}
