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
TF_DECLARE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, AR_API, RENDER_STUDIO_FILE_FORMAT_TOKENS);
#pragma warning(pop)

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioFileFormat);

class ArAsset;

class RenderStudioFileFormat : public SdfFileFormat
{
public:
    AR_API
    SdfAbstractDataRefPtr InitData(const FileFormatArguments& args) const override;

    AR_API
    virtual SdfLayer* _InstantiateNewLayer(
        const SdfFileFormatConstPtr& fileFormat,
        const std::string& identifier,
        const std::string& realPath,
        const ArAssetInfo& assetInfo,
        const FileFormatArguments& args) const override;

    AR_API
    virtual bool CanRead(const std::string& file) const override;

    AR_API
    virtual bool Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const override;

    AR_API
    virtual bool WriteToFile(const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment = std::string(),
        const FileFormatArguments& args = FileFormatArguments()) const override;

private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    RenderStudioFileFormat();
    virtual ~RenderStudioFileFormat();

    bool ProcessLiveUpdates();
    void Connect(const std::string& url);
    void Disconnect();
    RenderStudioDataPtr _GetRenderStudioData(SdfLayerHandle layer) const;
    void OnMessage(const std::string& message);

    friend class RenderStudioResolver;
    mutable RenderStudioLayerRegistry mLayerRegistry;
    std::shared_ptr<RenderStudio::Networking::WebsocketClient> mWebsocketClient;
};

PXR_NAMESPACE_CLOSE_SCOPE
