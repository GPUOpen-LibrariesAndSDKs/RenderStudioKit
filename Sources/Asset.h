#pragma once

#pragma warning(push, 0)
#include <pxr/pxr.h>
#include <pxr/usd/ar/api.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/base/arch/fileSystem.h>
#include <pxr/base/arch/errno.h>

#include <string>
#include <filesystem>
#pragma warning(pop)

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
    static std::shared_ptr<LocalStorageAsset> Open(const std::string& uuid, const std::filesystem::path& location);

    AR_API
    explicit LocalStorageAsset(const std::string& uuid, const std::filesystem::path& location);
};

PXR_NAMESPACE_CLOSE_SCOPE
