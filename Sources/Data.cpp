#include "Data.h"

#pragma warning(push, 0)
#include <iostream>
#include <set>

#include <pxr/base/trace/trace.h>
#include <pxr/base/work/utils.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerStateDelegate.h>
#include <pxr/usd/usd/notice.h>
#pragma warning(pop)

#include "Logger/Logger.h"
#include "Resolver.h"
#include "Serialization/Serialization.h"

PXR_NAMESPACE_OPEN_SCOPE

RenderStudioData::RenderStudioData()
{
    // Do nothing
}

RenderStudioData::~RenderStudioData()
{
    // Clear out mData in parallel, since it can get big.
    WorkSwapDestroyAsync(mData);
}

void
RenderStudioData::ApplyDelta(
    SdfLayerHandle& layer,
    std::vector<RenderStudioPrimitiveNotice>& notices,
    const SdfPath& path,
    const TfToken& key,
    const VtValue& value,
    SdfSpecType spec)
{
    // Create field if not exist
    if (layer->GetSpecType(path) == SdfSpecTypeUnknown)
    {
        layer->GetStateDelegate()->CreateSpec(path, spec, false);
        if (path.IsPrimPath())
        {
            notices.push_back(RenderStudioPrimitiveNotice(path, true));
        }
    }

    // Check if it's primitive lock by other user
    if (key == SdfFieldKeys->CustomData && value.IsHolding<VtDictionary>())
    {
        VtDictionary data = value.Get<VtDictionary>();
        auto it = data.find("owner");
        if (it != data.end() && it->first == "owner")
        {
            std::optional<std::string> owner = [&it]() -> std::optional<std::string>
            {
                std::string owner = it->second.Get<std::string>();
                if (owner == "None")
                {
                    return std::nullopt;
                }
                else
                {
                    return owner;
                }
            }();

            RenderStudioOwnerNotice(path, owner).Send();
        }
    }

    bool unacknowledgedYet = mUnacknowledgedFields.count(path) > 0;
    bool requireMerge = key == SdfChildrenKeys->PrimChildren && unacknowledgedYet;
    bool requireForceApply = path.GetNameToken().GetString().find("xformOp:") != std::string::npos;

    // Ignore all unacknowledged updates except mergeable or which must be force applied
    // Merge updates might not be skipped because we need to have both own edit and other user edit, overwrites
    // prohibited Transformations must be always applied to guarantee that each user would receive update because we
    // would use implicit primitive locks for editing All other unacknowledged attributes might be skipped, because we
    // are sure that our edit would be latest in history

    if (!requireForceApply && !requireMerge && unacknowledgedYet)
    {
        LOG_DEBUG << "Skip unacknowledged message: " << path;
        return;
    }

    if (requireMerge)
    {
        // Merge corner case
        auto localData = layer->GetField(path, key).GetWithDefault<std::vector<TfToken>>();
        auto remoteData = value.GetWithDefault<std::vector<TfToken>>();

        // Merge vectors, if our update is acknowledged then it would appear earlier in vector
        std::vector<TfToken> merged;

        if (unacknowledgedYet)
        {
            merged.insert(merged.end(), remoteData.begin(), remoteData.end());
            merged.insert(merged.end(), localData.begin(), localData.end());
        }
        else
        {
            merged.insert(merged.end(), localData.begin(), localData.end());
            merged.insert(merged.end(), remoteData.begin(), remoteData.end());
        }

        // Remove duplicates
        std::unordered_set<TfToken, TfToken::HashFunctor> seen;
        std::vector<TfToken> deduplicated;

        for (const auto& item : merged)
        {
            if (seen.find(item) == seen.end())
            {
                deduplicated.push_back(item);
                seen.insert(item);
            }
        }

        layer->GetStateDelegate()->SetField(path, key, VtValue { deduplicated });
    }
    else
    {
        // Regular field update
        layer->GetStateDelegate()->SetField(path, key, value);
    }

    if (key == SdfFieldKeys->Active)
    {
        notices.push_back(RenderStudioPrimitiveNotice(path, true));
    }
}

