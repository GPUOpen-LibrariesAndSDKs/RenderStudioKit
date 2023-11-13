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
    for (const auto& [layer, deltas] : channel.GetHistory())
    {
        for (const auto& delta : deltas)
        {
            RenderStudio::API::Event event { "Delta::Event", delta };
            connection->Send(boost::json::serialize(boost::json::value_from(event)));
        }
    }

    // History sending finished
    RenderStudio::API::Event event { "History::Event", RenderStudio::API::HistoryEvent {} };
    connection->Send(boost::json::serialize(boost::json::value_from(event)));

    DebugPrint();
}

void
Logic::OnDisconnected(ConnectionPtr connection)
{
    // Thread safety
    std::lock_guard<std::mutex> lock(mMutex);

    // Log
    LOG_INFO << "User \'" << connection->GetDebugName() << "\' exited \'" << connection->GetChannel() << "\'";

    // Remove from channel
    if (mChannels.count(connection->GetChannel()) == 0)
    {
        LOG_ERROR << "User \'" << connection->GetDebugName() << "\' disconnected from non-existing channel \'"
                  << connection->GetChannel() << "\'";
        return;
    }

    Channel& channel = mChannels.at(connection->GetChannel());
    channel.RemoveConnection(connection);

    if (channel.Empty())
    {
        mChannels.erase(connection->GetChannel());
    }

    DebugPrint();
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
        Overload {
            [&connection, this, &message](const RenderStudio::API::DeltaEvent& v)
            {
                // Thread safety
                std::lock_guard<std::mutex> lock(mMutex);

                if (mChannels.count(connection->GetChannel()) == 0)
                {
                    LOG_ERROR << "User \'" << connection->GetDebugName()
                              << "\' sent message from non-existent channel \'" << connection->GetChannel() << "\'";
                    return;
                }

                // Process sequence
                Channel& channel = mChannels.at(connection->GetChannel());
                std::size_t sequence = channel.GetSequenceNumber(v.layer);
                RenderStudio::API::DeltaEvent acknowledgedDelta = v;
                acknowledgedDelta.sequence = sequence;

                // Broadcast update to users
                channel.AddToHistory(acknowledgedDelta);
                channel.Send(
                    connection,
                    boost::json::serialize(
                        boost::json::value_from(RenderStudio::API::Event { "Delta::Event", acknowledgedDelta })));

                // Send acknowledge
                std::vector<pxr::SdfPath> paths;

                for (const auto& [key, value] : v.updates)
                {
                    paths.push_back(key);
                }

                RenderStudio::API::Event ack { "Acknowledge::Event",
                                               RenderStudio::API::AcknowledgeEvent { v.layer, paths, sequence } };
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
            },
            [&connection, this, &message](const RenderStudio::API::ReloadEvent& v)
            {
                // Thread safety
                std::lock_guard<std::mutex> lock(mMutex);

                if (mChannels.count(connection->GetChannel()) == 0)
                {
                    LOG_ERROR << "User \'" << connection->GetDebugName()
                              << "\' sent message from non-existent channel \'" << connection->GetChannel() << "\'";
                    return;
                }

                // Process sequence
                Channel& channel = mChannels.at(connection->GetChannel());
                std::size_t sequence = channel.GetSequenceNumber(v.layer);
                RenderStudio::API::ReloadEvent acknowledgedReload = v;
                acknowledgedReload.sequence = sequence;

                // Broadcast update to users
                channel.ClearHistory(v.layer);
                channel.Send(
                    connection,
                    boost::json::serialize(
                        boost::json::value_from(RenderStudio::API::Event { "Reload::Event", acknowledgedReload })));
            } },
        event.value().body);
}

void
Logic::DebugPrint() const
{
    if (mChannels.empty())
    {
        return;
    }

    LOG_DEBUG << ":: Global channels stats ::";
    for (const auto& [name, channel] : mChannels)
    {
        LOG_DEBUG << "Channel '" << name << "' stats";
        LOG_DEBUG << " - Connected users: " << channel.GetConnections().size();
        std::string layers = "[";
        for (auto it = channel.GetHistory().begin(); it != channel.GetHistory().end(); ++it)
        {
            layers += "(name: " + it->first + ", history: " + std::to_string(it->second.size()) + ")";
            if (std::next(it) != channel.GetHistory().end())
            {
                layers += ", ";
            }
        }
        layers += "]";
        LOG_DEBUG << " - Used layers: " << layers;
    }
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
        LOG_WARNING << "Can't parse: [" << ex.what() << "]: " << message;
        return {};
    }
}
