#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <queue>


namespace RenderStudio::Networking
{

struct WebsocketEndpoint
{
    std::string protocol;
    std::string host;
    std::string port;
    std::string path;

    static WebsocketEndpoint FromString(const std::string& url);
};

class WebsocketClient : public std::enable_shared_from_this<WebsocketClient>
{
public:
    using OnMessageFn = std::function<void(const std::string&)>;
    explicit WebsocketClient(const OnMessageFn& fn);
    ~WebsocketClient();

    void Connect(const WebsocketEndpoint& endpoint);
    void Disconnect();
    void SendMessageString(const std::string& message);

private:
    void Ping(boost::beast::error_code ec);
    void OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
    void OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type endpoint);
    void OnHandshake(boost::beast::error_code ec);
    void OnPing(boost::beast::error_code ec);
    void OnWrite(boost::beast::error_code ec, std::size_t transferred);
    void OnRead(boost::beast::error_code ec, std::size_t transferred);
    void OnClose(boost::beast::error_code ec);

private:
    WebsocketEndpoint mEndpoint;
    boost::asio::io_context mIoContext;
    boost::asio::ip::tcp::resolver mTcpResolver;
    boost::beast::websocket::stream<boost::beast::tcp_stream> mWebsocketStream;
    boost::beast::flat_buffer mReadBuffer;
    boost::asio::deadline_timer mPingTimer;
    OnMessageFn mOnMessageFn;
    std::thread mThread;
    bool mConnected;

    std::queue<std::string> mWriteQueue;
};

}