void
RenderStudioData::ProcessRemoteUpdates(SdfLayerHandle& layer)
{
    // Synchronize updates
    std::unique_lock<std::mutex> lock(mRemoteMutex);
    mIsProcessingRemoteUpdates = true;

    // Change block should gain performance
    std::unique_ptr<SdfChangeBlock> block = std::make_unique<SdfChangeBlock>();
    std::vector<RenderStudioPrimitiveNotice> notices;

    // Apply all the deltas (in sequence order)
    std::size_t nextRequestedSequence = mLatestAppliedSequence + 1;

    while (mRemoteDeltasQueue.find(nextRequestedSequence) != mRemoteDeltasQueue.end())
    {
        _HashTable& deltas = mRemoteDeltasQueue.at(nextRequestedSequence);

        for (const std::pair<SdfPath, _SpecData>& delta : deltas)
        {
            // Check if it's acknowledge message (for now it just doesn't contain fields)
            if (delta.second.fields.empty())
            {
                mUnacknowledgedFields.erase(delta.first);
                continue;
            }

            // Process updates
            for (const std::pair<TfToken, VtValue>& field : delta.second.fields)
            {
                ApplyDelta(layer, notices, delta.first, field.first, field.second, delta.second.specType);
            }

            notices.push_back(RenderStudioPrimitiveNotice(delta.first, false));
        }

        mLatestAppliedSequence = nextRequestedSequence;
        mRemoteDeltasQueue.erase(nextRequestedSequence);
        nextRequestedSequence += 1;
    }

    block.reset();

    // Deduplicate notices
    std::map<SdfPath, std::vector<RenderStudioPrimitiveNotice>> noticesMap;
    for (const RenderStudioPrimitiveNotice& notice : notices)
    {
        if (notice.IsValid())
        {
            noticesMap[notice.GetChangedPrim()].push_back(notice);
        }
    }

    for (const auto& [path, noticeVector] : noticesMap)
    {
        auto it = std::find_if(
            noticeVector.begin(), noticeVector.end(), [](const auto& item) { return item.WasResynched(); });

        if (it != noticeVector.end())
        {
            it->Send();
        }
        else
        {
            noticeVector.front().Send();
        }
    }

    mIsProcessingRemoteUpdates = false;
}

void
RenderStudioData::AccumulateRemoteUpdate(const _HashTable& deltas, std::size_t sequence)
{
    std::unique_lock<std::mutex> lock(mRemoteMutex);
    mRemoteDeltasQueue[sequence] = deltas;
}

RenderStudioData::_HashTable
RenderStudioData::FetchLocalDeltas()
{
    auto copy = mLocalDeltas;
    mLocalDeltas.clear();
    return copy;
}

void
RenderStudioData::OnLoaded()
{
    mIsLoaded = true;
}

bool
RenderStudioData::StreamsData() const
{
    return true;
}

bool
RenderStudioData::HasSpec(const SdfPath& path) const
{
    return mData.find(path) != mData.end();
}

void
RenderStudioData::EraseSpec(const SdfPath& path)
{
    _HashTable::iterator i = mData.find(path);
    if (!TF_VERIFY(i != mData.end(), "No spec to erase at <%s>", path.GetText()))
    {
        return;
    }
    mData.erase(i);
}

void
RenderStudioData::MoveSpec(const SdfPath& oldPath, const SdfPath& newPath)
{
    _HashTable::iterator old = mData.find(oldPath);
    if (!TF_VERIFY(old != mData.end(), "No spec to move at <%s>", oldPath.GetString().c_str()))
    {
        return;
    }
    bool inserted = mData.insert(std::make_pair(newPath, old->second)).second;
    if (!TF_VERIFY(inserted))
    {
        return;
    }
    mData.erase(old);
}

SdfSpecType
RenderStudioData::GetSpecType(const SdfPath& path) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i == mData.end())
    {
        return SdfSpecTypeUnknown;
    }
    return i->second.specType;
}

