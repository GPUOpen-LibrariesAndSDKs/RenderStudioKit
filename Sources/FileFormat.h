#pragma once

#pragma warning(push, 0)
#include <vector>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/usd/usd/api.h>
#include <pxr/usd/usd/usdFileFormat.h>
#pragma warning(pop)

#include "Data.h"
#include "Networking/WebsocketClient.h"
#include "Registry.h"

PXR_NAMESPACE_OPEN_SCOPE

#define RENDER_STUDIO_FILE_FORMAT_TOKENS ((Id, "studio"))((Version, "1.0"))((Target, "usd"))

#pragma warning(push, 0)
TF_DECLARE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, SDF_API, RENDER_STUDIO_FILE_FORMAT_TOKENS);
#pragma warning(pop)

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioFileFormat);

class ArAsset;

class RenderStudioFileFormat : public SdfFileFormat
{
public:
    SDF_API
    SdfAbstractDataRefPtr InitData(const FileFormatArguments& args) const override;

    SDF_API
    virtual SdfLayer* _InstantiateNewLayer(
        const SdfFileFormatConstPtr& fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo,
        const FileFormatArguments& args) const override;

    SDF_API
    virtual bool CanRead(const std::string& file) const override;

    SDF_API
    virtual bool Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const override;

private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    RenderStudioFileFormat();
    virtual ~RenderStudioFileFormat();

    void ProcessLiveUpdates();
    void Connect(const std::string& url);
    void Disconnect();
    RenderStudioDataPtr _GetRenderStudioData(SdfLayerHandle layer) const;
    void OnMessage(const std::string& message);

    friend class RenderStudioResolver;
    mutable RenderStudioLayerRegistry mLayerRegistry;
    std::shared_ptr<RenderStudio::Networking::WebsocketClient> mWebsocketClient;
};

PXR_NAMESPACE_CLOSE_SCOPE
