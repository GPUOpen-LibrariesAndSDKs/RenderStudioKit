#include "Asset.h"

#include <Resolver.h>

#include <Logger/Logger.h>
#include <Networking/LocalStorageApi.h>
#include <Networking/MaterialLibraryApi.h>

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

        void operator()(const char* b) { _mapping.reset(); }

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
LocalStorageAsset::Open(const std::string& uuid, const std::filesystem::path& location)
{
    return std::make_shared<LocalStorageAsset>(uuid, location);
}

LocalStorageAsset::LocalStorageAsset(const std::string& uuid, const std::filesystem::path& location)
{
    mUuid = uuid;

    // Download light
    auto package
        = RenderStudio::Networking::LocalStorageAPI::GetLightPackage(uuid, RenderStudioResolver::GetLocalStorageUrl());

    auto usdaLocation = RenderStudio::Networking::LocalStorageAPI::Download(
        package, location, RenderStudioResolver::GetLocalStorageUrl());
    mFileMapping = ArchOpenFile(usdaLocation.string().c_str(), "rb");
}

PXR_NAMESPACE_CLOSE_SCOPE
