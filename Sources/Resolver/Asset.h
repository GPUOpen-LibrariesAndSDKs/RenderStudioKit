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
#include <string>

#include <pxr/base/arch/errno.h>
#include <pxr/base/arch/fileSystem.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/asset.h>
#pragma warning(pop)

#include <Networking/LocalStorageApi.h>

PXR_NAMESPACE_OPEN_SCOPE

class GpuOpenAsset : public ArAsset
{
public:
    AR_API
    static std::shared_ptr<GpuOpenAsset> Open(const std::string& uuid, const std::filesystem::path& location);

    AR_API
    explicit GpuOpenAsset(const std::string& uuid, const std::filesystem::path& location);

    AR_API
    explicit GpuOpenAsset() = default;

    AR_API
    virtual ~GpuOpenAsset();

    AR_API
    virtual std::size_t GetSize() const override;

    AR_API
    virtual std::shared_ptr<const char> GetBuffer() const override;

    AR_API
    virtual std::size_t Read(void* buffer, std::size_t count, std::size_t offset) const override;

    AR_API
    virtual std::pair<FILE*, std::size_t> GetFileUnsafe() const override;

protected:
    std::string mUuid;
    FILE* mFileMapping;
};

class LocalStorageAsset : public GpuOpenAsset
{
public:
    AR_API
    static std::shared_ptr<LocalStorageAsset> Open(const std::string& name, const std::filesystem::path& location);

    AR_API
    explicit LocalStorageAsset(const std::string& name, const std::filesystem::path& location);
};

PXR_NAMESPACE_CLOSE_SCOPE
