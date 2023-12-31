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

#include "Serialization.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

using namespace boost::json;

// --- SdfPath ---

void
tag_invoke(const value_from_tag&, value& json, const SdfPath& v)
{
    json = v.GetString();
}

SdfPath
tag_invoke(const value_to_tag<SdfPath>&, const value& json)
{
    std::string path = value_to<std::string>(json);
    return path.empty() ? SdfPath {} : SdfPath { path };
}

// --- SdfAssetPath ---
void
tag_invoke(const value_from_tag&, value& json, const SdfAssetPath& v)
{
    object result;
    result["asset"] = v.GetAssetPath();
    result["resolved"] = v.GetResolvedPath();
    json = result;
}

SdfAssetPath
tag_invoke(const value_to_tag<SdfAssetPath>&, const value& json)
{
    object jsonObject = json.as_object();

    return SdfAssetPath { value_to<std::string>(jsonObject["asset"]), value_to<std::string>(jsonObject["resolved"]) };
}

// --- SdfReference ---
void
tag_invoke(const value_from_tag&, value& json, const SdfReference& v)
{
    std::string s = v.GetPrimPath().GetString();

    object result;
    result["asset"] = v.GetAssetPath();
    result["prim"] = value_from(v.GetPrimPath());
    result["offset"] = value_from(v.GetLayerOffset());
    json = result;
}

SdfReference
tag_invoke(const value_to_tag<SdfReference>&, const value& json)
{
    object jsonObject = json.as_object();

    return SdfReference { value_to<std::string>(jsonObject["asset"]),
                          value_to<SdfPath>(jsonObject["prim"]),
                          value_to<SdfLayerOffset>(jsonObject["offset"]) };
}

// --- SdfLayerOffset ---
void
tag_invoke(const value_from_tag&, value& json, const SdfLayerOffset& v)
{
    object result;
    result["offset"] = v.GetOffset();
    result["scale"] = v.GetScale();
    json = result;
}

SdfLayerOffset
tag_invoke(const value_to_tag<SdfLayerOffset>&, const value& json)
{
    object jsonObject = json.as_object();
    return SdfLayerOffset { value_to<double>(jsonObject["offset"]), value_to<double>(jsonObject["scale"]) };
}

// --- SdfLayerHandle ---

void
tag_invoke(const value_from_tag&, value& json, const SdfLayerHandle& v)
{
    json = v->GetIdentifier();
}

// --- TfToken ---

void
tag_invoke(const value_from_tag&, value& json, const TfToken& v)
{
    json = v.GetString();
}

TfToken
tag_invoke(const value_to_tag<TfToken>&, const value& json)
{
    return TfToken { value_to<std::string>(json) };
}

// --- VtValue ---

void
tag_invoke(const value_from_tag&, value& json, const VtValue& v)
{
    object result;

    std::string typeName = v.GetTypeName();
    typeName.erase(std::remove(typeName.begin(), typeName.end(), ' '), typeName.end());
    result["type"] = typeName;

    auto& data = result["data"];

    if (v.IsHolding<GfVec3d>())
    {
        data = value_from(v.Get<GfVec3d>());
    }
    else if (v.IsHolding<GfVec3f>())
    {
        data = value_from(v.Get<GfVec3f>());
    }
    else if (v.IsHolding<bool>())
    {
        data = value_from(v.Get<bool>());
    }
    else if (v.IsHolding<SdfSpecifier>())
    {
        data = value_from(v.Get<SdfSpecifier>());
    }
    else if (v.IsHolding<TfToken>())
    {
        data = value_from(v.Get<TfToken>());
    }
    else if (v.IsHolding<std::string>())
    {
        data = value_from(v.Get<std::string>());
    }
    else if (v.IsHolding<SdfVariability>())
    {
        data = value_from(v.Get<SdfVariability>());
    }
    else if (v.IsHolding<VtArray<int>>())
    {
        data = value_from(v.Get<VtArray<int>>());
    }
    else if (v.IsHolding<VtArray<float>>())
    {
        data = value_from(v.Get<VtArray<float>>());
    }
    else if (v.IsHolding<VtArray<TfToken>>())
    {
        data = value_from(v.Get<VtArray<TfToken>>());
    }
    else if (v.IsHolding<VtArray<GfVec2f>>())
    {
        data = value_from(v.Get<VtArray<GfVec2f>>());
    }
    else if (v.IsHolding<VtArray<GfVec3f>>())
    {
        data = value_from(v.Get<VtArray<GfVec3f>>());
    }
    else if (v.IsHolding<double>())
    {
        data = value_from(v.Get<double>());
    }
    else if (v.IsHolding<float>())
    {
        data = value_from(v.Get<float>());
    }
    else if (v.IsHolding<std::vector<TfToken>>())
    {
        data = value_from(v.Get<std::vector<TfToken>>());
    }
    else if (v.IsHolding<GfMatrix4d>())
    {
        data = value_from(v.Get<GfMatrix4d>());
    }
    else if (v.IsHolding<SdfListOp<SdfPath>>())
    {
        data = value_from(v.Get<SdfListOp<SdfPath>>());
    }
    else if (v.IsHolding<SdfListOp<SdfReference>>())
    {
        data = value_from(v.Get<SdfListOp<SdfReference>>());
    }
    else if (v.IsHolding<SdfListOp<TfToken>>())
    {
        data = value_from(v.Get<SdfListOp<TfToken>>());
    }
    else if (v.IsHolding<std::vector<SdfPath>>())
    {
        data = value_from(v.Get<std::vector<SdfPath>>());
    }
    else if (v.IsHolding<int>())
    {
        data = value_from(v.Get<int>());
    }
    else if (v.IsHolding<VtDictionary>())
    {
        data = value_from(v.Get<VtDictionary>());
    }
    else if (v.IsHolding<SdfAssetPath>())
    {
        data = value_from(v.Get<SdfAssetPath>());
    }
    else if (v.IsHolding<SdfValueBlock>())
    {
        data = value_from(v.Get<SdfValueBlock>());
    }
    else
    {
        throw std::runtime_error("Can't serialize type: " + v.GetTypeName());
    }

    json = result;
}

