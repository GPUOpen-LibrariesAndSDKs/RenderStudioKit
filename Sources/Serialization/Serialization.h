#pragma once

#include <pxr/pxr.h>
#include <boost/json.hpp>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/layer.h>

PXR_NAMESPACE_OPEN_SCOPE

using namespace boost::json;

// --- SdfPath ---
void tag_invoke(const value_from_tag&, value& json, const SdfPath& v);
SdfPath tag_invoke(const value_to_tag<SdfPath>&, const value& json);

// --- SdfAssetPath ---
void tag_invoke(const value_from_tag&, value& json, const SdfAssetPath& v);
SdfAssetPath tag_invoke(const value_to_tag<SdfAssetPath>&, const value& json);

// --- SdfLayerHandle ---
void tag_invoke(const value_from_tag&, value& json, const SdfLayerHandle& v);

// --- TfToken ---
void tag_invoke(const value_from_tag&, value& json, const TfToken& v);
TfToken tag_invoke(const value_to_tag<TfToken>&, const value& json);

// --- VtValue ---
void tag_invoke(const value_from_tag&, value& json, const VtValue& v);
VtValue tag_invoke(const value_to_tag<VtValue>&, const value& json);

// --- GfVec3d ---
void tag_invoke(const value_from_tag&, value& json, const GfVec3d& v);
GfVec3d tag_invoke(const value_to_tag<GfVec3d>&, const value& json);

// --- GfVec2f ---
void tag_invoke(const value_from_tag&, value& json, const GfVec2f& v);
GfVec2f tag_invoke(const value_to_tag<GfVec2f>&, const value& json);

// --- GfVec3f ---
void tag_invoke(const value_from_tag&, value& json, const GfVec3f& v);
GfVec3f tag_invoke(const value_to_tag<GfVec3f>&, const value& json);

// --- SdfSpecifier ---
void tag_invoke(const value_from_tag&, value& json, const SdfSpecifier& v);
SdfSpecifier tag_invoke(const value_to_tag<SdfSpecifier>&, const value& json);

// --- SdfSpecType ---
void tag_invoke(const value_from_tag&, value& json, const SdfSpecType& v);
SdfSpecType tag_invoke(const value_to_tag<SdfSpecType>&, const value& json);

// --- SdfVariability ---
void tag_invoke(const value_from_tag&, value& json, const SdfVariability& v);
SdfVariability tag_invoke(const value_to_tag<SdfVariability>&, const value& json);

// --- GfMatrix4d ---
void tag_invoke(const value_from_tag&, value& json, const GfMatrix4d& v);
GfMatrix4d tag_invoke(const value_to_tag<GfMatrix4d>&, const value& json);

// --- GfMatrix4d ---
void tag_invoke(const value_from_tag&, value& json, const GfMatrix4d& v);
GfMatrix4d tag_invoke(const value_to_tag<GfMatrix4d>&, const value& json);

// --- SdfListOp ---
template <typename T>
void tag_invoke(const value_from_tag&, value& json, const SdfListOp<T>& v)
{
    object result;

    array explicitItems;
    array addedItems;
    array prependedItems;
    array appendedItems;
    array deletedItems;
    array orderedItems;

    for (const T& item : v.GetExplicitItems())
    {
        explicitItems.push_back(value_from(item));
    }

    for (const T& item : v.GetAddedItems())
    {
        addedItems.push_back(value_from(item));
    }

    for (const T& item : v.GetPrependedItems())
    {
        prependedItems.push_back(value_from(item));
    }

    for (const T& item : v.GetAppendedItems())
    {
        appendedItems.push_back(value_from(item));
    }

    for (const T& item : v.GetDeletedItems())
    {
        deletedItems.push_back(value_from(item));
    }

    for (const T& item : v.GetOrderedItems())
    {
        orderedItems.push_back(value_from(item));
    }

    result["explicit"] = explicitItems;
    result["added"] = addedItems;
    result["prepended"] = appendedItems;
    result["appended"] = appendedItems;
    result["deleted"] = deletedItems;
    result["ordered"] = orderedItems;
}


template <typename T>
SdfListOp<T> tag_invoke(const value_to_tag<SdfListOp<T>>&, const value& json)
{
    SdfListOp<T> result;

    SdfListOp<T>::ItemVector explicitItems;
    SdfListOp<T>::ItemVector addedItems;
    SdfListOp<T>::ItemVector prependedItems;
    SdfListOp<T>::ItemVector appendedItems;
    SdfListOp<T>::ItemVector deletedItems;
    SdfListOp<T>::ItemVector orderedItems;

    for (const auto& item : json["explicit"].as_array())
    {
        explicitItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json["added"].as_array())
    {
        addedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json["prepended"].as_array())
    {
        prependedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json["appended"].as_array())
    {
        appendedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json["deleted"].as_array())
    {
        deletedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json["ordered"].as_array())
    {
        orderedItems.push_back(value_to<T>(item));
    }
}

// --- VtArray<T> ---

/*
void tag_invoke(const value_from_tag&, value& json, const VtArray<int>& v)
{
    array result;

    for (const auto& item : v)
    {
        result.push_back(item);
    }

    json = result;
}*/

template <typename T>
VtArray<T> tag_invoke(const value_to_tag<VtArray<T>>&, const value& json)
{
    array jsonArray = json.as_array();

    VtArray<T> result(jsonArray.size());

    for (auto i = 0; i < jsonArray.size(); i++)
    {
        result[i] = value_to<T>(jsonArray[i]);
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
