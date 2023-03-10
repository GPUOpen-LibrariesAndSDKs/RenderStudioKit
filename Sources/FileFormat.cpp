#include "FileFormat.h"
#include "Data.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/asset.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, RENDER_STUDIO_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(RenderStudioFileFormat, SdfTextFileFormat);
}


void
RenderStudioFileFormat::ProcessLiveUpdates()
{
    // Remove expired layers
    mCreatedLayers.erase(std::remove_if(mCreatedLayers.begin(), mCreatedLayers.end(), [](SdfLayerHandle& layer)
    {
        return layer.IsExpired();
    }), mCreatedLayers.end());

    std::for_each(mCreatedLayers.begin(), mCreatedLayers.end(), [](SdfLayerHandle& layer)
    {
        SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(*layer);
        RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);
        RenderStudioDataPtr data = TfConst_cast<RenderStudioDataPtr>(casted);
        data->ProcessLiveUpdates(layer);
    });
}

SdfAbstractDataRefPtr
RenderStudioFileFormat::InitData(const FileFormatArguments &args) const
{
    RenderStudioData* metadata = new RenderStudioData;
    metadata->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(metadata);
}

SdfLayer*
RenderStudioFileFormat::_InstantiateNewLayer(
    const SdfFileFormatConstPtr& fileFormat, 
    const std::string& identifier, const std::string& realPath, 
    const ArAssetInfo& assetInfo, const FileFormatArguments& args) const
{
    SdfLayer* layer = SdfFileFormat::_InstantiateNewLayer(fileFormat, identifier, realPath, assetInfo, args);
    mCreatedLayers.push_back(SdfLayerHandle{ layer });
    return layer;
}

RenderStudioFileFormat::RenderStudioFileFormat()
    : SdfTextFileFormat(RenderStudioFileFormatTokens->Id,
                        RenderStudioFileFormatTokens->Version,
                        UsdUsdFileFormatTokens->Target)
{
    // Do Nothing.
}

RenderStudioFileFormat::~RenderStudioFileFormat()
{
    // Do Nothing.
}

PXR_NAMESPACE_CLOSE_SCOPE

