
#ifndef PXR_USD_AR_WEBUSD_ASSET_H
#define PXR_USD_AR_WEBUSD_ASSET_H

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/asset.h"

PXR_NAMESPACE_OPEN_SCOPE

class RenderStudioAsset : public ArAsset
{
public:
    AR_API
    explicit RenderStudioAsset(std::shared_ptr<const char> assetPtr, std::size_t assetSize);

    AR_API
    ~RenderStudioAsset();

    AR_API
    virtual size_t GetSize() const override;

    AR_API
    virtual std::shared_ptr<const char> GetBuffer() const override;

    AR_API
    virtual size_t Read(void* buffer, size_t count, size_t offset) const override;

    AR_API
    virtual std::pair<FILE*, size_t> GetFileUnsafe() const;

private:
    std::shared_ptr<const char> mData;
    std::size_t mDataSize;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_WEBUSD_ASSET_H
