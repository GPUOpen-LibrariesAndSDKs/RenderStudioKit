#pragma once

#include "Channel.h"

#include <Logger/Logger.h>
#include <Networking/WebsocketServer.h>

class Logic : public RenderStudio::Networking::IServerLogic
{
public:
    void OnConnected(ConnectionPtr connection) override;
    void OnDisconnected(ConnectionPtr connection) override;
    void OnMessage(ConnectionPtr connection, const std::string& message);

private:
    std::optional<RenderStudio::API::Event> ParseEvent(const std::string& message);

    std::map<std::string, Channel> mChannels;
    std::mutex mMutex;
};
