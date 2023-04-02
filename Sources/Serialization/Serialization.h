#pragma once

#pragma warning(push, 0)
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/reference.h>

#include <boost/json.hpp>
#pragma warning(pop)

PXR_NAMESPACE_OPEN_SCOPE

using namespace boost::json;

// --- SdfPath ---
void tag_invoke(const value_from_tag&, value& json, const SdfPath& v);
SdfPath tag_invoke(const value_to_tag<SdfPath>&, const value& json);

// --- SdfAssetPath ---
void tag_invoke(const value_from_tag&, value& json, const SdfAssetPath& v);
SdfAssetPath tag_invoke(const value_to_tag<SdfAssetPath>&, const value& json);

// --- SdfReference ---
void tag_invoke(const value_from_tag&, value& json, const SdfReference& v);
SdfReference tag_invoke(const value_to_tag<SdfReference>&, const value& json);

// --- SdfLayerOffset ---
void tag_invoke(const value_from_tag&, value& json, const SdfLayerOffset& v);
SdfLayerOffset tag_invoke(const value_to_tag<SdfLayerOffset>&, const value& json);

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
void
tag_invoke(const value_from_tag&, value& json, const SdfListOp<T>& v)
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
    result["prepended"] = prependedItems;
    result["appended"] = appendedItems;
    result["deleted"] = deletedItems;
    result["ordered"] = orderedItems;

    json = result;
}

template <typename T>
SdfListOp<T>
tag_invoke(const value_to_tag<SdfListOp<T>>&, const value& json)
{
    typename SdfListOp<T>::ItemVector explicitItems {};
    typename SdfListOp<T>::ItemVector addedItems {};
    typename SdfListOp<T>::ItemVector prependedItems {};
    typename SdfListOp<T>::ItemVector appendedItems {};
    typename SdfListOp<T>::ItemVector deletedItems {};
    typename SdfListOp<T>::ItemVector orderedItems {};

    for (const auto& item : json.as_object().at("explicit").as_array())
    {
        explicitItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json.as_object().at("added").as_array())
    {
        addedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json.as_object().at("prepended").as_array())
    {
        prependedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json.as_object().at("appended").as_array())
    {
        appendedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json.as_object().at("deleted").as_array())
    {
        deletedItems.push_back(value_to<T>(item));
    }

    for (const auto& item : json.as_object().at("ordered").as_array())
    {
        orderedItems.push_back(value_to<T>(item));
    }

    // Documentation says do not use 'ordered' or 'added' item lists, so skip
    // https://openusd.org/release/api/class_sdf_list_op.html#a9dc4cbc201d40b0e55948f251fbf65f0

    if (!explicitItems.empty())
    {
        return SdfListOp<T>::CreateExplicit(explicitItems);
    }
    else
    {
        return SdfListOp<T>::Create(prependedItems, appendedItems, deletedItems);
    }
}

// --- VtArray<T> ---
template <typename T>
void
tag_invoke(const value_from_tag&, value& json, const VtArray<T>& v)
{
    array result;

    for (const auto& item : v)
    {
        result.push_back(value_from(item));
    }

    json = result;
}

template <typename T>
VtArray<T>
tag_invoke(const value_to_tag<VtArray<T>>&, const value& json)
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
