#include "Asset.h"

#include <stdexcept>
#include <pxr/pxr.h>
#define FMT_HEADER_ONLY
#include <fmt/format-inl.h>
#include <Logger.hpp>

PXR_NAMESPACE_OPEN_SCOPE

WebUsdAsset::WebUsdAsset(std::shared_ptr<const char> assetPtr, std::size_t assetSize)
    : mData(assetPtr) 
    , mDataSize(assetSize)
{

}

WebUsdAsset::~WebUsdAsset()
{

}

size_t
WebUsdAsset::GetSize() const
{
    return mDataSize;
}

std::shared_ptr<const char>
WebUsdAsset::GetBuffer() const
{
    return mData;
}
    
size_t
WebUsdAsset::Read(void* buffer, size_t count, size_t offset) const
{
    std::memcpy(buffer, mData.get() + offset, count);
    return count;
}

std::pair<FILE*, size_t>
WebUsdAsset::GetFileUnsafe() const
{
    throw std::runtime_error("GetFileUnsafe() called on WebUsdAsset");
}

PXR_NAMESPACE_CLOSE_SCOPE
