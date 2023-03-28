#pragma once

#pragma warning(push, 0)
#include <pxr/usd/sdf/layer.h>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioLayerRegistry
{
public:
    void AddLayer(SdfLayerHandle layer);
    void RemoveExpiredLayers();
    void ForEachLayer(const std::function<void(SdfLayerHandle)>& fn);
    SdfLayerHandle GetByIdentifier(const std::string& identifier);

private:
    mutable std::map<std::string, SdfLayerHandle> mCreatedLayers;
};

PXR_NAMESPACE_CLOSE_SCOPE
