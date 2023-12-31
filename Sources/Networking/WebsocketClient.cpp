// Copyright 2023 Advanced Micro Devices, Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "WebsocketClient.h"

#pragma warning(push, 0)
#include <functional>

#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <uriparser/Uri.h>
#pragma warning(pop)

#ifdef PLATFORM_WINDOWS
#include "Certificates.h"
#endif

#include <Logger/Logger.h>

namespace
{

template <typename... Ts> struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

} // namespace

namespace RenderStudio::Networking
{

WebsocketClient::WebsocketClient(IClientLogic& logic)
    : mTcpResolver(boost::asio::make_strand(mIoContext))
    , mPingTimer(mIoContext)
    , mConnected(false)
    , mLogic(logic)
{
}

WebsocketClient::~WebsocketClient() { Disconnect(); }

std::future<bool>
WebsocketClient::Connect(const Url& endpoint)
{
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    mEndpoint = endpoint;

    if (endpoint.Ssl())
    {
        mSslContext = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

#ifdef PLATFORM_WINDOWS
        AddWindowsRootCertificates(*mSslContext.get());
#endif
        mWebsocketStream = std::make_shared<SslStream>(boost::asio::make_strand(mIoContext), *mSslContext.get());
    }
    else
    {
        mWebsocketStream = std::make_shared<TcpStream>(boost::asio::make_strand(mIoContext));
    }

    mTcpResolver.async_resolve(
        mEndpoint.Host(),
        mEndpoint.Port(),
        boost::beast::bind_front_handler(&WebsocketClient::OnResolve, shared_from_this(), promise));

    mThread = std::thread([this]() { mIoContext.run(); });

    mThread.detach();
    return future;
}

std::future<bool>
WebsocketClient::Disconnect()
{
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    if (mConnected)
    {
        std::visit(
            [this, promise](auto& stream)
            {
                stream->async_close(
                    boost::beast::websocket::close_code::normal,
                    boost::beast::bind_front_handler(&WebsocketClient::OnClose, shared_from_this(), promise));
            },
            mWebsocketStream);
    }
    else
    {
        mLogic.OnDisconnected();
        promise->set_value(true);
    }

    mPingTimer.cancel();
    mConnected = false;

    return future;
}

void
WebsocketClient::Send(const std::string& message)
{
    // From boost documentation:
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    std::visit(
        [this, &message](auto& stream)
        {
            boost::asio::post(
                stream->get_executor(),
                boost::beast::bind_front_handler(&WebsocketClient::Write, shared_from_this(), message));
        },
        mWebsocketStream);
}

void
WebsocketClient::Write(const std::string& message)
{
    mWriteQueue.push(message);

    if (!mConnected)
    {
        return;
    }

    if (mWriteQueue.size() > 1)
    {
        return;
    }

    std::visit(
        [this](auto& stream)
        {
            stream->async_write(
                boost::asio::buffer(mWriteQueue.back()),
                boost::beast::bind_front_handler(&WebsocketClient::OnWrite, shared_from_this()));
        },
        mWebsocketStream);
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
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        Disconnect();
        return;
    }

    std::visit(
        [this](auto& stream)
        { stream->async_ping("", boost::beast::bind_front_handler(&WebsocketClient::OnPing, shared_from_this())); },
        mWebsocketStream);
}

void
WebsocketClient::OnResolve(
    std::shared_ptr<std::promise<bool>> promise,
    boost::beast::error_code ec,
    boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        LOG_ERROR << "[WebsocketClient]  " << ec.message();
        promise->set_value(false);
        Disconnect();
        return;
    }

    std::visit(
        [this, &results, promise](auto& stream)
        {
            boost::beast::get_lowest_layer(*stream.get()).expires_after(std::chrono::seconds(30));

            boost::beast::get_lowest_layer(*stream.get())
                .async_connect(
                    results,
                    boost::beast::bind_front_handler(&WebsocketClient::OnConnect, shared_from_this(), promise));
        },
        mWebsocketStream);
}

