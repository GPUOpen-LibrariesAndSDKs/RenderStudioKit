//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "./renderStudioAiPrimAPI.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/typed.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType) { TfType::Define<RenderStudioAiPrimAPI, TfType::Bases<UsdAPISchemaBase>>(); }

TF_DEFINE_PRIVATE_TOKENS(_schemaTokens, (RenderStudioAiPrimAPI));

/* virtual */
RenderStudioAiPrimAPI::~RenderStudioAiPrimAPI() { }

/* static */
RenderStudioAiPrimAPI
RenderStudioAiPrimAPI::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage)
    {
        TF_CODING_ERROR("Invalid stage");
        return RenderStudioAiPrimAPI();
    }
    return RenderStudioAiPrimAPI(stage->GetPrimAtPath(path));
}

/* virtual */
UsdSchemaKind
RenderStudioAiPrimAPI::_GetSchemaKind() const
{
    return RenderStudioAiPrimAPI::schemaKind;
}

/* static */
bool
RenderStudioAiPrimAPI::CanApply(const UsdPrim& prim, std::string* whyNot)
{
    return prim.CanApplyAPI<RenderStudioAiPrimAPI>(whyNot);
}

/* static */
RenderStudioAiPrimAPI
RenderStudioAiPrimAPI::Apply(const UsdPrim& prim)
{
    if (prim.ApplyAPI<RenderStudioAiPrimAPI>())
    {
        return RenderStudioAiPrimAPI(prim);
    }
    return RenderStudioAiPrimAPI();
}

/* static */
const TfType&
RenderStudioAiPrimAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<RenderStudioAiPrimAPI>();
    return tfType;
}

/* static */
bool
RenderStudioAiPrimAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType&
RenderStudioAiPrimAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
RenderStudioAiPrimAPI::GetPromptAttr() const
{
    return GetPrim().GetAttribute(Tokens->paramsPrompt);
}

UsdAttribute
RenderStudioAiPrimAPI::CreatePromptAttr(VtValue const& defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        Tokens->paramsPrompt,
        SdfValueTypeNames->String,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

UsdAttribute
RenderStudioAiPrimAPI::GetDescriptionAttr() const
{
    return GetPrim().GetAttribute(Tokens->generatedDescription);
}

UsdAttribute
RenderStudioAiPrimAPI::CreateDescriptionAttr(VtValue const& defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
        Tokens->generatedDescription,
        SdfValueTypeNames->String,
        /* custom = */ false,
        SdfVariabilityVarying,
        defaultValue,
        writeSparsely);
}

namespace
{
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left, const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
} // namespace

/*static*/
const TfTokenVector&
RenderStudioAiPrimAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        Tokens->paramsPrompt,
        Tokens->generatedDescription,
    };
    static TfTokenVector allNames
        = _ConcatenateAttributeNames(UsdAPISchemaBase::GetSchemaAttributeNames(true), localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include <iostream>

#include "AiTools.h"

#include <Logger/Logger.h>

PXR_NAMESPACE_OPEN_SCOPE

void
RenderStudioAiPrimAPI::Generate()
{
    // Guard has attribute
    if (!GetPromptAttr().HasAuthoredValue())
    {
        LOG_WARNING << "RenderStudioAiPrimAPI::Generate() called without authored prompt";
        return;
    }

    std::string prompt;
    GetPromptAttr().Get<std::string>(&prompt);

    // Guard has attribute value
    if (prompt.empty())
    {
        LOG_WARNING << "RenderStudioAiPrimAPI::Generate() called with empty prompt";
        return;
    }

    // Clear previous generation
    for (const auto& prim : GetPrim().GetChildren())
    {
        GetPrim().GetStage()->RemovePrim(prim.GetPath());
    }

    GetDescriptionAttr().Clear();

    // Do new generation
    auto description = RenderStudio::AI::Tools::ProcessPrompt(prompt, GetPrim());

    if (description.has_value())
    {
        CreateDescriptionAttr().Set(description.value());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
