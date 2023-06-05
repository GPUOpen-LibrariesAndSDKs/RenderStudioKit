#include <iostream>

#pragma warning(push, 0)
#include <list>
#include <numeric>

#include <boost/asio/ip/address.hpp>
#include <boost/json/src.hpp>
#pragma warning(pop)

#include <Logger/Logger.h>
#include <Networking/WebsocketServer.h>
#include <Serialization/Api.h>

using Connection = RenderStudio::Networking::WebsocketSession;
using ConnectionPtr = std::shared_ptr<Connection>;

class SessionManager
{
public:
    void AddSession(const ConnectionPtr& connection)
    {
        mSessions[connection->GetChannel()].push_back(connection);
        LOG_INFO << connection->GetDebugName() << " joined " << connection->GetChannel();
    }

    void RemoveSession(const ConnectionPtr& connection)
    {
        mSessions[connection->GetChannel()].remove_if(
            [&connection](const auto& session) { return session->GetDebugName() == connection->GetDebugName(); });
        LOG_INFO << connection->GetDebugName() << " left " << connection->GetChannel();
    }

    void SendToChannel(const ConnectionPtr& connection, const std::string& message)
    {
        for (ConnectionPtr& session : mSessions[connection->GetChannel()])
        {
            if (session->GetDebugName() == connection->GetDebugName())
            {
                continue;
            }

            session->Write(message);
        }
    }

private:
    std::map<std::string, std::list<ConnectionPtr>> mSessions;
};

class Logic : public RenderStudio::Networking::IServerLogic
{
public:
    void OnConnected(const ConnectionPtr& connection) override
    {
        mSessionManager.AddSession(connection);
        connection->Write("Welcome to our server");
    }

    void OnDisconnected(const ConnectionPtr& connection) override { mSessionManager.RemoveSession(connection); }

    void OnMessage(const ConnectionPtr& connection, const std::string& message)
    {
        std::string identifier;
        RenderStudioApi::DeltaType deltas;
        std::size_t sequence = 0;

        try
        {
            std::tie(identifier, deltas, sequence) = RenderStudioApi::DeserializeDeltas(message);
        }
        catch (const std::exception& ex)
        {
            LOG_WARNING << "Can't parse: " << message << "[" << ex.what() << "]";
        }

        mSessionManager.SendToChannel(connection, message);
    }

private:
    SessionManager mSessionManager;
};

auto
main() -> int
try
{
    const boost::asio::ip::address address = boost::asio::ip::make_address("0.0.0.0");
    const std::uint16_t port = 10000;
    const std::int32_t threads = 10;

    LOG_INFO << "Server started at " << address << ":" << port << " (threads: " << threads << ")";

    boost::asio::io_context ioc { threads };
    Logic logic;

    auto server = RenderStudio::Networking::WebsocketServer::Create(
        ioc, boost::asio::ip::tcp::endpoint { address, port }, logic);

    server->Run();

    std::vector<std::thread> workers;
    workers.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        workers.emplace_back([&ioc]() { ioc.run(); });
    }
    ioc.run();

    return EXIT_SUCCESS;
}
catch (const std::exception& ex)
{
    LOG_FATAL << "[Server Fatal Exception] " << ex.what();
    return EXIT_FAILURE;
}
