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

#include "Channel.h"

Channel::Channel(const std::string& name)
    : mName(name)
{
    LOG_INFO << "Created channel \'" << mName << "\'";
}

Channel::~Channel() { LOG_INFO << "Removed channel \'" << mName << "\'"; }

void
Channel::AddConnection(ConnectionPtr connection)
{
    mConnections.push_back(connection);
}

void
Channel::RemoveConnection(ConnectionPtr connection)
{
    mConnections.remove_if([&connection](const ConnectionPtr& entry)
                           { return entry->GetDebugName() == connection->GetDebugName(); });
}

void
Channel::Send(ConnectionPtr connection, const std::string& message)
{
    for (ConnectionPtr& entry : mConnections)
    {
        // Skip sender
        if (entry->GetDebugName() == connection->GetDebugName())
        {
            continue;
        }
        entry->Send(message);
    }
}

const std::vector<RenderStudio::API::DeltaEvent>&
Channel::GetHistory() const
{
    return mHistory;
}

void
Channel::AddToHistory(const RenderStudio::API::DeltaEvent& v)
{
    mHistory.push_back(v);
}

bool
Channel::Empty() const
{
    return mConnections.empty();
}

std::size_t
Channel::GetSequenceNumber() const
{
    return mHistory.size() + 1;
}
