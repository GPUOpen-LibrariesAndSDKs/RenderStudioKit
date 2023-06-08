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
        LOG_WARNING << "Can't find layer with id: " << identifier;
        return {};
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
