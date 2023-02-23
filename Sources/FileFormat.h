#pragma once
 
#include <pxr/pxr.h>
#include <pxr/usd/usd/api.h>
#include <pxr/usd/sdf/textFileFormat.h>
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/staticTokens.h>

PXR_NAMESPACE_OPEN_SCOPE

#define RENDER_STUDIO_FILE_FORMAT_TOKENS \
    ((Id,      "studio"))             \
    ((Version, "1.0"))

TF_DECLARE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, USD_API, RENDER_STUDIO_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioFileFormat);

class ArAsset;

/// \class UsdUsdaFileFormat
///
/// File format used by textual USD files.
///
class RenderStudioFileFormat : public SdfTextFileFormat
{
public:
    SdfAbstractDataRefPtr InitData(
        const FileFormatArguments &args) const override;

    /*SDF_API
    virtual bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;*/

protected:
    /*SDF_API
        bool _ReadFromAsset(
            SdfLayer* layer,
            const std::string& resolvedPath,
            const std::shared_ptr<ArAsset>& asset,
            bool metadataOnly) const;*/

private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    RenderStudioFileFormat();
    virtual ~RenderStudioFileFormat();

    friend class UsdUsdFileFormat;
};

PXR_NAMESPACE_CLOSE_SCOPE
