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

static SdfFileFormatConstPtr
_GetFileFormat(const TfToken& formatId)
{
    const SdfFileFormatConstPtr fileFormat = SdfFileFormat::FindById(formatId);
    TF_VERIFY(fileFormat);
    return fileFormat;
}

static const UsdUsdcFileFormatConstPtr&
_GetUsdcFileFormat()
{
    static const auto usdcFormat
        = TfDynamic_cast<UsdUsdcFileFormatConstPtr>(_GetFileFormat(UsdUsdcFileFormatTokens->Id));
    return usdcFormat;
}

static const UsdUsdaFileFormatConstPtr&
_GetUsdaFileFormat()
{
    static const auto usdaFormat
        = TfDynamic_cast<UsdUsdaFileFormatConstPtr>(_GetFileFormat(UsdUsdaFileFormatTokens->Id));
    return usdaFormat;
}

static const SdfFileFormatConstPtr
_GetOriginalFormat(const std::string& path)
{
    std::size_t separator = path.find_first_of('?');
    std::string raw = path.substr(0, separator);
    std::string extension = std::filesystem::path(raw).extension().string();

    // Hardcoded cases if no extension provided
    // TODO: Force usage of extensions
    if (extension.empty())
    {
        if (path.find("gpuopen:/") != std::string::npos)
        {
            extension = ".mtlx";
        }

        if (path.find("storage:/") != std::string::npos)
        {
            extension = ".usda";
        }
    }

    if (extension.find(".usd") != std::string::npos)
    {
        const auto& usdcFileFormat = _GetUsdcFileFormat();
        const auto& usdaFileFormat = _GetUsdaFileFormat();

        if (usdcFileFormat->CanRead(path))
        {
            return usdcFileFormat;
        }

        if (usdaFileFormat->CanRead(path))
        {
            return usdaFileFormat;
        }
    }

    // Fallback to search by extension
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

RenderStudioDataPtr
RenderStudioFileFormat::_GetRenderStudioData(const SdfLayer& layer) const
{
    SdfAbstractDataConstPtr abstract = SdfFileFormat::_GetLayerData(layer);
    RenderStudioDataConstPtr casted = TfDynamic_cast<RenderStudioDataConstPtr>(abstract);

    if (casted == nullptr)
    {
        throw std::runtime_error("Layer has no RenderStudio data");
    }

    return TfConst_cast<RenderStudioDataPtr>(casted);
}

bool
RenderStudioFileFormat::ProcessLiveUpdates()
{
    bool updated = false;

    // Fetch all incoming events and release lock so newer events could be accumulated
    std::unique_lock<std::mutex> lock(mEventMutex);
    auto deltas = std::move(mAccumulatedDeltas);
    auto reloads = std::move(mRequestedReloads);
    auto acknowledges = std::move(mAccumulatedAcknowledges);
    lock.unlock();

    // Process reloads. It's safe to do it from beginning, since we already discarded all the deltas before reloading
    mReloadInProgress = true;
    for (const std::string& id : reloads)
    {
        SdfLayerHandle layer = mLayerRegistry.GetByIdentifier(id);
        if (layer == nullptr)
        {
            LOG_WARNING << "Tried to reload expired layer: " << id;
            continue;
        }
        layer->Reload();
        RenderStudioNotice::LayerReloaded(id).Send();
        updated = true;
    }
    mReloadInProgress = false;

    // Process deltas
    mLayerRegistry.ForEachLayer(
        [this, &updated, &deltas, &reloads, &acknowledges](SdfLayerHandle layer)
        {
            RenderStudioDataPtr data = _GetRenderStudioData(layer);

            if (data == nullptr)
            {
                LOG_ERROR << "Data was null for layer: " << layer->GetIdentifier();
                return;
            }

            // Send local deltas
            auto local = data->FetchLocalDeltas();
            if (!local.empty())
            {
                try
                {
                    // Convert internal format to API update
                    RenderStudio::API::DeltaEvent body;
                    body.layer = layer->GetIdentifier();
                    body.user = RenderStudioResolver::GetCurrentUserId();
                    body.sequence = std::nullopt;

                    for (const auto& [key, value] : local)
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

            // Accumulate remote acknowledges inside data
            if (auto it = acknowledges.find(layer->GetIdentifier()); it != acknowledges.end())
            {
                for (const RenderStudio::API::AcknowledgeEvent& acknowledge : it->second)
                {
                    // Convert API update to internal format
                    RenderStudioData::_HashTable updates;
                    for (const auto& path : acknowledge.paths)
                    {
                        updates[path] = RenderStudioData::_SpecData {};
                    }
                    data->AccumulateRemoteUpdate(updates, acknowledge.sequence);
                }
            }

            // Accumulate remote deltas inside data
            if (auto it = deltas.find(layer->GetIdentifier()); it != deltas.end())
            {
                for (const RenderStudio::API::DeltaEvent& delta : it->second)
                {
                    // Convert API update to internal format
                    RenderStudioData::_HashTable updates;
                    for (const auto& [key, value] : delta.updates)
                    {
                        RenderStudioData::_SpecData spec;
                        spec.specType = value.specType;
                        spec.fields = value.fields;
                        updates[key] = spec;
                    }
                    data->AccumulateRemoteUpdate(updates, delta.sequence.value());
                }
            }

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
    mWebsocketClient = RenderStudio::Networking::WebsocketClient::Create(*this);

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

    // Here's first time layer read
    bool firstTimeLoading = mLayerRegistry.GetByIdentifier(layer->GetIdentifier()) == nullptr;
    if (firstTimeLoading)
    {
        mLayerRegistry.AddLayer(SdfLayerHandle { layer });
    }

    // Notify other clients about reloading if we initiated it
    if (!mReloadInProgress && mWebsocketClient != nullptr && !firstTimeLoading)
    {
        // Here's consider layer reloading
        RenderStudio::API::ReloadEvent body;
        body.layer = layer->GetIdentifier();
        body.user = RenderStudioResolver::GetCurrentUserId();
        body.sequence = std::nullopt;
        RenderStudio::API::Event event { "Reload::Event", body };
        mWebsocketClient->Send(boost::json::serialize(boost::json::value_from(event)));
    }

    _GetRenderStudioData(SdfLayerHandle { layer })->SetOriginalFormat(format);
    _GetRenderStudioData(SdfLayerHandle { layer })->OnLoaded();

    return result;
}

bool
RenderStudioFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    // TODO: Add support for custom args
    SdfFileFormatConstPtr format = _GetRenderStudioData(layer)->GetOriginalFormat();

    if (format == nullptr)
    {
        return false;
    }

    std::string resolvedPath = RenderStudioResolver::ResolveImpl(filePath);
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

void
RenderStudioFileFormat::ProcessDeltaEvent(const RenderStudio::API::DeltaEvent& v)
{
    if (!v.sequence.has_value())
    {
        LOG_WARNING << "Got update without sequence number";
        return;
    }

    std::lock_guard<std::mutex> lock(mEventMutex);
    mAccumulatedDeltas[v.layer].push_back(v);
}

void
RenderStudioFileFormat::ProcessHistoryEvent(const RenderStudio::API::HistoryEvent& v)
{
    // Send history notice
    (void)v;
    RenderStudioNotice::LiveHistoryStatus("History", "RenderStudio::Internal");
}

void
RenderStudioFileFormat::ProcessAcknowledgeEvent(const RenderStudio::API::AcknowledgeEvent& v)
{
    std::lock_guard<std::mutex> lock(mEventMutex);
    mAccumulatedAcknowledges[v.layer].push_back(v);
}

void
RenderStudioFileFormat::ProcessReloadEvent(const RenderStudio::API::ReloadEvent& v)
{
    if (!v.sequence.has_value())
    {
        LOG_WARNING << "Got update without sequence number";
        return;
    }

    std::lock_guard<std::mutex> lock(mEventMutex);

    // Clear all deltas that happen before reload, we not going to apply them
    if (auto it = mAccumulatedDeltas.find(v.layer); it != mAccumulatedDeltas.end())
    {
        mAccumulatedDeltas.erase(it);
    }

    // Clear all acknowledges that happen before reload, data would be re-created so not use them
    if (auto it = mAccumulatedAcknowledges.find(v.layer); it != mAccumulatedAcknowledges.end())
    {
        mAccumulatedAcknowledges.erase(it);
    }

    // Store information that reloading is required
    mRequestedReloads.push_back(v.layer);
}

void
RenderStudioFileFormat::OnConnected()
{
    LOG_INFO << "Connected RenderStudioKit with remote Live server";
    RenderStudioNotice::LiveConnectionChanged(true).Send();
}

void
RenderStudioFileFormat::OnDisconnected()
{
    LOG_INFO << "Disconnected RenderStudioKit from remote Live server";
    RenderStudioNotice::LiveConnectionChanged(false).Send();
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
        Overload { [this](const RenderStudio::API::DeltaEvent& v) { ProcessDeltaEvent(v); },
                   [this](const RenderStudio::API::HistoryEvent& v) { ProcessHistoryEvent(v); },
                   [this](const RenderStudio::API::AcknowledgeEvent& v) { ProcessAcknowledgeEvent(v); },
                   [this](const RenderStudio::API::ReloadEvent& v) { ProcessReloadEvent(v); } },
        event.value().body);
}

PXR_NAMESPACE_CLOSE_SCOPE
