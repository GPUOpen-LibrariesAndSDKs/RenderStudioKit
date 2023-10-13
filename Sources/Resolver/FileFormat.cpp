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

#include "FileFormat.h"

#pragma warning(push, 0)
#include <filesystem>

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
#include "Resolver.h"

#include <Logger/Logger.h>
#include <Serialization/Api.h>

PXR_NAMESPACE_OPEN_SCOPE

#pragma warning(push, 0)
TF_DEFINE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, RENDER_STUDIO_FILE_FORMAT_TOKENS);
#pragma warning(pop)

TF_REGISTRY_FUNCTION(TfType) { SDF_DEFINE_FILE_FORMAT(RenderStudioFileFormat, SdfFileFormat); }

namespace
{

static const SdfFileFormatConstPtr
_GetOriginalFormat(const std::string& path)
{
    std::string extension = std::filesystem::path(path).extension().string();
    return SdfFileFormat::FindByExtension(extension);
}

static std::optional<RenderStudio::API::Event>
ParseEvent(const std::string& message)
{
    try
    {
        boost::json::value json = boost::json::parse(message);
        return boost::json::value_to<RenderStudio::API::Event>(json);
    }
    catch (const std::exception& ex)
    {
        LOG_WARNING << "Can't parse: " << message << " [" << ex.what() << "]";
        return {};
    }
}

template <typename... Ts> struct Overload : Ts...
{
    using Ts::operator()...;
};

template <class... Ts> Overload(Ts...) -> Overload<Ts...>;

} // namespace

RenderStudioDataPtr
RenderStudioFileFormat::_GetRenderStudioData(SdfLayerHandle layer) const
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
    auto event = ParseEvent(message);

    if (!event.has_value())
    {
        return;
    }

    std::visit(
        Overload { [this](const RenderStudio::API::DeltaEvent& v)
                   {
                       if (!v.sequence.has_value())
                       {
                           LOG_WARNING << "Got update without sequence number";
                           return;
                       }

                       SdfLayerHandle layer = mLayerRegistry.GetByIdentifier(v.layer);

                       if (layer == nullptr)
                       {
                           LOG_WARNING << "Got update for unknown layer: " << v.layer;
                           return;
                       }

                       // Convert API update to internal format
                       RenderStudioData::_HashTable updates;
                       for (const auto& [key, value] : v.updates)
                       {
                           RenderStudioData::_SpecData spec;
                           spec.specType = value.specType;
                           spec.fields = value.fields;
                           updates[key] = spec;
                       }

                       // Append deltas to the data
                       _GetRenderStudioData(layer)->AccumulateRemoteUpdate(updates, v.sequence.value());
                   },
                   [](const RenderStudio::API::HistoryEvent& v)
                   {
                       // Send history notice
                       (void)v;
                       RenderStudioLoadingNotice("History", "RenderStudio::Internal");
                   },
                   [this](const RenderStudio::API::AcknowledgeEvent& v)
                   {
                       SdfLayerHandle layer = mLayerRegistry.GetByIdentifier(v.layer);

                       if (layer == nullptr)
                       {
                           LOG_WARNING << "Got acknowledge for unknown layer: " << v.layer;
                           return;
                       }

                       // Convert API update to internal format
                       RenderStudioData::_HashTable updates;
                       for (const auto& path : v.paths)
                       {
                           updates[path] = RenderStudioData::_SpecData {};
                       }

                       // Append deltas to the data
                       _GetRenderStudioData(layer)->AccumulateRemoteUpdate(updates, v.sequence);
                   } },
        event.value().body);
}

