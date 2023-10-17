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

#include "Registry.h"

#pragma warning(push, 0)

#if (VS_PLATFORM_TOOLSET > 141)
#include <map>
#else
#include <experimental/map>
#endif

#pragma warning(pop)

#include <Logger/Logger.h>

PXR_NAMESPACE_OPEN_SCOPE

void
RenderStudioLayerRegistry::AddLayer(SdfLayerHandle layer)
{
    mCreatedLayers[layer->GetIdentifier()] = layer;
}

void
RenderStudioLayerRegistry::RemoveExpiredLayers()
{
#if (VS_PLATFORM_TOOLSET > 141)
    std::erase_if(mCreatedLayers, [](const auto& pair) { return pair.second.IsExpired(); });
#else
    std::experimental::erase_if(mCreatedLayers, [](const auto& pair) { return pair.second.IsExpired(); });
#endif
}
void
RenderStudioLayerRegistry::ForEachLayer(const std::function<void(SdfLayerHandle)>& fn)
{
    RemoveExpiredLayers();

    for (const auto& [identifier, layer] : mCreatedLayers)
    {
        if (!layer.IsExpired())
        {
            fn(layer);
        }
    }
}

SdfLayerHandle
RenderStudioLayerRegistry::GetByIdentifier(const std::string& identifier)
{
    if (mCreatedLayers.count(identifier))
    {
        return mCreatedLayers[identifier];
    }
    else
    {
        return {};
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
