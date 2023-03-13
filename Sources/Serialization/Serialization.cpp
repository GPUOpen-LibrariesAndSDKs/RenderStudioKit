#include "Serialization.h"

PXR_NAMESPACE_OPEN_SCOPE

using namespace boost::json;

// --- SdfPath ---

void tag_invoke(const value_from_tag&, value& json, const SdfPath& v)
{
    json = v.GetString();
}

SdfPath tag_invoke(const value_to_tag<SdfPath>&, const value& json)
{
    return SdfPath{ value_to<std::string>(json) };
}

// --- SdfAssetPath ---
void tag_invoke(const value_from_tag&, value& json, const SdfAssetPath& v)
{
    object result;
    result["asset"] = v.GetAssetPath();
    result["resolved"] = v.GetResolvedPath();
    json = result;
}

SdfAssetPath tag_invoke(const value_to_tag<SdfAssetPath>&, const value& json)
{
    object jsonObject = json.as_object();

    return SdfAssetPath{ 
        value_to<std::string>(jsonObject["asset"]),
        value_to<std::string>(jsonObject["resolved"])
    };
}

// --- SdfLayerHandle ---

void tag_invoke(const value_from_tag&, value& json, const SdfLayerHandle& v)
{
    json = v->GetIdentifier();
}

// --- TfToken ---

void tag_invoke(const value_from_tag&, value& json, const TfToken& v)
{
    json = v.GetString();
}

TfToken tag_invoke(const value_to_tag<TfToken>&, const value& json)
{
    return TfToken{ value_to<std::string>(json) };
}

// --- VtValue ---

void tag_invoke(const value_from_tag&, value& json, const VtValue& v)
{
    object result;
    result["type"] = v.GetTypeName();
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
    else
    {
        throw std::runtime_error("Can't serialize type: " + v.GetTypeName());
    }

    json = result;
}

VtValue tag_invoke(const value_to_tag<VtValue>&, const value& json)
{
    object jsonObject = json.as_object();
    std::string type = value_to<std::string>(jsonObject["type"]);
    value data = jsonObject["data"];

    if ("GfVec3d" == type)
    {
        return VtValue{ value_to<GfVec3d>(data)};
    }
    if ("GfVec3f" == type)
    {
        return VtValue{ value_to<GfVec3f>(data) };
    }
    else if ("bool" == type)
    {
        return VtValue{ value_to<bool>(data) };
    }
    else if ("SdfSpecifier" == type)
    {
        return VtValue{ value_to<SdfSpecifier>(data) };
    }
    else if ("TfToken" == type)
    {
        return VtValue{ value_to<TfToken>(data) };
    }
    else if ("string" == type)
    {
        return VtValue{ value_to<std::string>(data) };
    }
    else if ("SdfVariability" == type)
    {
        return VtValue{ value_to<SdfVariability>(data) };
    }
    else if ("VtArray<int>" == type)
    {
        return VtValue{ value_to<VtArray<int>>(data) };
    }
    else if ("VtArray<TfToken>" == type)
    {
        return VtValue{ value_to<VtArray<TfToken>>(data) };
    }
    else if ("VtArray<GfVec2f>" == type)
    {
        return VtValue{ value_to<VtArray<GfVec2f>>(data) };
    }
    else if ("VtArray<GfVec3f>" == type)
    {
        return VtValue{ value_to<VtArray<GfVec3f>>(data) };
    }
    else if ("double" == type)
    {
        return VtValue{ value_to<double>(data) };
    }
    else if ("vector<TfToken,allocator<TfToken> >" == type)
    {
        return VtValue{ value_to<std::vector<TfToken>>(data) };
    }
    else if ("vector<SdfPath,allocator<SdfPath> >" == type)
    {
        return VtValue{ value_to<std::vector<SdfPath>>(data) };
    }
    else if ("GfMatrix4d" == type)
    {
        return VtValue{ value_to<GfMatrix4d>(data) };
    }
    else if ("float" == type)
    {
        return VtValue{ value_to<float>(data) };
    }
    else if ("int" == type)
    {
        return VtValue{ value_to<int>(data) };
    }
    else if ("VtArray<float>" == type)
    {
        return VtValue{ value_to<VtArray<float>>(data) };
    }
    else if ("VtDictionary" == type)
    {
        return VtValue{ value_to<VtDictionary>(data) };
    }
    else if ("SdfAssetPath" == type)
    {
        return VtValue{ value_to<SdfAssetPath>(data) };
    }
    else
    {
        throw std::runtime_error("Can't parse type: " + type);
    }
}

