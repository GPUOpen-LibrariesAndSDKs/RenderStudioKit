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
#include <memory>
#include <queue>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>
#pragma warning(pop)

namespace RenderStudio::Networking
{

class WebsocketSession;
class WebsocketServer;

struct IServerLogic
{
    virtual void OnConnected(std::shared_ptr<WebsocketSession> session) = 0;
    virtual void OnDisconnected(std::shared_ptr<WebsocketSession> session) = 0;
    virtual void OnMessage(std::shared_ptr<WebsocketSession> session, const std::string& message) = 0;
    virtual ~IServerLogic() = default;
};

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    explicit HttpSession(boost::asio::ip::tcp::socket&& socket, IServerLogic& logic);

public:
    template <typename... Args> static std::shared_ptr<HttpSession> Create(Args&&... args)
    {
        return std::shared_ptr<HttpSession> { new HttpSession { std::forward<Args>(args)... } };
    }

private:
    void Run();
    void OnRun();
    void Read();
    void OnRead(boost::beast::error_code ec, std::size_t transferred);

private:
    friend class WebsocketServer;

    boost::beast::tcp_stream mTcpStream;
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> mParser;
    boost::beast::flat_buffer mBuffer;
    IServerLogic& mServerLogic;
};

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    explicit WebsocketSession(
        boost::asio::ip::tcp::socket&& socket,
        IServerLogic& logic,
        boost::beast::http::message<true, boost::beast::http::string_body, boost::beast::http::fields> request);

public:
    template <typename... Args> static std::shared_ptr<WebsocketSession> Create(Args&&... args)
    {
        return std::shared_ptr<WebsocketSession> { new WebsocketSession { std::forward<Args>(args)... } };
    }

    void Send(const std::string& message);

    std::string GetDebugName() const;
    std::string GetChannel() const;

private:
    friend class HttpSession;

    void Run();
    void OnRun();
    void OnAccept(boost::beast::error_code ec);
    void Read();
    void OnRead(boost::beast::error_code ec, std::size_t transferred);
    void Write(const std::string& message);
    void OnWrite(boost::beast::error_code ec, std::size_t transferred);

private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> mWebsocketStream;
    boost::beast::flat_buffer mReadBuffer;
    std::string mDebugName;
    IServerLogic& mServerLogic;
    boost::beast::http::message<true, boost::beast::http::string_body, boost::beast::http::fields> mRequest;
    std::string mChannel;
    std::queue<std::string> mWriteQueue;
    bool mConnected = false;
};

class WebsocketServer : public std::enable_shared_from_this<WebsocketServer>
{
    explicit WebsocketServer(
        boost::asio::io_context& ioc,
        boost::asio::ip::tcp::endpoint endpoint,
        IServerLogic& logic);

public:
    template <typename... Args> static std::shared_ptr<WebsocketServer> Create(Args&&... args)
    {
        return std::shared_ptr<WebsocketServer> { new WebsocketServer { std::forward<Args>(args)... } };
    }

    void Run();

private:
    void Accept();
    void OnAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

    boost::asio::io_context& mIoContext;
    boost::asio::ip::tcp::endpoint mTcpEndpoint;
    boost::asio::ip::tcp::acceptor mTcpAcceptor;
    IServerLogic& mServerLogic;
};

} // namespace RenderStudio::Networking
