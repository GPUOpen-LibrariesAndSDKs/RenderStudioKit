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

#pragma once

#pragma warning(push, 0)
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <variant>

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
    std::future<bool> GetConnectionStatus();

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

    using SslStream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;
    using TcpStream = boost::beast::websocket::stream<boost::beast::tcp_stream>;

    std::variant<std::shared_ptr<SslStream>, std::shared_ptr<TcpStream>> mWebsocketStream;
    std::shared_ptr<boost::asio::ssl::context> mSslContext;
    boost::beast::flat_buffer mReadBuffer;
    boost::asio::deadline_timer mPingTimer;
    OnMessageFn mOnMessageFn;
    std::thread mThread;
    bool mConnected;
    std::string mSslHost;

    std::queue<std::string> mWriteQueue;
    std::promise<bool> mConnectionPromise;
};

} // namespace RenderStudio::Networking