// --- GfVec3d ---

void tag_invoke(const value_from_tag&, value& json, const GfVec3d& v)
{
    array result;

    for (auto i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec3d tag_invoke(const value_to_tag<GfVec3d>&, const value& json)
{
    array jsonArray = json.as_array();
    return GfVec3d{
        value_to<GfVec3d::ScalarType>(jsonArray.at(0)),
        value_to<GfVec3d::ScalarType>(jsonArray.at(1)),
        value_to<GfVec3d::ScalarType>(jsonArray.at(2))
    };
}

// --- GfVec2f ---

void tag_invoke(const value_from_tag&, value& json, const GfVec2f& v)
{
    array result;

    for (auto i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec2f tag_invoke(const value_to_tag<GfVec2f>&, const value& json)
{
    array jsonArray = json.as_array();

    return GfVec2f{
        value_to<GfVec2f::ScalarType>(jsonArray.at(0)),
        value_to<GfVec2f::ScalarType>(jsonArray.at(1))
    };
}

// --- GfVec3f ---

void tag_invoke(const value_from_tag&, value& json, const GfVec3f& v)
{
    array result;

    for (auto i = 0; i < v.dimension; i++)
    {
        result.push_back(v[i]);
    }

    json = result;
}

GfVec3f tag_invoke(const value_to_tag<GfVec3f>&, const value& json)
{
    array jsonArray = json.as_array();
    return GfVec3f{
        value_to<GfVec3f::ScalarType>(jsonArray.at(0)),
        value_to<GfVec3f::ScalarType>(jsonArray.at(1)),
        value_to<GfVec3f::ScalarType>(jsonArray.at(2))
    };
}

// --- SdfSpecifier ---

void tag_invoke(const value_from_tag&, value& json, const SdfSpecifier& v)
{
    json = static_cast<std::int32_t>(v);
}

SdfSpecifier tag_invoke(const value_to_tag<SdfSpecifier>&, const value& json)
{
    return SdfSpecifier(value_to<std::int32_t>(json));
}

// --- SdfSpecType ---

void tag_invoke(const value_from_tag&, value& json, const SdfSpecType& v)
{
    json = static_cast<std::int32_t>(v);
}

SdfSpecType tag_invoke(const value_to_tag<SdfSpecType>&, const value& json)
{
    return SdfSpecType(value_to<std::int32_t>(json));
}

// --- SdfVariability ---

void tag_invoke(const value_from_tag&, value& json, const SdfVariability& v)
{
    json = v;
}

SdfVariability tag_invoke(const value_to_tag<SdfVariability>&, const value& json)
{
    return SdfVariability(value_to<std::int32_t>(json));
}

// --- GfMatrix4d ---

void tag_invoke(const value_from_tag&, value& json, const GfMatrix4d& v)
{
    array result;

    for (auto i = 0; i < GfMatrix4d::numRows * GfMatrix4d::numColumns; i++)
    {
        result.push_back(*v[i]);
    }

    json = result;
}

GfMatrix4d tag_invoke(const value_to_tag<GfMatrix4d>&, const value& json)
{
    GfMatrix4d result;
    array jsonArray = json.as_array();

    for (auto i = 0; i < jsonArray.size(); i++)
    {
        *result[i] = value_to<GfMatrix4d::ScalarType>(jsonArray.at(i));
    }

    return result;
}


PXR_NAMESPACE_CLOSE_SCOPE

