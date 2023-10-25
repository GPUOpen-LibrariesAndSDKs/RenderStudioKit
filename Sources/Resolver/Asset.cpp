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

#include "Asset.h"

#include "Resolver.h"

#include <Logger/Logger.h>
#include <Networking/MaterialLibraryApi.h>
#include <Notice/Notice.h>

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<GpuOpenAsset>
GpuOpenAsset::Open(const std::string& uuid, const std::filesystem::path& location)
{
    return std::make_shared<GpuOpenAsset>(uuid, location);
}

GpuOpenAsset::GpuOpenAsset(const std::string& uuid, const std::filesystem::path& location)
    : mUuid(uuid)
{
    // Download material
    try
    {
        auto package = RenderStudio::Networking::MaterialLibraryAPI::GetMaterialPackage(uuid);
        auto material = *std::min_element(
            package.results.begin(),
            package.results.end(),
            [](const auto& a, const auto& b) { return a.size_mb < b.size_mb; });

        auto mtlxRootLocation = RenderStudio::Networking::MaterialLibraryAPI::Download(material, location);
        mFileMapping = ArchOpenFile(mtlxRootLocation.string().c_str(), "rb");
    }
    catch (const std::exception& ex)
    {
        LOG_FATAL << ex.what();
    }
}

GpuOpenAsset::~GpuOpenAsset() { fclose(mFileMapping); }

std::size_t
GpuOpenAsset::GetSize() const
{
    return ArchGetFileLength(mFileMapping);
}

std::shared_ptr<const char>
GpuOpenAsset::GetBuffer() const
{
    ArchConstFileMapping mapping = ArchMapFileReadOnly(mFileMapping);
    if (!mapping)
    {
        return nullptr;
    }

    struct _Deleter
    {
        explicit _Deleter(ArchConstFileMapping&& mapping)
            : _mapping(new ArchConstFileMapping(std::move(mapping)))
        {
        }

        void operator()(const char* b)
        {
            (void)b;
            _mapping.reset();
        }

        std::shared_ptr<ArchConstFileMapping> _mapping;
    };

    const char* buffer = mapping.get();
    return std::shared_ptr<const char>(buffer, _Deleter(std::move(mapping)));
}

std::size_t
GpuOpenAsset::Read(void* buffer, std::size_t count, std::size_t offset) const
{
    std::int64_t numRead = ArchPRead(mFileMapping, buffer, count, offset);
    if (numRead == -1)
    {
        LOG_ERROR << "Error occurred reading file: " << ArchStrerror();
        return 0;
    }
    return numRead;
}

std::pair<FILE*, std::size_t>
GpuOpenAsset::GetFileUnsafe() const
{
    return std::make_pair(mFileMapping, 0);
}

std::shared_ptr<LocalStorageAsset>
LocalStorageAsset::Open(const std::string& name, const std::filesystem::path& location)
{
    return std::make_shared<LocalStorageAsset>(name, location);
}

LocalStorageAsset::LocalStorageAsset(const std::string& name, const std::filesystem::path& location)
{
    // Aggressive optimization here, ignoring mUuid and applying first .usda file under light directory assuming that
    // it's valid
    mUuid = "empty";

    std::filesystem::path usdaLocation;

    if (!std::filesystem::exists(location))
    {
        // Download light
        auto package = RenderStudio::Networking::LocalStorageAPI::GetLightPackage(
            name, RenderStudioResolver::GetLocalStorageUrl());

        usdaLocation = RenderStudio::Networking::LocalStorageAPI::Download(
            package, location, RenderStudioResolver::GetLocalStorageUrl());
    }
    else
    {
        for (auto const& entry : std::filesystem::recursive_directory_iterator(location))
        {
            if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".usda")
            {
                usdaLocation = entry.path();
                break;
            }
        }

        if (usdaLocation.empty())
        {
            throw std::runtime_error("Storage was corrupted, can't find " + location.string());
        }
    }

    mFileMapping = ArchOpenFile(usdaLocation.string().c_str(), "rb");
}

PXR_NAMESPACE_CLOSE_SCOPE