void
WebsocketClient::OnConnect(
    std::shared_ptr<std::promise<bool>> promise,
    boost::beast::error_code ec,
    boost::asio::ip::tcp::resolver::endpoint_type endpoint)
{
    if (ec)
    {
        promise->set_value(false);
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        Disconnect();
        return;
    }

    std::visit(
        [this](auto& stream)
        {
            boost::beast::get_lowest_layer(*stream.get()).expires_never();

            stream->set_option(
                boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

            stream->set_option(boost::beast::websocket::stream_base::decorator(
                [](boost::beast::websocket::request_type& request)
                { request.set(boost::beast::http::field::user_agent, std::string("RenderStudio Resolver")); }));
        },
        mWebsocketStream);

    std::visit(
        Overload {
            [this, &endpoint, promise](const std::shared_ptr<SslStream>& stream)
            {
                if (!SSL_set_tlsext_host_name(stream->next_layer().native_handle(), mEndpoint.Host().c_str()))
                {
                    boost::beast::error_code sslEc { static_cast<int>(::ERR_get_error()),
                                                     boost::asio::error::get_ssl_category() };
                    throw boost::beast::system_error { sslEc };
                }

                mSslHost = mEndpoint.Host() + ":" + std::to_string(endpoint.port());

                stream->next_layer().async_handshake(
                    boost::asio::ssl::stream_base::client,
                    boost::beast::bind_front_handler(&WebsocketClient::OnSslHandshake, shared_from_this(), promise));
            },
            [this, &endpoint, promise](const std::shared_ptr<TcpStream>& stream)
            {
                mSslHost = mEndpoint.Host() + ":" + std::to_string(endpoint.port());

                stream->async_handshake(
                    mSslHost,
                    mEndpoint.Target(),
                    boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this(), promise));
            } },
        mWebsocketStream);
}

void
WebsocketClient::OnSslHandshake(std::shared_ptr<std::promise<bool>> promise, boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        promise->set_value(false);
        Disconnect();
        return;
    }

    std::visit(
        [this, promise](auto& stream)
        {
            stream->async_handshake(
                mSslHost,
                mEndpoint.Target(),
                boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this(), promise));
        },
        mWebsocketStream);
}

void
WebsocketClient::OnHandshake(std::shared_ptr<std::promise<bool>> promise, boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        promise->set_value(false);
        Disconnect();
        return;
    }

    LOG_INFO << "[WebsocketClient] Connected to " << mEndpoint.Host();

    mConnected = true;
    mLogic.OnConnected();
    promise->set_value(true);
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
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        Disconnect();
        return;
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
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        Disconnect();
        return;
    }

    mWriteQueue.pop();

    if (!mWriteQueue.empty())
    {

        std::visit(
            [this](auto& stream)
            {
                stream->async_write(
                    boost::asio::buffer(mWriteQueue.back()),
                    boost::beast::bind_front_handler(&WebsocketClient::OnWrite, shared_from_this()));
            },
            mWebsocketStream);
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
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        Disconnect();
        return;
    }

    if (transferred > 0)
    {
        mLogic.OnMessage(boost::beast::buffers_to_string(mReadBuffer.data()));
        mReadBuffer.clear();
    }

    std::visit(
        [this](auto& stream) {
            stream->async_read(
                mReadBuffer, boost::beast::bind_front_handler(&WebsocketClient::OnRead, shared_from_this()));
        },
        mWebsocketStream);
}

void
WebsocketClient::OnClose(std::shared_ptr<std::promise<bool>> promise, boost::beast::error_code ec)
{
    mLogic.OnDisconnected();

    if (!mConnected)
    {
        promise->set_value(true);
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[WebsocketClient] " << ec.message();
        promise->set_value(false);
        Disconnect();
    }
}

bool
WebsocketClient::IsPortInUse(std::uint16_t port)
{
    boost::asio::io_context svc;
    boost::asio::ip::tcp::acceptor a(svc);
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port);
    a.open(boost::asio::ip::tcp::v4(), ec);
    a.bind(endpoint, ec);
    return ec == boost::asio::error::address_in_use;
}

} // namespace RenderStudio::Networking
