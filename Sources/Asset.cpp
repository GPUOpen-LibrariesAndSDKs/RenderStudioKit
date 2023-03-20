#include "Asset.h"

#include <Networking/MaterialLibraryApi.h>
#include <Logger/Logger.h>

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
    auto package = RenderStudio::Networking::MaterialLibraryAPI::GetMaterialPackage(uuid);
    auto material = *std::min_element(
        package.results.begin(),
        package.results.end(),
        [](const auto& a, const auto& b) { return a.size_mb < b.size_mb; });

    auto mtlxRootLocation = RenderStudio::Networking::MaterialLibraryAPI::Download(material, location);
    mFileMapping = ArchOpenFile(mtlxRootLocation.string().c_str(), "rb");
}

GpuOpenAsset::~GpuOpenAsset()
{ 
    fclose(mFileMapping);
}

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

PXR_NAMESPACE_CLOSE_SCOPE