void
RenderStudioData::CreateSpec(const SdfPath& path, SdfSpecType specType)
{
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown))
    {
        return;
    }
    mData[path].specType = specType;
}

void
RenderStudioData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    TF_FOR_ALL(it, mData)
    {
        if (!visitor->VisitSpec(*this, it->first))
        {
            break;
        }
    }
}

bool
RenderStudioData::Has(const SdfPath& path, const TfToken& field, SdfAbstractDataValue* value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field))
    {
        if (value)
        {
            return value->StoreValue(*fieldValue);
        }
        return true;
    }
    return false;
}

bool
RenderStudioData::Has(const SdfPath& path, const TfToken& field, VtValue* value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field))
    {
        if (value)
        {
            *value = *fieldValue;
        }
        return true;
    }
    return false;
}

bool
RenderStudioData::HasSpecAndField(
    const SdfPath& path,
    const TfToken& fieldName,
    SdfAbstractDataValue* value,
    SdfSpecType* specType) const
{
    if (VtValue const* v = _GetSpecTypeAndFieldValue(path, fieldName, specType))
    {
        return !value || value->StoreValue(*v);
    }
    return false;
}

bool
RenderStudioData::HasSpecAndField(const SdfPath& path, const TfToken& fieldName, VtValue* value, SdfSpecType* specType)
    const
{
    if (VtValue const* v = _GetSpecTypeAndFieldValue(path, fieldName, specType))
    {
        if (value)
        {
            *value = *v;
        }
        return true;
    }
    return false;
}

const VtValue*
RenderStudioData::_GetSpecTypeAndFieldValue(const SdfPath& path, const TfToken& field, SdfSpecType* specType) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i == mData.end())
    {
        *specType = SdfSpecTypeUnknown;
    }
    else
    {
        const _SpecData& spec = i->second;
        *specType = spec.specType;
        for (auto const& f : spec.fields)
        {
            if (f.first == field)
            {
                return &f.second;
            }
        }
    }
    return nullptr;
}

const VtValue*
RenderStudioData::_GetFieldValue(const SdfPath& path, const TfToken& field) const
{
    _HashTable::const_iterator i = mData.find(path);
    if (i != mData.end())
    {
        const _SpecData& spec = i->second;
        for (auto const& f : spec.fields)
        {
            if (f.first == field)
            {
                return &f.second;
            }
        }
    }
    return nullptr;
}

VtValue*
RenderStudioData::_GetMutableFieldValue(const SdfPath& path, const TfToken& field)
{
    _HashTable::iterator i = mData.find(path);
    if (i != mData.end())
    {
        _SpecData& spec = i->second;
        for (size_t j = 0, jEnd = spec.fields.size(); j != jEnd; ++j)
        {
            if (spec.fields[j].first == field)
            {
                return &spec.fields[j].second;
            }
        }
    }
    return NULL;
}

VtValue
RenderStudioData::Get(const SdfPath& path, const TfToken& field) const
{
    if (const VtValue* value = _GetFieldValue(path, field))
    {
        return *value;
    }
    return VtValue();
}

void
RenderStudioData::Set(const SdfPath& path, const TfToken& field, const VtValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "RenderStudioData::Set");

    if (value.IsEmpty())
    {
        Erase(path, field);
        return;
    }

    // Default
    VtValue* newValue = _GetOrCreateFieldValue(path, field);

    if (newValue)
    {
        *newValue = value;
    }

    // USD calls Set() method while setting fields, we don't want to update local deltas in such case
    if (mIsProcessingRemoteUpdates || !mIsLoaded)
    {
        return;
    }

    // Delta
    VtValue* newValueDelta = _GetOrCreateFieldValueDelta(path, field);
    if (newValueDelta)
    {
        *newValueDelta = value;
    }
}

void
RenderStudioData::Set(const SdfPath& path, const TfToken& field, const SdfAbstractDataConstValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "RenderStudioData::Set");

    // Default
    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue)
    {
        value.GetValue(newValue);
    }

    // USD calls Set() method while setting fields, we don't want to update local deltas in such case
    if (mIsProcessingRemoteUpdates || !mIsLoaded)
    {
        return;
    }

    // Delta
    VtValue* newValueDelta = _GetOrCreateFieldValueDelta(path, field);
    if (newValueDelta)
    {
        value.GetValue(newValueDelta);
    }
}

