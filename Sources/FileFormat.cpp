#include "FileFormat.h"
#include "Data.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/usdaFileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/asset.h>

#include <Logger.hpp>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(RenderStudioFileFormatTokens, RENDER_STUDIO_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(RenderStudioFileFormat, SdfTextFileFormat);
}

SdfAbstractDataRefPtr
RenderStudioFileFormat::InitData(const FileFormatArguments &args) const
{
    Logger::Error("RenderStudioFileFormat::InitData");
    
    RenderStudioData* metadata = new RenderStudioData;
    metadata->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(metadata);
}

RenderStudioFileFormat::RenderStudioFileFormat()
    : SdfTextFileFormat(RenderStudioFileFormatTokens->Id,
                        RenderStudioFileFormatTokens->Version,
                        UsdUsdFileFormatTokens->Target)
{
    // Do Nothing.
}

RenderStudioFileFormat::~RenderStudioFileFormat()
{
    // Do Nothing.
}

PXR_NAMESPACE_CLOSE_SCOPE

