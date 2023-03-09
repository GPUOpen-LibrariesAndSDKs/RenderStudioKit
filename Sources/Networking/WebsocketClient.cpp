#include "WebsocketClient.h"

#include <iostream>
#include <boost/asio/strand.hpp>

#include <Logger/Logger.h>

namespace RenderStudio::Networking
{

WebsocketClient::WebsocketClient(const OnMessageFn& fn)
    : mTcpResolver(boost::asio::make_strand(mIoContext))
    , mWebsocketStream(boost::asio::make_strand(mIoContext))
    , mPingTimer(mIoContext)
    , mOnMessageFn(fn)
    , mConnected(false)
{
}

WebsocketClient::~WebsocketClient()
{
    Disconnect();
}

void WebsocketClient::Connect(const WebsocketEndpoint& endpoint)
{
    mEndpoint = endpoint;

    mTcpResolver.async_resolve(
        mEndpoint.host,
        mEndpoint.port,
        boost::beast::bind_front_handler(&WebsocketClient::OnResolve, shared_from_this())
    );

    mThread = std::thread([this]() { mIoContext.run(); });
    mThread.detach();
}

void WebsocketClient::Disconnect()
{
    if (mConnected)
    {
        mWebsocketStream.async_close(
            boost::beast::websocket::close_code::normal,
            boost::beast::bind_front_handler(&WebsocketClient::OnClose, shared_from_this()));
    }

    mPingTimer.cancel();
    mConnected = false;
}

void WebsocketClient::SendMessageString(const std::string& message)
{
    mWriteQueue.push(message);

    if (!mConnected)
    {
        LOG_WARNING << "[Networking SendMessage] Not connected yet\n";
        return;
    }

    mWebsocketStream.async_write(
        boost::asio::buffer(mWriteQueue.back()),
        boost::beast::bind_front_handler(&WebsocketClient::OnWrite, shared_from_this())
    );
}

void WebsocketClient::Ping(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking Ping] " << ec.message();
        return Disconnect();
    }

    mWebsocketStream.async_ping(
        "",
        boost::beast::bind_front_handler(&WebsocketClient::OnPing, shared_from_this())
    );
}

void WebsocketClient::OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnResolve] " << ec.message();
        return Disconnect();
    }

    boost::beast::get_lowest_layer(mWebsocketStream).expires_after(std::chrono::seconds(30));

    boost::beast::get_lowest_layer(mWebsocketStream).async_connect(
        results,
        boost::beast::bind_front_handler(&WebsocketClient::OnConnect, shared_from_this())
    );
}

void WebsocketClient::OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type endpoint)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnConnect] " << ec.message();
        return Disconnect();
    }

    boost::beast::get_lowest_layer(mWebsocketStream).expires_never();

    mWebsocketStream.set_option(boost::beast::websocket::stream_base::timeout::suggested(
        boost::beast::role_type::client
    ));

    mWebsocketStream.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::request_type& request)
        {
            request.set(boost::beast::http::field::user_agent, std::string("RenderStudio Resolver"));
        }
    ));

    mWebsocketStream.async_handshake(
        mEndpoint.host + ":" + std::to_string(endpoint.port()),
        mEndpoint.path,
        boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this())
    );
}

void WebsocketClient::OnHandshake(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnHandshake] " << ec.message();
        return Disconnect();
    }

    mConnected = true;
    OnPing({});
    OnRead({}, 0);
}

void WebsocketClient::OnPing(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnPing] " << ec.message();
        return Disconnect();
    }

    mPingTimer.expires_from_now(boost::posix_time::seconds(5));

    mPingTimer.async_wait(
        boost::beast::bind_front_handler(&WebsocketClient::Ping, shared_from_this())
    );
}

void WebsocketClient::OnWrite(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);

    if (ec)
    {
        LOG_ERROR << "[Networking OnWrite] " << ec.message();
        return Disconnect();
    }

    mWriteQueue.pop();
}

void WebsocketClient::OnRead(boost::beast::error_code ec, std::size_t transferred)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnRead] " << ec.message();
        return Disconnect();
    }

    if (transferred > 0)
    {
        mOnMessageFn(boost::beast::buffers_to_string(mReadBuffer.data()));
        mReadBuffer.clear();
    }

    mWebsocketStream.async_read(
        mReadBuffer,
        boost::beast::bind_front_handler(&WebsocketClient::OnRead, shared_from_this())
    );
}

void WebsocketClient::OnClose(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking OnClose] " << ec.message();
        return Disconnect();
    }
}

}
