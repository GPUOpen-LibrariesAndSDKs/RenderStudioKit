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

#include "Kit.h"

#include <Logger/Logger.h>
#include <Networking/Syncthing.h>
#include <Resolver/Resolver.h>

namespace RenderStudio::Kit
{

void
LiveSessionConnect(const LiveSessionInfo& info)
{
    pxr::RenderStudioResolver::StartLiveMode(info);
}

bool
LiveSessionUpdate()
{
    return pxr::RenderStudioResolver::ProcessLiveUpdates();
}

void
LiveSessionDisconnect()
{
    pxr::RenderStudioResolver::StopLiveMode();
}

void
SharedWorkspaceConnect()
{
    RenderStudio::Networking::Syncthing::Connect();
}

void
SharedWorkspaceDisconnect()
{
    RenderStudio::Networking::Syncthing::Disconnect();
}

bool
IsUnresolvable(const std::string& path)
{
    return pxr::RenderStudioResolver::IsUnresolvable(path);
}

std::string
Unresolve(const std::string& path)
{
    return pxr::RenderStudioResolver::Unresolve(path);
}

bool
IsRenderStudioPath(const std::string& path)
{
    return pxr::RenderStudioResolver::IsRenderStudioPath(path);
}

void
SetWorkspacePath(const std::string& path)
{
    pxr::RenderStudioResolver::SetWorkspacePath(path);
    RenderStudio::Networking::Syncthing::SetWorkspacePath(path);
    LOG_INFO << "[RenderStudio Kit] Set workspace path to: " << path;
}

void
SetWorkspaceUrl(const std::string& url)
{
    RenderStudio::Networking::Syncthing::SetWorkspaceUrl(url);
    LOG_INFO << "[RenderStudio Kit] Set URL of remote workspace server to: " << url;
}

std::string
GetWorkspacePath()
{
    return pxr::RenderStudioResolver::GetWorkspacePath();
}

std::string
GetWorkspaceUrl()
{
    return RenderStudio::Networking::Syncthing::GetWorkspaceUrl();
}

} // namespace RenderStudio::Kit
