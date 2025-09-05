#include "sparkland/coinbase_client.h"
#include <simdjson.h>

int main() {
    sparkland::CoinbaseClient client("wss://ws-feed.exchange.coinbase.com", "BTC-USD");

    client.set_message_handler([](simdjson::padded_string_view payload){
        std::cout<<"Yes"<<std::endl;
        simdjson::ondemand::parser parser;
        auto doc = parser.iterate(payload);
    
        // Method 1: Use find_field() instead of operator[]
        auto price_field = doc.find_field("price");
        std::cout<<"Yes"<<std::endl;
        if (!price_field.error()) {
            double price = price_field.get_double();
            std::cout << "Price: " << price << "\n";
        } else {
            std::cout << "Price field not found in JSON\n";
        }
    });
    

    client.start();

    std::this_thread::sleep_for(std::chrono::seconds(20));
    client.stop();
}
