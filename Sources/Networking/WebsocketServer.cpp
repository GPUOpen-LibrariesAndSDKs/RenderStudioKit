#include "WebsocketServer.h"

namespace RenderStudio::Networking
{

WebsocketServer::WebsocketServer()
{
    boost::beast::error_code ec;

    mTcpAcceptor.open(mTcpEndpoint.protocol(), ec);
    if (ec)
    {
        throw std::runtime_error("[Networking] " + ec.message());
    }

    mTcpAcceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
    {
        throw std::runtime_error("[Networking] " + ec.message());
    }

    mTcpAcceptor.bind(mTcpEndpoint, ec);
    if (ec)
    {
        throw std::runtime_error("[Networking] " + ec.message());
    }

    mTcpAcceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        throw std::runtime_error("[Networking] " + ec.message());
    }

    // Start accepting connections
    Accept();
}

void
WebsocketServer::Accept()
{
    /*mTcpAcceptor.async_accept(
        boost::asio::make_strand(mIoContext),
        boost::beast::bind_front_handler(&WebsocketServer::OnAccept, std::shared_from_this())
    )*/
}

} // namespace RenderStudio::Networking
