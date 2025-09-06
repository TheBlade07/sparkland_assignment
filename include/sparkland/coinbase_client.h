#ifndef COINBASE_CLIENT_H
#define COINBASE_CLIENT_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <simdjson.h>

#include <functional>
#include <string>
#include <thread>
#include <atomic>


namespace sparkland{

using ContextPtr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
using AsioClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using MessageHandler = std::function<void(simdjson::padded_string_view)>;

class CoinbaseClient {
public:
    CoinbaseClient(const std::string& uri, const std::vector<std::string>& product_ids);
    ~CoinbaseClient();

    // Delete copy constructor and assignment operator
    CoinbaseClient(const CoinbaseClient&) = delete;
    CoinbaseClient& operator=(const CoinbaseClient&) = delete;

    // Start connection and event loop in a background thread
    void start();

    // Stop connection and event loop
    void stop();

    void set_message_handler(MessageHandler handler);

private:
    void on_open(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, AsioClient::message_ptr msg);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

    void send_subscribe();
    ContextPtr on_tls_init(websocketpp::connection_hdl);

private:
    std::string m_uri;
    std::vector<std::string> m_product_ids;
    MessageHandler m_handler;

    AsioClient m_client;
    websocketpp::connection_hdl m_hdl;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};

}

#endif
