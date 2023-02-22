#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <memory>
#include <thread>
#include <future>
 
#include <cstdlib>
#include <map>
#include <string>
#include <sstream>
#include <json.hpp>

#include <Logger.hpp>


typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
 
class connection_metadata {
    std::promise<void> m_connected_promise;
public:
    typedef std::shared_ptr<connection_metadata> ptr;
 
    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri, std::promise<void> connected_promise)
      : m_id(id)
      , m_hdl(hdl)
      , m_uri(uri)
      , m_server("N/A")
      , m_connected_promise(std::move(connected_promise))
    {}
 
    void on_open(client * c, websocketpp::connection_hdl hdl) { 
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");

        m_connected_promise.set_value();
    }
 
    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
        Logger::Error("Websocket failed, {}", m_error_reason);
    }

    void on_close(client * c, websocketpp::connection_hdl hdl) {
        
    }

    void on_interrupt(client * c, websocketpp::connection_hdl hdl) {
        Logger::Error("Websocket interrupted");
    }

    void on_message(client * c, websocketpp::connection_hdl hdl, message_ptr message) {
        std::lock_guard<std::mutex> lock(m_event_mutex);

        auto json = nlohmann::json::parse(message->get_payload());
        
        std::string event = json["event"].get<std::string>();
        if (event != "StageLayer") {
            return;
        } 

        nlohmann::json body = json["body"];
        std::string tag = body["tag"].get<std::string>();
        std::string data = body["data"].get<std::string>();

        // Check if we have subscription for tag
        auto subscription_iter = std::find_if(std::begin(m_subscriptions), std::end(m_subscriptions), [&tag](const auto& entry) {
            return entry.first == tag;
        });

        if (subscription_iter != std::end(m_subscriptions)) {
            // Resolve for existing subscription
            subscription_iter->second.set_value(json);
            m_subscriptions.erase(subscription_iter);
        } else {
            // Add to queue
            m_messages[tag] = json;
        }
    }

    void subscribe_to_tag(std::string tag, std::promise<nlohmann::json> promise) {
        std::lock_guard<std::mutex> lock(m_event_mutex);

        // Check if message already fetched
        auto message_iter = std::find_if(std::begin(m_messages), std::end(m_messages), [&tag](const auto& entry) {
            return entry.first == tag;
        });

        if (message_iter != std::end(m_messages)) {
            // Resolve immediately
            promise.set_value(message_iter->second);
            m_messages.erase(message_iter);
        } else {
            // Add to queue
            m_subscriptions[tag] = std::move(promise);
        }
    }

    websocketpp::connection_hdl get_hdl() const { return m_hdl; }
 
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::map<std::string, nlohmann::json> m_messages;
    std::map<std::string, std::promise<nlohmann::json>> m_subscriptions;
    std::mutex m_event_mutex;
};
 
class websocket_endpoint {
public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread.reset(new std::thread(&client::run, &m_endpoint));
    }

    bool IsConnected() const {
        return !m_connection_list.empty();
    }
 
    std::future<void> Connect(std::string const & uri) {
        std::promise<void> connected_promise;
        std::future<void> connected_future = connected_promise.get_future();

        websocketpp::lib::error_code ec;
 
        client::connection_ptr con = m_endpoint.get_connection(uri, ec);
 
        if (ec) {
            std::cout << "[AR] [WebSocket] Connect initialization error: " << ec.message() << std::endl;
            return connected_future;
        }
 
        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri, std::move(connected_promise)));
        m_connection_list[new_id] = metadata_ptr;
 
        con->set_open_handler(std::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_fail_handler(std::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_message_handler(std::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1,
            std::placeholders::_2
        ));
        con->set_close_handler(std::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
        con->set_interrupt_handler(std::bind(
            &connection_metadata::on_interrupt,
            metadata_ptr,
            &m_endpoint,
            std::placeholders::_1
        ));
 
        m_endpoint.connect(con);
 
        return connected_future;
    }
 
    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }

    void send(std::string message) {
        int id = 0;
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "[AR] [WebSocket] No connection found with id " << id << std::endl;
        }

        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "[AR] [WebSocket] Error sending message: " << ec.message() << std::endl;
        }
    }

    std::future<nlohmann::json> wait_for_message_tag(std::string tag) {
        std::promise<nlohmann::json> promise;
        std::future<nlohmann::json> future = promise.get_future();

        int id = 0;
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "[AR] [WebSocket] No connection found with id " << id << std::endl;
        }

        metadata_it->second->subscribe_to_tag(tag, std::move(promise));

        return future;
    }

private:
    typedef std::map<int,connection_metadata::ptr> con_list;
 
    client m_endpoint;
    std::shared_ptr<std::thread> m_thread;
 
    con_list m_connection_list;
    int m_next_id;
};
