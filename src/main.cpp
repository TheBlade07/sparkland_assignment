#include "sparkland/coinbase_client.h"
#include "sparkland/types.h"
#include "sparkland/tick_parser.h"
#include "sparkland/csv_logger.h"
#include "sparkland/logger.h"


std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main() {

    // Get logger instance
    sparkland::Logger& logger = sparkland::Logger::getInstance();
    logger.info("Starting the application");

    // Pre-allocated buffer to use
    sparkland::TickRingBuffer ring_buffer;

    // Ticker symbols to subscribe
    std::vector<std::string> products = {
        "BTC-USD",
        "ETH-USD", 
        "SOL-USD",
    };

    // Create components
    sparkland::TickParser parser(ring_buffer, products);
    sparkland::CSVLogger csv_logger(ring_buffer, "ticks.csv");
    sparkland::CoinbaseClient client("wss://ws-feed.exchange.coinbase.com", products);

    client.set_message_handler([&parser, &logger](simdjson::padded_string_view payload){
        if (!parser.parse_and_push(payload)) {
            logger.error("Failed to push tick into ring buffer\n");
        }
    });

    // Handle Ctrl+C clean exit
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Start components
    csv_logger.start();
    client.start();

    std::cout<<"Application Started... (Press Ctrl+C to stop)"<<std::endl;

    // coinbase connection monitoring
    auto last_connection_check = std::chrono::steady_clock::now();
    auto connection_timeout = std::chrono::seconds(10);  // 10 second timeout
    bool connection_established = false;

    while (running) {
        auto now = std::chrono::steady_clock::now();
        bool currently_connected = client.is_connected();

        // Check connection status
        if (currently_connected) {
            if (!connection_established) {
                connection_established = true;
                logger.info("Connection established! Market data streaming...");
            }
            last_connection_check = now;
        }
        else{
            // Connection lost or never established
            if (connection_established) {
                logger.error("Connection lost! Initiating shutdown...");
                break;
            }
            
            // Check for connection timeout on startup
            if (now - last_connection_check > connection_timeout) {
                logger.error("Connection timeout! Failed to connect within " + 
                    std::to_string(connection_timeout.count()) + " seconds");
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    logger.info("Initiating shutdown...");
    client.stop();
    // Add some sleep for any in process data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    csv_logger.stop();
    logger.info("Shutdown complete.");
}