VtValue*
RenderStudioData::_GetOrCreateFieldValue(const SdfPath& path, const TfToken& field)
{
    _HashTable::iterator i = mData.find(path);
    if (!TF_VERIFY(i != mData.end(), "No spec at <%s> when trying to set field '%s'", path.GetText(), field.GetText()))
    {
        return nullptr;
    }

    _SpecData& spec = i->second;
    for (auto& f : spec.fields)
    {
        if (f.first == field)
        {
            return &f.second;
        }
    }

    spec.fields.emplace_back(std::piecewise_construct, std::forward_as_tuple(field), std::forward_as_tuple());

    return &spec.fields.back().second;
}

VtValue*
RenderStudioData::_GetOrCreateFieldValueDelta(const SdfPath& path, const TfToken& field)
{
    // Apply spec type from mData to _deltas
    if (mLocalDeltas.find(path) == mLocalDeltas.end())
    {
        _HashTable::iterator i = mData.find(path);

        if (!TF_VERIFY(
                i != mData.end(), "No spec at <%s> when trying to set field '%s'", path.GetText(), field.GetText()))
        {
            return nullptr;
        }

        mLocalDeltas[path].specType = i->second.specType;
    }

    _HashTable::iterator i = mLocalDeltas.find(path);
    _SpecData& spec = i->second;

    for (auto& f : spec.fields)
    {
        if (f.first == field)
        {
            return &f.second;
        }
    }

    spec.fields.emplace_back(std::piecewise_construct, std::forward_as_tuple(field), std::forward_as_tuple());

    // Hypothesis: Ignoring all unacknowledged paths during updates receiving would grant data consistency
    mUnacknowledgedFields.insert(path);

    return &spec.fields.back().second;
}

void
RenderStudioData::Erase(const SdfPath& path, const TfToken& field)
{
    _HashTable::iterator i = mData.find(path);
    if (i == mData.end())
    {
        return;
    }

    _SpecData& spec = i->second;
    for (size_t j = 0, jEnd = spec.fields.size(); j != jEnd; ++j)
    {
        if (spec.fields[j].first == field)
        {
            spec.fields.erase(spec.fields.begin() + j);
            return;
        }
    }
}

std::vector<TfToken>
RenderStudioData::List(const SdfPath& path) const
{
    std::vector<TfToken> names;
    _HashTable::const_iterator i = mData.find(path);
    if (i != mData.end())
    {
        const _SpecData& spec = i->second;

        const size_t numFields = spec.fields.size();
        names.resize(numFields);
        for (size_t j = 0; j != numFields; ++j)
        {
            names[j] = spec.fields[j].first;
        }
    }
    return names;
}

////////////////////////////////////////////////////////////////////////
// This is a basic prototype implementation of the time-sampling API
// for in-memory, non cached presto layers.

std::set<double>
RenderStudioData::ListAllTimeSamples() const
{
    // Use a set to determine unique times.
    std::set<double> times;

    TF_FOR_ALL(i, mData)
    {
        std::set<double> timesForPath = ListTimeSamplesForPath(i->first);
        times.insert(timesForPath.begin(), timesForPath.end());
    }

    return times;
}

std::set<double>
RenderStudioData::ListTimeSamplesForPath(const SdfPath& path) const
{
    std::set<double> times;

    VtValue value = Get(path, SdfDataTokens->TimeSamples);
    if (value.IsHolding<SdfTimeSampleMap>())
    {
        const SdfTimeSampleMap& timeSampleMap = value.UncheckedGet<SdfTimeSampleMap>();
        TF_FOR_ALL(j, timeSampleMap) { times.insert(j->first); }
    }

    return times;
}

