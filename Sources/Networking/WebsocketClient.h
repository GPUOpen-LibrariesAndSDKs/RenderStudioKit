#pragma once

#pragma warning(push, 0)
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>
#pragma warning(pop)

#include "Url.h"

namespace boost::beast
{
template <class T> class ssl_stream;
}

namespace boost::asio::ssl
{
class context;
}

namespace RenderStudio::Networking
{

class WebsocketClient : public std::enable_shared_from_this<WebsocketClient>
{
public:
    using OnMessageFn = std::function<void(const std::string&)>;

    template <typename... Args> static std::shared_ptr<WebsocketClient> Create(Args&&... args)
    {
        return std::shared_ptr<WebsocketClient> { new WebsocketClient { std::forward<Args>(args)... } };
    }

    ~WebsocketClient();

    void Connect(const Url& endpoint);
    void Disconnect();
    void Send(const std::string& message);

private:
    explicit WebsocketClient(const OnMessageFn& fn);

    void Ping(boost::beast::error_code ec);
    void OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
    void OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type endpoint);
    void OnHandshake(boost::beast::error_code ec);
    void OnSslHandshake(boost::beast::error_code ec);
    void OnPing(boost::beast::error_code ec);
    void Write(const std::string& message);
    void OnWrite(boost::beast::error_code ec, std::size_t transferred);
    void OnRead(boost::beast::error_code ec, std::size_t transferred);
    void OnClose(boost::beast::error_code ec);

private:
    Url mEndpoint;
    boost::asio::io_context mIoContext;
    boost::asio::ip::tcp::resolver mTcpResolver;
    std::shared_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>
        mWebsocketStream;
    std::shared_ptr<boost::asio::ssl::context> mSslContext;
    boost::beast::flat_buffer mReadBuffer;
    boost::asio::deadline_timer mPingTimer;
    OnMessageFn mOnMessageFn;
    std::thread mThread;
    bool mConnected;
    std::string mSslHost;

    std::queue<std::string> mWriteQueue;
};

} // namespace RenderStudio::Networking