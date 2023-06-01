#pragma once

#pragma warning(push, 0)
#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>
#pragma warning(pop)

namespace RenderStudio::Networking
{

class WebsocketServer : public std::enable_shared_from_this<WebsocketServer>
{
public:
    explicit WebsocketServer();

private:
    void Accept();

    boost::asio::io_context mIoContext;
    boost::asio::ip::tcp::acceptor mTcpAcceptor { mIoContext };
    boost::asio::ip::tcp::endpoint mTcpEndpoint { boost::asio::ip::make_address("0.0.0.0"), 10000ui16 };
};

} // namespace RenderStudio::Networking