template <class Container, class GetTime>
static bool
_GetBracketingTimeSamplesImpl(
    const Container& samples,
    const GetTime& getTime,
    const double time,
    double* tLower,
    double* tUpper)
{
    if (samples.empty())
    {
        // No samples.
        return false;
    }
    else if (time <= getTime(*samples.begin()))
    {
        // Time is at-or-before the first sample.
        *tLower = *tUpper = getTime(*samples.begin());
    }
    else if (time >= getTime(*samples.rbegin()))
    {
        // Time is at-or-after the last sample.
        *tLower = *tUpper = getTime(*samples.rbegin());
    }
    else
    {
        auto iter = samples.lower_bound(time);
        if (getTime(*iter) == time)
        {
            // Time is exactly on a sample.
            *tLower = *tUpper = getTime(*iter);
        }
        else
        {
            // Time is in-between samples; return the bracketing times.
            *tUpper = getTime(*iter);
            --iter;
            *tLower = getTime(*iter);
        }
    }
    return true;
}

static bool
_GetBracketingTimeSamples(const std::set<double>& samples, double time, double* tLower, double* tUpper)
{
    return _GetBracketingTimeSamplesImpl(
        samples, [](double t) { return t; }, time, tLower, tUpper);
}

static bool
_GetBracketingTimeSamples(const SdfTimeSampleMap& samples, double time, double* tLower, double* tUpper)
{
    return _GetBracketingTimeSamplesImpl(
        samples, [](SdfTimeSampleMap::value_type const& p) { return p.first; }, time, tLower, tUpper);
}

bool
RenderStudioData::GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const
{
    return _GetBracketingTimeSamples(ListAllTimeSamples(), time, tLower, tUpper);
}

size_t
RenderStudioData::GetNumTimeSamplesForPath(const SdfPath& path) const
{
    if (const VtValue* fval = _GetFieldValue(path, SdfDataTokens->TimeSamples))
    {
        if (fval->IsHolding<SdfTimeSampleMap>())
        {
            return fval->UncheckedGet<SdfTimeSampleMap>().size();
        }
    }
    return 0;
}

bool
RenderStudioData::GetBracketingTimeSamplesForPath(const SdfPath& path, double time, double* tLower, double* tUpper)
    const
{
    const VtValue* fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>())
    {
        auto const& tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        return _GetBracketingTimeSamples(tsmap, time, tLower, tUpper);
    }
    return false;
}

bool
RenderStudioData::QueryTimeSample(const SdfPath& path, double time, VtValue* value) const
{
    const VtValue* fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>())
    {
        auto const& tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end())
        {
            if (value)
                *value = iter->second;
            return true;
        }
    }
    return false;
}

bool
RenderStudioData::QueryTimeSample(const SdfPath& path, double time, SdfAbstractDataValue* value) const
{
    const VtValue* fval = _GetFieldValue(path, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>())
    {
        auto const& tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end())
        {
            return !value || value->StoreValue(iter->second);
        }
    }
    return false;
}

void
RenderStudioData::SetTimeSample(const SdfPath& path, double time, const VtValue& value)
{
    if (value.IsEmpty())
    {
        EraseTimeSample(path, time);
        return;
    }

    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue* fieldValue = _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>())
    {
        fieldValue->UncheckedSwap(newSamples);
    }

    // Insert or overwrite into newSamples.
    newSamples[time] = value;

    // Set back into the field.
    if (fieldValue)
    {
        fieldValue->Swap(newSamples);
    }
    else
    {
        Set(path, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
    }
}

void
RenderStudioData::EraseTimeSample(const SdfPath& path, double time)
{
    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue* fieldValue = _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.  If we do not have one,
    // there's nothing to erase so we're done.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>())
    {
        fieldValue->UncheckedSwap(newSamples);
    }
    else
    {
        return;
    }

    // Erase from newSamples.
    newSamples.erase(time);

    // Check to see if the result is empty.  In that case we remove the field.
    if (newSamples.empty())
    {
        Erase(path, SdfDataTokens->TimeSamples);
    }
    else
    {
        fieldValue->UncheckedSwap(newSamples);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
