#pragma once

#include <Logger/Logger.h>
#include <Networking/WebsocketServer.h>
#include <Serialization/Api.h>

using Connection = RenderStudio::Networking::WebsocketSession;
using ConnectionPtr = std::shared_ptr<Connection>;

class Channel
{
public:
    Channel(const std::string& name);
    ~Channel();
    Channel(const Channel& rgs) = delete;

    void AddConnection(ConnectionPtr connection);
    void RemoveConnection(ConnectionPtr connection);
    void Send(ConnectionPtr connection, const std::string& message);
    const std::vector<RenderStudio::API::DeltaEvent>& GetHistory() const;
    void AddToHistory(const RenderStudio::API::DeltaEvent& v);
    bool Empty() const;
    std::size_t GetSequenceNumber() const;

private:
    std::vector<RenderStudio::API::DeltaEvent> mHistory;
    std::list<ConnectionPtr> mConnections;
    std::string mName;
};