VtValue
tag_invoke(const value_to_tag<VtValue>&, const value& json)
{
    object jsonObject = json.as_object();
    std::string type = value_to<std::string>(jsonObject["type"]);
    value data = jsonObject["data"];

    if ("GfVec3d" == type)
    {
        return VtValue { value_to<GfVec3d>(data) };
    }
    if ("GfVec3f" == type)
    {
        return VtValue { value_to<GfVec3f>(data) };
    }
    else if ("bool" == type)
    {
        return VtValue { value_to<bool>(data) };
    }
    else if ("SdfSpecifier" == type)
    {
        return VtValue { value_to<SdfSpecifier>(data) };
    }
    else if ("TfToken" == type)
    {
        return VtValue { value_to<TfToken>(data) };
    }
    else if ("string" == type)
    {
        return VtValue { value_to<std::string>(data) };
    }
    else if ("SdfVariability" == type)
    {
        return VtValue { value_to<SdfVariability>(data) };
    }
    else if ("VtArray<int>" == type)
    {
        return VtValue { value_to<VtArray<int>>(data) };
    }
    else if ("VtArray<TfToken>" == type)
    {
        return VtValue { value_to<VtArray<TfToken>>(data) };
    }
    else if ("VtArray<GfVec2f>" == type)
    {
        return VtValue { value_to<VtArray<GfVec2f>>(data) };
    }
    else if ("VtArray<GfVec3f>" == type)
    {
        return VtValue { value_to<VtArray<GfVec3f>>(data) };
    }
    else if ("double" == type)
    {
        return VtValue { value_to<double>(data) };
    }
    else if ("vector<TfToken,allocator<TfToken>>" == type)
    {
        return VtValue { value_to<std::vector<TfToken>>(data) };
    }
    else if ("vector<SdfPath,allocator<SdfPath>>" == type)
    {
        return VtValue { value_to<std::vector<SdfPath>>(data) };
    }
    else if ("GfMatrix4d" == type)
    {
        return VtValue { value_to<GfMatrix4d>(data) };
    }
    else if ("float" == type)
    {
        return VtValue { value_to<float>(data) };
    }
    else if ("int" == type)
    {
        return VtValue { value_to<int>(data) };
    }
    else if ("VtArray<float>" == type)
    {
        return VtValue { value_to<VtArray<float>>(data) };
    }
    else if ("VtDictionary" == type)
    {
        return VtValue { value_to<VtDictionary>(data) };
    }
    else if ("SdfAssetPath" == type)
    {
        return VtValue { value_to<SdfAssetPath>(data) };
    }
    else if ("SdfListOp<SdfPath>" == type)
    {
        return VtValue { value_to<SdfListOp<SdfPath>>(data) };
    }
    else if ("SdfListOp<SdfReference>" == type)
    {
        return VtValue { value_to<SdfListOp<SdfReference>>(data) };
    }
    else if ("SdfListOp<TfToken>" == type)
    {
        return VtValue { value_to<SdfListOp<TfToken>>(data) };
    }
    else if ("SdfValueBlock" == type)
    {
        return VtValue { value_to<SdfValueBlock>(data) };
    }
    else
    {
        throw std::runtime_error("Can't parse type: " + type);
    }
}

