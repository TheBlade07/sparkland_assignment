#include "sparkland/coinbase_client.h"

#include <websocketpp/common/asio.hpp>
#include <websocketpp/common/asio_ssl.hpp>

#include <iostream>
#include <sstream>
#include <chrono>

namespace sparkland {

CoinbaseClient::CoinbaseClient(const std::string& uri, const std::string& product_id)
    : m_uri(uri), m_product_id(product_id) {
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.init_asio();

    m_client.set_tls_init_handler([this](websocketpp::connection_hdl hdl) {
        return this->on_tls_init(hdl);
    });
    m_client.set_open_handler([this](websocketpp::connection_hdl hdl) { on_open(hdl); });
    m_client.set_message_handler([this](websocketpp::connection_hdl hdl, AsioClient::message_ptr msg) { on_message(hdl, msg); });
    m_client.set_fail_handler([this](websocketpp::connection_hdl hdl) { on_fail(hdl); });
    m_client.set_close_handler([this](websocketpp::connection_hdl hdl) { on_close(hdl); });
}

CoinbaseClient::~CoinbaseClient() {
    stop();
}

void CoinbaseClient::start() {
    if (m_running.exchange(true)) return;

    websocketpp::lib::error_code ec;
    auto con = m_client.get_connection(m_uri, ec);
    if (ec) {
        std::cerr << "Connection error: " << ec.message() << "\n";
        return;
    }

    m_hdl = con->get_handle();
    m_client.connect(con);

    m_thread = std::thread([this]() {
        m_client.run();
    });
}

void CoinbaseClient::stop() {
    if (!m_running.exchange(false)) return;

    websocketpp::lib::error_code ec;
    m_client.close(m_hdl, websocketpp::close::status::normal, "Client shutdown", ec);

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void CoinbaseClient::set_message_handler(MessageHandler handler) {
    m_handler = std::move(handler);
}

void CoinbaseClient::on_open(websocketpp::connection_hdl hdl) {
    std::cout << "Connected to Coinbase WS\n";
    send_subscribe();
}

void CoinbaseClient::on_message(websocketpp::connection_hdl, AsioClient::message_ptr msg) {
    if (m_handler) {
        // Pass zero-copy payload to handler
        simdjson::padded_string payload(msg->get_payload().data(), msg->get_payload().size());
        m_handler(payload);
    }
}

void CoinbaseClient::on_fail(websocketpp::connection_hdl) {
    std::cerr << "Connection failed. Will need reconnection strategy.\n";
}

void CoinbaseClient::on_close(websocketpp::connection_hdl) {
    std::cerr << "Connection closed.\n";
}

void CoinbaseClient::send_subscribe() {
    std::ostringstream oss;
    oss << R"({"type": "subscribe", "product_ids": [")" << m_product_id
        << R"("], "channels": ["ticker"]})";

    websocketpp::lib::error_code ec;
    m_client.send(m_hdl, oss.str(), websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cerr << "Send error: " << ec.message() << "\n";
    }
}

ContextPtr CoinbaseClient::on_tls_init(websocketpp::connection_hdl) {
    ContextPtr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
        std::cout << "TLS Initialization Error: " << e.what() << std::endl;
    }

    return ctx;
}

}
