#include "WebsocketServer.h"

#pragma warning(push, 0)
#include <boost/asio/strand.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#pragma warning(pop)

#include <Logger/Logger.h>

namespace
{

struct Util
{
    static std::pair<std::string, std::string> ExtractChannelAndUserNames(std::string target)
    {
        std::string channelName;
        std::string userName;

        std::size_t queryPosition = target.find('?');

        if (queryPosition != std::string::npos)
        {
            channelName = target.substr(0, queryPosition);

            std::string query = target.substr(queryPosition + 1);
            std::size_t start = 0;
            std::size_t end = 0;

            while (end != std::string::npos)
            {
                end = query.find('&', start);

                std::string keyValue = query.substr(start, end - start);
                std::size_t equalsPos = keyValue.find('=');

                if (equalsPos != std::string::npos)
                {
                    std::string key = keyValue.substr(0, equalsPos);
                    std::string value = keyValue.substr(equalsPos + 1);

                    if (key == "user")
                    {
                        userName = value;
                    }
                }

                start = end + 1;
            }
        }
        else
        {
            channelName = target;
        }

        channelName.erase(std::remove(channelName.begin(), channelName.end(), '/'), channelName.end());

        return { channelName, userName };
    }
};

} // namespace

namespace RenderStudio::Networking
{

WebsocketServer::WebsocketServer(
    boost::asio::io_context& ioc,
    boost::asio::ip::tcp::endpoint endpoint,
    IServerLogic& logic)
    : mIoContext(ioc)
    , mTcpEndpoint(endpoint)
    , mTcpAcceptor(ioc)
    , mServerLogic(logic)
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
}

void
WebsocketServer::Run()
{
    Accept();
}

void
WebsocketServer::Accept()
{
    mTcpAcceptor.async_accept(
        boost::asio::make_strand(mIoContext),
        boost::beast::bind_front_handler(&WebsocketServer::OnAccept, shared_from_this()));
}

void
WebsocketServer::OnAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return;
    }

    HttpSession::Create(std::move(socket), mServerLogic)->Run();
    Accept();
}

HttpSession::HttpSession(boost::asio::ip::tcp::socket&& socket, IServerLogic& logic)
    : mTcpStream(std::move(socket))
    , mServerLogic(logic)
{
}

void
HttpSession::Run()
{
    boost::asio::dispatch(
        mTcpStream.get_executor(), boost::beast::bind_front_handler(&HttpSession::OnRun, shared_from_this()));
}

void
HttpSession::OnRun()
{
    Read();
}

void
HttpSession::Read()
{
    mParser.emplace();
    mParser->body_limit(10000);
    mTcpStream.expires_after(std::chrono::seconds(30));
    boost::beast::http::async_read(
        mTcpStream,
        mBuffer,
        mParser->get(),
        boost::beast::bind_front_handler(&HttpSession::OnRead, shared_from_this()));
}

void
HttpSession::OnRead(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);

    if (ec == boost::beast::http::error::end_of_stream)
    {
        mTcpStream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return;
    }

    if (boost::beast::websocket::is_upgrade(mParser->get()))
    {
        WebsocketSession::Create(mTcpStream.release_socket(), mServerLogic, mParser->release())->Run();
    }
}

WebsocketSession::WebsocketSession(
    boost::asio::ip::tcp::socket&& socket,
    IServerLogic& logic,
    boost::beast::http::message<true, boost::beast::http::string_body, boost::beast::http::fields> request)
    : mWebsocketStream(std::move(socket))
    , mServerLogic(logic)
    , mRequest(request)
{
    std::tie(mChannel, mDebugName) = Util::ExtractChannelAndUserNames(request.target().to_string());

    if (mDebugName.empty())
    {
        std::stringstream ss;
        ss << mWebsocketStream.next_layer().socket().remote_endpoint();
        mDebugName = ss.str();
    }
}

void
WebsocketSession::Run()
{
    boost::asio::dispatch(
        mWebsocketStream.get_executor(),
        boost::beast::bind_front_handler(&WebsocketSession::OnRun, shared_from_this()));
}

void
WebsocketSession::Send(const std::string& message)
{
    // From boost documentation:
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    boost::asio::post(
        mWebsocketStream.get_executor(),
        boost::beast::bind_front_handler(&WebsocketSession::Write, shared_from_this(), message));
}

void
WebsocketSession::Write(const std::string& message)
{
    mWriteQueue.push(message);

    if (mWriteQueue.size() > 1)
    {
        return;
    }

    mWebsocketStream.async_write(
        boost::asio::buffer(mWriteQueue.back()),
        boost::beast::bind_front_handler(&WebsocketSession::OnWrite, shared_from_this()));
}

std::string
WebsocketSession::GetDebugName() const
{
    return mDebugName;
}

std::string
WebsocketSession::GetChannel() const
{
    return mChannel;
}

void
WebsocketSession::OnRun()
{
    mWebsocketStream.set_option(
        boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
    mWebsocketStream.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res)
        { res.set(boost::beast::http::field::server, std::string("RenderStudio Resolver Server")); }));
    mWebsocketStream.async_accept(
        mRequest, boost::beast::bind_front_handler(&WebsocketSession::OnAccept, shared_from_this()));
}

void
WebsocketSession::OnAccept(boost::beast::error_code ec)
{
    if (ec)
    {
        LOG_ERROR << "[Networking] " << ec.message();
        return;
    }

    mServerLogic.OnConnected(shared_from_this());
    Read();
}

void
WebsocketSession::Read()
{
    mReadBuffer.consume(mReadBuffer.size());
    mWebsocketStream.async_read(
        mReadBuffer, boost::beast::bind_front_handler(&WebsocketSession::OnRead, shared_from_this()));
}

void
WebsocketSession::OnRead(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);

    if (ec)
    {
        mServerLogic.OnDisconnected(shared_from_this());
        if (ec != boost::asio::error::connection_reset && ec != boost::beast::websocket::error::closed)
        {
            LOG_ERROR << "[Networking] Disconnected: " << ec.message();
        }
        return;
    }

    mServerLogic.OnMessage(shared_from_this(), boost::beast::buffers_to_string(mReadBuffer.data()));
    Read();
}

void
WebsocketSession::OnWrite(boost::beast::error_code ec, std::size_t transferred)
{
    boost::ignore_unused(transferred);

    if (ec)
    {
        mServerLogic.OnDisconnected(shared_from_this());
        if (ec != boost::asio::error::connection_reset && ec != boost::beast::websocket::error::closed)
        {
            LOG_ERROR << "[Networking] Disconnected: " << ec.message();
        }
        return;
    }

    mWriteQueue.pop();

    if (!mWriteQueue.empty())
    {
        mWebsocketStream.async_write(
            boost::asio::buffer(mWriteQueue.front()),
            boost::beast::bind_front_handler(&WebsocketSession::OnWrite, shared_from_this()));
    }
}

} // namespace RenderStudio::Networking
