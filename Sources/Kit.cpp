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

#include <Resolver/Resolver.h>

namespace RenderStudio::Kit
{

void
LiveSessionConnect(const LiveSessionInfo& info)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::StartLiveMode(info);
}

bool
LiveSessionUpdate()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::ProcessLiveUpdates();
}

void
LiveSessionDisconnect()
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::StopLiveMode();
}

bool
IsUnresovableToRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::IsUnresovableToRenderStudioPath(path);
}

std::string
UnresolveToRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::Unresolve(path);
}

bool
IsRenderStudioPath(const std::string& path)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    return RenderStudioResolver::IsRenderStudioPath(path);
}

} // namespace RenderStudio::Kit
