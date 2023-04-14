#include "FileFormat.h"

#pragma warning(push, 0)
#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/usd/usdcFileFormat.h>

#include <boost/json/src.hpp>
#pragma warning(pop)

#include <Resolver.h>

#include "Data.h"

#include <Logger/Logger.h>
#include <Serialization/Api.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, RENDER_STUDIO_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType) { SDF_DEFINE_FILE_FORMAT(RenderStudioFileFormat, SdfFileFormat); }

namespace
{
static SdfFileFormatConstPtr
_GetFileFormat(const TfToken& formatId)
{
    const SdfFileFormatConstPtr fileFormat = SdfFileFormat::FindById(formatId);
    TF_VERIFY(fileFormat);
    return fileFormat;
}

static const UsdUsdFileFormatConstPtr&
_GetUsdFileFormat()
{
    static const auto usdFormat = TfDynamic_cast<UsdUsdFileFormatConstPtr>(_GetFileFormat(UsdUsdFileFormatTokens->Id));
    return usdFormat;
}

static const SdfFileFormatConstPtr&
_GetMtlxFileFormat()
{
    static const auto mtlxFormat = _GetFileFormat(TfToken { "mtlx" });
    return mtlxFormat;
}

} // namespace

RenderStudioDataPtr
RenderStudioFileFormat::_GetRenderStudioData(SdfLayerHandle layer)
{
    SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(*layer);
    RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);

    if (casted == nullptr)
    {
        throw std::runtime_error("Layer has no RenderStudio data");
    }

    return TfConst_cast<RenderStudioDataPtr>(casted);
}

void
RenderStudioFileFormat::OnMessage(const std::string& message)
{
    LOG_DEBUG << "Received message: " << message;

    // Receive deltas
    std::string identifier;
    RenderStudioApi::DeltaType deltas;
    std::size_t sequence = 0;

    try
    {
        std::tie(identifier, deltas, sequence) = RenderStudioApi::DeserializeDeltas(message);
    }
    catch (const std::exception& ex)
    {
        LOG_WARNING << "Can't parse: " << message << "[" << ex.what() << "]";
    }

    // Append deltas to the data
    SdfLayerHandle layer = mLayerRegistry.GetByIdentifier(identifier);

    if (layer != nullptr)
    {
        _GetRenderStudioData(layer)->AccumulateRemoteUpdate(layer, deltas, sequence);
    }
}

void
RenderStudioFileFormat::ProcessLiveUpdates()
{
    mLayerRegistry.ForEachLayer(
        [this](SdfLayerHandle layer)
        {
            RenderStudioDataPtr data = _GetRenderStudioData(layer);

            // Send local deltas
            auto deltas = data->FetchLocalDeltas();
            if (!deltas.empty())
            {
                try
                {
                    boost::json::object deltasJson
                        = RenderStudioApi::SerializeDeltas(layer, deltas, RenderStudioResolver::GetCurrentUserId());
                    mWebsocketClient->SendMessageString(boost::json::serialize(deltasJson));
                }
                catch (const std::exception& ex)
                {
                    LOG_WARNING << ex.what();
                }
            }

            // Apply remote deltas
            data->ProcessRemoteUpdates(layer);
        });
}

void
RenderStudioFileFormat::Connect(const std::string& url)
{
    mLayerRegistry.RemoveExpiredLayers();

    // Create client
    mWebsocketClient = std::make_shared<RenderStudio::Networking::WebsocketClient>([this](const std::string& message)
                                                                                   { OnMessage(message); });

    // Connect to endpoint
    try
    {
        auto endpoint = RenderStudio::Networking::Url::Parse(url);
        mWebsocketClient->Connect(endpoint);
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR << "Can't connect to remote " + url + ": " << ex.what();
    }
}

void
RenderStudioFileFormat::Disconnect()
{
    mWebsocketClient->Disconnect();
    mWebsocketClient.reset();
}

SdfAbstractDataRefPtr
RenderStudioFileFormat::InitData(const FileFormatArguments& args) const
{
    // Copy-pasted from USD. By default USD requires at least pseudo root spec.
    RenderStudioData* metadata = new RenderStudioData;
    metadata->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(metadata);
}

SdfLayer*
RenderStudioFileFormat::_InstantiateNewLayer(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier,
    const std::string& realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments& args) const
{
    // During creation of layer save it for further usage
    SdfLayer* layer = SdfFileFormat::_InstantiateNewLayer(fileFormat, identifier, realPath, assetInfo, args);
    mLayerRegistry.AddLayer(SdfLayerHandle { layer });
    return layer;
}

bool
RenderStudioFileFormat::CanRead(const std::string& file) const
{
    // Delegate reading to USD
    return _GetUsdFileFormat()->CanRead(file);
}

bool
RenderStudioFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const
{
    bool result = false;

    // Delegate reading to USD
    if (resolvedPath.rfind("gpuopen:/", 0) == 0)
    {
        result = _GetMtlxFileFormat()->Read(layer, resolvedPath, metadataOnly);
    }
    else
    {
        result = _GetUsdFileFormat()->Read(layer, resolvedPath, metadataOnly);
    }

    // USD uses own format of data. Current approach is to copy it into RenderStudioData
    SdfAbstractDataConstPtr abstractData = SdfFileFormat::_GetLayerData(*layer);
    SdfAbstractDataRefPtr renderStudioData = InitData(GetDefaultFileFormatArguments());
    renderStudioData->CopyFrom(abstractData);
    SdfFileFormat::_SetLayerData(layer, renderStudioData);

    return result;
}

RenderStudioFileFormat::RenderStudioFileFormat()
    : SdfFileFormat(
        RenderStudioFileFormatTokens->Id,
        RenderStudioFileFormatTokens->Version,
        RenderStudioFileFormatTokens->Target,
        RenderStudioFileFormatTokens->Id)
{
}

RenderStudioFileFormat::~RenderStudioFileFormat() { mWebsocketClient->Disconnect(); }

PXR_NAMESPACE_CLOSE_SCOPE
