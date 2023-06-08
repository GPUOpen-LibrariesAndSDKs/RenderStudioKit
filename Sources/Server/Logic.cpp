#include "Logic.h"

template <typename... Ts> struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

void
Logic::OnConnected(ConnectionPtr connection)
{
    // Thread safety
    std::lock_guard<std::mutex> lock(mMutex);

    // Get or create channel
    auto [iter, inserted] = mChannels.try_emplace(connection->GetChannel(), connection->GetChannel());
    Channel& channel = iter->second;

    // Log
    LOG_INFO << "User \'" << connection->GetDebugName() << "\' joined \'" << connection->GetChannel() << "\'";
    channel.AddConnection(connection);

    // Send history
    for (const RenderStudio::API::DeltaEvent& delta : channel.GetHistory())
    {
        RenderStudio::API::Event event { "Delta::Event", delta };
        connection->Send(boost::json::serialize(boost::json::value_from(event)));
    }

    // History sending finished
    RenderStudio::API::Event event { "History::Event", RenderStudio::API::HistoryEvent {} };
    connection->Send(boost::json::serialize(boost::json::value_from(event)));
}

void
Logic::OnDisconnected(ConnectionPtr connection)
{
    // Thread safety
    std::lock_guard<std::mutex> lock(mMutex);

    // Log
    LOG_INFO << "User \'" << connection->GetDebugName() << "\' exited \'" << connection->GetChannel() << "\'";

    // Remove from channel
    Channel& channel = mChannels.at(connection->GetChannel());
    channel.RemoveConnection(connection);

    if (channel.Empty())
    {
        mChannels.erase(connection->GetChannel());
    }
}

void
Logic::OnMessage(ConnectionPtr connection, const std::string& message)
{
    auto event = ParseEvent(message);
    if (!event.has_value())
    {
        return;
    }

    std::visit(
        Overload { [&connection, this, &message](const RenderStudio::API::DeltaEvent& v)
                   {
                       // Thread safety
                       std::lock_guard<std::mutex> lock(mMutex);

                       // Process sequence
                       Channel& channel = mChannels.at(connection->GetChannel());
                       std::size_t sequence = channel.GetSequenceNumber();
                       RenderStudio::API::DeltaEvent acknowledgedDelta = v;
                       acknowledgedDelta.sequence = sequence;

                       // Broadcast update to users
                       channel.AddToHistory(acknowledgedDelta);
                       channel.Send(
                           connection,
                           boost::json::serialize(boost::json::value_from(
                               RenderStudio::API::Event { "Delta::Event", acknowledgedDelta })));

                       // Send acknowledge
                       std::vector<pxr::SdfPath> paths;

                       for (const auto& [key, value] : v.updates)
                       {
                           paths.push_back(key);
                       }

                       RenderStudio::API::Event ack {
                           "Acknowledge::Event", RenderStudio::API::AcknowledgeEvent { v.layer, paths, sequence }
                       };
                       connection->Send(boost::json::serialize(boost::json::value_from(ack)));
                   },
                   [](const RenderStudio::API::HistoryEvent& v)
                   {
                       (void)v;
                       // Do nothing. Only clients receive history
                   },
                   [](const RenderStudio::API::AcknowledgeEvent& v)
                   {
                       (void)v;
                       // Do nothing. Only clients receive acknowledges
                   } },
        event.value().body);
}

std::optional<RenderStudio::API::Event>
Logic::ParseEvent(const std::string& message)
{
    try
    {
        boost::json::value json = boost::json::parse(message);
        return boost::json::value_to<RenderStudio::API::Event>(json);
    }
    catch (const std::exception& ex)
    {
        LOG_WARNING << "Can't parse: " << message << "[" << ex.what() << "]";
        return {};
    }
}
