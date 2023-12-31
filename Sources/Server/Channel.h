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
    const std::map<std::string, std::vector<RenderStudio::API::DeltaEvent>>& GetHistory() const;
    const std::list<ConnectionPtr>& GetConnections() const;
    void AddToHistory(const RenderStudio::API::DeltaEvent& v);
    void ClearHistory(const std::string& layer);
    bool Empty() const;
    std::size_t GetSequenceNumber(const std::string& layer) const;

private:
    std::map<std::string, std::vector<RenderStudio::API::DeltaEvent>> mHistory;
    std::list<ConnectionPtr> mConnections;
    std::string mName;
};
