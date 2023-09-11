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

WebsocketClient::WebsocketClient(const OnMessageFn& fn)
    : mTcpResolver(boost::asio::make_strand(mIoContext))
    , mPingTimer(mIoContext)
    , mOnMessageFn(fn)
    , mConnected(false)
{
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
        std::visit(
            [this](auto& stream)
            {
                stream->async_close(
                    boost::beast::websocket::close_code::normal,
                    boost::beast::bind_front_handler(&WebsocketClient::OnClose, shared_from_this()));
            },
            mWebsocketStream);

        LOG_DEBUG << "Websocket disconnected";
    }

    mPingTimer.cancel();
    mConnected = false;
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
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    std::visit(
        [this](auto& stream)
        { stream->async_ping("", boost::beast::bind_front_handler(&WebsocketClient::OnPing, shared_from_this())); },
        mWebsocketStream);
}

void
WebsocketClient::OnResolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    std::visit(
        [this, &results](auto& stream)
        {
            boost::beast::get_lowest_layer(*stream.get()).expires_after(std::chrono::seconds(30));

            boost::beast::get_lowest_layer(*stream.get())
                .async_connect(
                    results, boost::beast::bind_front_handler(&WebsocketClient::OnConnect, shared_from_this()));
        },
        mWebsocketStream);
}

void
WebsocketClient::OnConnect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::endpoint_type endpoint)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
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
        Overload { [this, &endpoint](const std::shared_ptr<SslStream>& stream)
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
                           boost::beast::bind_front_handler(&WebsocketClient::OnSslHandshake, shared_from_this()));
                   },
                   [this, &endpoint](const std::shared_ptr<TcpStream>& stream)
                   {
                       mSslHost = mEndpoint.Host() + ":" + std::to_string(endpoint.port());

                       stream->async_handshake(
                           mSslHost,
                           mEndpoint.Target(),
                           boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this()));
                   } },
        mWebsocketStream);
}

void
WebsocketClient::OnSslHandshake(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    std::visit(
        [this](auto& stream)
        {
            stream->async_handshake(
                mSslHost,
                mEndpoint.Target(),
                boost::beast::bind_front_handler(&WebsocketClient::OnHandshake, shared_from_this()));
        },
        mWebsocketStream);
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
        LOG_ERROR << "[Networking] " << ec.message();
        return Disconnect();
    }

    if (transferred > 0)
    {
        mOnMessageFn(boost::beast::buffers_to_string(mReadBuffer.data()));
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
