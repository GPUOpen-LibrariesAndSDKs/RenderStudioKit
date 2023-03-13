#include "Asset.h"

#include <stdexcept>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

RenderStudioAsset::RenderStudioAsset(std::shared_ptr<const char> assetPtr, std::size_t assetSize)
    : mData(assetPtr)
    , mDataSize(assetSize)
{
}

RenderStudioAsset::~RenderStudioAsset() { }

size_t
RenderStudioAsset::GetSize() const
{
    return mDataSize;
}

std::shared_ptr<const char>
RenderStudioAsset::GetBuffer() const
{
    return mData;
}

size_t
RenderStudioAsset::Read(void* buffer, size_t count, size_t offset) const
{
    std::memcpy(buffer, mData.get() + offset, count);
    return count;
}

std::pair<FILE*, size_t>
RenderStudioAsset::GetFileUnsafe() const
{
    throw std::runtime_error("GetFileUnsafe() called on RenderStudioAsset");
}

PXR_NAMESPACE_CLOSE_SCOPE
