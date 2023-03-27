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

void
RenderStudioFileFormat::ProcessLiveUpdates()
{
    // Remove expired layers
    mCreatedLayers.erase(
        std::remove_if(
            mCreatedLayers.begin(), mCreatedLayers.end(), [](SdfLayerHandle& layer) { return layer.IsExpired(); }),
        mCreatedLayers.end());

    // Update each layer (incoming + outcoming deltas)
    std::for_each(
        mCreatedLayers.begin(),
        mCreatedLayers.end(),
        [this](SdfLayerHandle& layer)
        {
            // Cast data
            SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(*layer);
            RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);
            RenderStudioDataPtr data = TfConst_cast<RenderStudioDataPtr>(casted);

            // Send local deltas
            // First fetch would just clear them, because they already stored on clients PC's - no need to sync
            auto deltas = data->FetchLocalDeltas();

            if (!deltas.empty())
            {
                try
                {
                    boost::json::object deltasJson = RenderStudioApi::SerializeDeltas(layer, deltas);
                    mWebsocketClient->SendMessageString(boost::json::serialize(deltasJson));
                }
                catch (const std::exception& ex)
                {
                    LOG_WARNING << ex.what();
                }
            }

            // Apply remote deltas
            data->ApplyRemoteDeltas(layer);
        });
}

void RenderStudioFileFormat::Connect(const std::string& url) {
    // Create client
    mWebsocketClient = std::make_shared<RenderStudio::Networking::WebsocketClient>(
        [this](const std::string& message)
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
                LOG_WARNING << "Can't parse: " << message;
            }

            // Get layer by name
            auto it = std::find_if(
                mCreatedLayers.begin(),
                mCreatedLayers.end(),
                [&](SdfLayerHandle layer) { return layer->GetIdentifier() == identifier; });

            if (it == mCreatedLayers.end())
            {
                LOG_WARNING << "Can't find layer with id: " << identifier;
                return;
            }

            // Cast data and append deltas
            SdfLayerHandle layer = *it;
            SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(*layer);
            RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);
            RenderStudioDataPtr data = TfConst_cast<RenderStudioDataPtr>(casted);
            data->AddRemoteSequence(layer, deltas, sequence);
        });

    // Remove expired layers
    mCreatedLayers.erase(
        std::remove_if(
            mCreatedLayers.begin(), mCreatedLayers.end(), [](SdfLayerHandle& layer) { return layer.IsExpired(); }),
        mCreatedLayers.end());

    // Connect to endpoint
    auto endpoint = RenderStudio::Networking::WebsocketEndpoint::FromString(url);
    mWebsocketClient->Connect(endpoint);
}

void
RenderStudioFileFormat::Disconnect()
{
    mWebsocketClient->Disconnect();
    mWebsocketClient = nullptr;
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
    mCreatedLayers.push_back(SdfLayerHandle { layer });
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
