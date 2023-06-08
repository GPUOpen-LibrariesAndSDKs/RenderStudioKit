#include "WebsocketClient.h"

#pragma warning(push, 0)
#include <functional>

#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <uriparser/Uri.h>
#pragma warning(pop)

#include "Certificates.h"

#include <Logger/Logger.h>

namespace RenderStudio::Networking
{

WebsocketClient::WebsocketClient(const OnMessageFn& fn)
    : mTcpResolver(boost::asio::make_strand(mIoContext))
    , mPingTimer(mIoContext)
    , mOnMessageFn(fn)
    , mConnected(false)
{
    mSslContext = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
    AddWindowsRootCertificates(*mSslContext.get());
    mWebsocketStream
        = std::make_shared<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>(
            boost::asio::make_strand(mIoContext), *mSslContext.get());
}

WebsocketClient::~WebsocketClient()
{
    Disconnect();
    LOG_DEBUG << "Websocket client destroyed";
}

void
WebsocketClient::Connect(const Url& endpoint)
{
    mEndpoint = endpoint;

    mTcpResolver.async_resolve(
        mEndpoint.Host(),
        mEndpoint.Port(),
        boost::beast::bind_front_handler(&WebsocketClient::OnResolve, shared_from_this()));

    mThread = std::thread(
        [this]()
        {
            mIoContext.run();
            LOG_DEBUG << "Websocket thread finished";
        });

    mThread.detach();
}

void
WebsocketClient::Disconnect()
{
    if (mConnected)
    {
        mWebsocketStream->async_close(
            boost::beast::websocket::close_code::normal,
            boost::beast::bind_front_handler(&WebsocketClient::OnClose, shared_from_this()));

        LOG_DEBUG << "Websocket disconnected";
    }

    mPingTimer.cancel();
    mConnected = false;
}

void
WebsocketClient::Send(const std::string& message)
{
    mWriteQueue.push(message);

    if (!mConnected)
    {
        return;
    }

    mWebsocketStream->async_write(
        boost::asio::buffer(mWriteQueue.back()),
        boost::beast::bind_front_handler(&WebsocketClient::OnWrite, shared_from_this()));
}

void
WebsocketClient::Ping(boost::beast::error_code ec)
{
    if (!mConnected)
    {
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    mWebsocketStream->async_ping("", boost::beast::bind_front_handler(&WebsocketClient::OnPing, shared_from_this()));
}

void
WebsocketClient::OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    boost::beast::get_lowest_layer(*mWebsocketStream.get()).expires_after(std::chrono::seconds(30));

    boost::beast::get_lowest_layer(*mWebsocketStream.get())
        .async_connect(results, boost::beast::bind_front_handler(&WebsocketClient::OnConnect, shared_from_this()));
}

void
WebsocketClient::OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type endpoint)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    boost::beast::get_lowest_layer(*mWebsocketStream.get()).expires_never();

    mWebsocketStream->set_option(
        boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

    mWebsocketStream->set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& request)
        { request.set(boost::beast::http::field::user_agent, std::string("RenderStudio Resolver")); }));

    if (!SSL_set_tlsext_host_name(mWebsocketStream->next_layer().native_handle(), mEndpoint.Host().c_str()))
    {
        boost::beast::error_code sslEc { static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
        throw boost::beast::system_error { sslEc };
    }

    mSslHost = mEndpoint.Host() + ":" + std::to_string(endpoint.port());

    mWebsocketStream->next_layer().async_handshake(
        boost::asio::ssl::stream_base::client,
        boost::beast::bind_front_handler(&WebsocketClient::OnSslHandshake, shared_from_this()));
}

void
WebsocketClient::OnSslHandshake(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    mWebsocketStream->async_handshake(
        mSslHost,
        mEndpoint.Target(),
        boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this()));
}

void
WebsocketClient::OnHandshake(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    LOG_INFO << "[Networking] Connected to " << mEndpoint.Host();

    mConnected = true;
    OnPing({});
    OnRead({}, 0);
}

void
WebsocketClient::OnPing(boost::beast::error_code ec)
{
    if (!mConnected)
    {
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    mPingTimer.expires_from_now(boost::posix_time::seconds(5));

    mPingTimer.async_wait(boost::beast::bind_front_handler(&WebsocketClient::Ping, shared_from_this()));
}

void
WebsocketClient::OnWrite(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    mWriteQueue.pop();

    if (!mWriteQueue.empty())
    {
        mWebsocketStream->async_write(
            boost::asio::buffer(mWriteQueue.back()),
            boost::beast::bind_front_handler(&WebsocketClient::OnWrite, shared_from_this()));
    }
}

void
WebsocketClient::OnRead(boost::beast::error_code ec, std::size_t transferred)
{
    if (!mConnected)
    {
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    if (transferred > 0)
    {
        mOnMessageFn(boost::beast::buffers_to_string(mReadBuffer.data()));
        mReadBuffer.clear();
    }

    mWebsocketStream->async_read(
        mReadBuffer, boost::beast::bind_front_handler(&WebsocketClient::OnRead, shared_from_this()));
}

void
WebsocketClient::OnClose(boost::beast::error_code ec)
{
    if (!mConnected)
    {
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }
}

} // namespace RenderStudio::Networking