bool
RenderStudioFileFormat::ProcessLiveUpdates()
{
    bool updated = false;

    mLayerRegistry.ForEachLayer(
        [this, &updated](SdfLayerHandle layer)
        {
            RenderStudioDataPtr data = _GetRenderStudioData(layer);

            if (data == nullptr)
            {
                LOG_ERROR << "Data was null for layer: " << layer->GetIdentifier();
                return;
            }

            // Send local deltas
            auto deltas = data->FetchLocalDeltas();
            if (!deltas.empty())
            {
                try
                {
                    // Convert internal format to API update
                    RenderStudio::API::DeltaEvent body;
                    body.layer = layer->GetIdentifier();
                    body.user = RenderStudioResolver::GetCurrentUserId();
                    body.sequence = std::nullopt;

                    for (const auto& [key, value] : deltas)
                    {
                        body.updates[key] = RenderStudio::API::SpecData { value.specType, value.fields };
                    }

                    RenderStudio::API::Event event { "Delta::Event", body };
                    mWebsocketClient->Send(boost::json::serialize(boost::json::value_from(event)));
                }
                catch (const std::exception& ex)
                {
                    LOG_WARNING << ex.what();
                }
            }

            // Apply remote deltas
            std::size_t sequence = data->GetSequence();
            data->ProcessRemoteUpdates(layer);

            // Changed sequence number means there was an applied update
            if (sequence != data->GetSequence())
            {
                updated = true;
            }
        });

    return updated;
}

void
RenderStudioFileFormat::Connect(const std::string& url)
{
    mLayerRegistry.RemoveExpiredLayers();

    // Create client
    mWebsocketClient
        = RenderStudio::Networking::WebsocketClient::Create([this](const std::string& message) { OnMessage(message); });

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
    if (mWebsocketClient == nullptr)
    {
        LOG_WARNING << "Tried to disconnect WebSocket that doesn't exist, skipping";
        return;
    }

    mWebsocketClient->Disconnect();
    mWebsocketClient.reset();
}

SdfAbstractDataRefPtr
RenderStudioFileFormat::InitData(const FileFormatArguments& args) const
{
    (void)args;
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
    return layer;
}

bool
RenderStudioFileFormat::CanRead(const std::string& file) const
{
    SdfFileFormatConstPtr format = _GetOriginalFormat(file);

    if (format == nullptr)
    {
        return false;
    }

    return format->CanRead(file);
}

bool
RenderStudioFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const
{
    SdfFileFormatConstPtr format = _GetOriginalFormat(resolvedPath);

    if (format == nullptr)
    {
        return false;
    }

    bool result = format->Read(layer, resolvedPath, metadataOnly);

    if (!result)
    {
        return result;
    }

    // USD uses own format of data. Current approach is to copy it into RenderStudioData
    SdfAbstractDataConstPtr abstractData = SdfFileFormat::_GetLayerData(*layer);
    SdfAbstractDataRefPtr renderStudioData = InitData(GetDefaultFileFormatArguments());
    renderStudioData->CopyFrom(abstractData);
    SdfFileFormat::_SetLayerData(layer, renderStudioData);

    // Tell layer that loading is finished and all new updates would be considered as user edit
    if (mLayerRegistry.GetByIdentifier(layer->GetIdentifier()) == nullptr)
    {
        _GetRenderStudioData(SdfLayerHandle { layer })->OnLoaded();
        mLayerRegistry.AddLayer(SdfLayerHandle { layer });
    }

    return result;
}

bool
RenderStudioFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    std::string resolvedPath = RenderStudioResolver::ResolveImpl(filePath);
    SdfFileFormatConstPtr format = _GetOriginalFormat(resolvedPath);

    if (format == nullptr)
    {
        return false;
    }

    return format->WriteToFile(layer, resolvedPath, comment, args);
}

RenderStudioFileFormat::RenderStudioFileFormat()
    : SdfFileFormat(
        RenderStudioFileFormatTokens->Id,
        RenderStudioFileFormatTokens->Version,
        RenderStudioFileFormatTokens->Target,
        RenderStudioFileFormatTokens->Id)
{
}

RenderStudioFileFormat::~RenderStudioFileFormat()
{
    if (mWebsocketClient != nullptr)
    {
        mWebsocketClient->Disconnect();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
