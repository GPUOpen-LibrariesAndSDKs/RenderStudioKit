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
