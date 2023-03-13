#include "FileFormat.h"
#include "Data.h"
#include "Logger/Logger.h"
#include "Serialization/Api.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/asset.h>

#include <boost/json/src.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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

    std::for_each(mCreatedLayers.begin(), mCreatedLayers.end(), [this](SdfLayerHandle& layer)
    {
        // Cast data
        SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(*layer);
        RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);
        RenderStudioDataPtr data = TfConst_cast<RenderStudioDataPtr>(casted);

        // Send local deltas
        auto deltas = data->FetchLocalDeltas();
        static bool firstLaunch = true;

        if (!deltas.empty() && !firstLaunch)
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
            catch (...)
            {
                LOG_FATAL << "Unknown error";
            }
        }

        firstLaunch = false;

        // Apply remote deltas
        data->ApplyRemoteDeltas(layer);
    });
}

void RenderStudioFileFormat::Connect(const std::string& url)
{
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    mWebsocketClient->Connect(RenderStudio::Networking::WebsocketEndpoint::FromString(url));
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
    mWebsocketClient = std::make_shared<RenderStudio::Networking::WebsocketClient>(
        [this](const std::string& message)
        {
            // Receive deltas
            std::string identifier;
            RenderStudioApi::DeltaType deltas;

            try
            {
                std::tie(identifier, deltas) = RenderStudioApi::DeserializeDeltas(message);
            }
            catch (const std::exception& ex)
            {
                LOG_WARNING << "Can't parse: " << message;
            }

            // Get layer by name
            auto it = std::find_if(mCreatedLayers.begin(), mCreatedLayers.end(), [&](SdfLayerHandle layer)
            { 
                return layer->GetIdentifier() == identifier;
            });

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
            data->AppendRemoteDeltas(layer, deltas);
        }
    );
}

RenderStudioFileFormat::~RenderStudioFileFormat()
{
    mWebsocketClient->Disconnect();
}

PXR_NAMESPACE_CLOSE_SCOPE

