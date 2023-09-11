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
#include <filesystem>

#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/resolver.h>
#pragma warning(pop)

#include "FileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioResolver : public ArDefaultResolver
{
public:
    AR_API
    RenderStudioResolver();

    AR_API
    virtual ~RenderStudioResolver();

    struct LiveModeInfo
    {
        std::string liveUrl;
        std::string storageUrl;
        std::string channelId;
        std::string userId;
    };

    AR_API
    static void StartLiveMode(const LiveModeInfo& info);

    AR_API
    static bool ProcessLiveUpdates();

    AR_API
    static void StopLiveMode();

    AR_API
    static std::string GetLocalStorageUrl();

    AR_API
    static std::string GetCurrentUserId();

    AR_API
    static bool IsRenderStudioPath(const std::string& path);

    AR_API
    static bool IsUnresovableToRenderStudioPath(const std::string& path);

    AR_API
    static std::string Unresolve(const std::string& path);

    AR_API
    virtual std::string _GetExtension(const std::string& path) const override;

    AR_API
    virtual ArResolvedPath _Resolve(const std::string& path) const override;

protected:
    AR_API
    std::string _CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const override;

    AR_API
    virtual std::shared_ptr<ArAsset> _OpenAsset(const ArResolvedPath& resolvedPath) const override;

private:
    static std::filesystem::path GetDocumentsDirectory();
    static std::filesystem::path GetRootPath();

    static inline LiveModeInfo sLiveModeInfo;
    static inline RenderStudioFileFormatPtr sFileFormat;
};

PXR_NAMESPACE_CLOSE_SCOPE
