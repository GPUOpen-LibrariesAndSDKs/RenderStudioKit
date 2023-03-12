#pragma once
 
#include <pxr/pxr.h>
#include <pxr/usd/usd/api.h>
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/usd/sdf/layer.h>

#include <vector>
#include "Data.h"
#include "Networking/WebsocketClient.h"

PXR_NAMESPACE_OPEN_SCOPE

#define RENDER_STUDIO_FILE_FORMAT_TOKENS \
    ((Id,      "studio"))                \
    ((Version, "1.0"))                   \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, USD_API, RENDER_STUDIO_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioFileFormat);

class ArAsset;

class RenderStudioFileFormat : public SdfTextFileFormat
{
public:
    SDF_API
    SdfAbstractDataRefPtr InitData(
        const FileFormatArguments &args) const override;
    
    SDF_API
    virtual SdfLayer* _InstantiateNewLayer(
        const SdfFileFormatConstPtr& fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo,
        const FileFormatArguments& args) const override;

private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    RenderStudioFileFormat();
    virtual ~RenderStudioFileFormat();

    void ProcessLiveUpdates();
    void Connect(const std::string& url);

    friend class UsdUsdFileFormat;
    friend class RenderStudioResolver;

    mutable std::vector<SdfLayerHandle> mCreatedLayers;
    std::shared_ptr<RenderStudio::Networking::WebsocketClient> mWebsocketClient;
};

PXR_NAMESPACE_CLOSE_SCOPE
