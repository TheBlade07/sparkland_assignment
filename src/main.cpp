#include "sparkland/coinbase_client.h"
#include "sparkland/ring_buffer.h"
#include "sparkland/tick.h"
#include "sparkland/tick_parser.h"

int main() {

    // pre allocated a buffer to use
    sparkland::RingBuffer<sparkland::Tick, 1024> ring_buffer;

    // Create tick parser
    sparkland::TickParser parser(ring_buffer);

    // create a client to connect with coinbase
    sparkland::CoinbaseClient client("wss://ws-feed.exchange.coinbase.com", "BTC-USD");

    client.set_message_handler([&parser](simdjson::padded_string_view payload){
        if (!parser.parse_and_push(payload)) {
            std::cerr << "Failed to push tick into ring buffer\n";
        }
    });    

    client.start();

    std::this_thread::sleep_for(std::chrono::seconds(20));
    client.stop();
}