// --- GfVec3d ---

void
tag_invoke(const value_from_tag&, value& json, const GfVec3d& v)
{
    array result;

    for (std::size_t i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec3d
tag_invoke(const value_to_tag<GfVec3d>&, const value& json)
{
    array jsonArray = json.as_array();
    return GfVec3d { value_to<GfVec3d::ScalarType>(jsonArray.at(0)),
                     value_to<GfVec3d::ScalarType>(jsonArray.at(1)),
                     value_to<GfVec3d::ScalarType>(jsonArray.at(2)) };
}

// --- GfVec2f ---

void
tag_invoke(const value_from_tag&, value& json, const GfVec2f& v)
{
    array result;

    for (std::size_t i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec2f
tag_invoke(const value_to_tag<GfVec2f>&, const value& json)
{
    array jsonArray = json.as_array();

    return GfVec2f { value_to<GfVec2f::ScalarType>(jsonArray.at(0)), value_to<GfVec2f::ScalarType>(jsonArray.at(1)) };
}

// --- GfVec3f ---

void
tag_invoke(const value_from_tag&, value& json, const GfVec3f& v)
{
    array result;

    for (std::size_t i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec3f
tag_invoke(const value_to_tag<GfVec3f>&, const value& json)
{
    array jsonArray = json.as_array();
    return GfVec3f { value_to<GfVec3f::ScalarType>(jsonArray.at(0)),
                     value_to<GfVec3f::ScalarType>(jsonArray.at(1)),
                     value_to<GfVec3f::ScalarType>(jsonArray.at(2)) };
}

// --- SdfSpecifier ---

void
tag_invoke(const value_from_tag&, value& json, const SdfSpecifier& v)
{
    json = static_cast<std::int32_t>(v);
}

SdfSpecifier
tag_invoke(const value_to_tag<SdfSpecifier>&, const value& json)
{
    return SdfSpecifier(value_to<std::int32_t>(json));
}

// --- SdfSpecType ---

void
tag_invoke(const value_from_tag&, value& json, const SdfSpecType& v)
{
    json = static_cast<std::int32_t>(v);
}

SdfSpecType
tag_invoke(const value_to_tag<SdfSpecType>&, const value& json)
{
    return SdfSpecType(value_to<std::int32_t>(json));
}

// --- SdfVariability ---

void
tag_invoke(const value_from_tag&, value& json, const SdfVariability& v)
{
    json = v;
}

SdfVariability
tag_invoke(const value_to_tag<SdfVariability>&, const value& json)
{
    return SdfVariability(value_to<std::int32_t>(json));
}

// --- GfMatrix4d ---

void
tag_invoke(const value_from_tag&, value& json, const GfMatrix4d& v)
{
    array result;

    for (std::size_t i = 0; i < GfMatrix4d::numRows * GfMatrix4d::numColumns; i++)
    {
        result.push_back(*v[static_cast<std::int32_t>(i)]);
    }

    json = result;
}

GfMatrix4d
tag_invoke(const value_to_tag<GfMatrix4d>&, const value& json)
{
    GfMatrix4d result {};
    array jsonArray = json.as_array();

    for (std::size_t i = 0; i < jsonArray.size(); i++)
    {
        *result[static_cast<std::int32_t>(i)]
            = value_to<GfMatrix4d::ScalarType>(jsonArray.at(static_cast<std::int32_t>(i)));
    }

    return result;
}

// --- VtDictionary ---

void
tag_invoke(const value_from_tag&, value& json, const VtDictionary& v)
{
    object result;

    for (const auto& item : v)
    {
        result[item.first] = value_from(item.second);
    }

    json = result;
}

VtDictionary
tag_invoke(const value_to_tag<VtDictionary>&, const value& json)
{
    VtDictionary result {};
    object jsonObject = json.as_object();

    for (const auto& item : jsonObject)
    {
#if BOOST_VERSION >= 108000
        result[item.key()] = value_to<VtValue>(item.value());
#else
        result[item.key().to_string()] = value_to<VtValue>(item.value());
#endif
    }

    return result;
}

void
tag_invoke(const value_from_tag&, value& json, const SdfValueBlock& v)
{
    (void)v;
    json = {};
}

SdfValueBlock
tag_invoke(const value_to_tag<SdfValueBlock>&, const value& json)
{
    (void)json;
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
