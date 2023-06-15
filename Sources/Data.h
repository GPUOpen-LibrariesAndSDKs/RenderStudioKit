#pragma once

#pragma warning(push, 0)
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/abstractData.h>
#include <pxr/usd/sdf/api.h>
#include <pxr/usd/sdf/data.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#pragma warning(pop)

#include "Notice.h"
#include "Serialization/Api.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioData);

class RenderStudioFileFormat;

class RenderStudioData : public SdfAbstractData
{
public:
    RenderStudioData();

    AR_API
    virtual ~RenderStudioData();

    /// SdfAbstractData overrides
    AR_API
    virtual bool StreamsData() const;

    AR_API
    virtual void CreateSpec(const SdfPath& path, SdfSpecType specType);
    AR_API
    virtual bool HasSpec(const SdfPath& path) const;
    AR_API
    virtual void EraseSpec(const SdfPath& path);
    AR_API
    virtual void MoveSpec(const SdfPath& oldPath, const SdfPath& newPath);
    AR_API
    virtual SdfSpecType GetSpecType(const SdfPath& path) const;

    AR_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName, SdfAbstractDataValue* value) const;
    AR_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName, VtValue* value = NULL) const;
    AR_API
    virtual bool
    HasSpecAndField(const SdfPath& path, const TfToken& fieldName, SdfAbstractDataValue* value, SdfSpecType* specType)
        const;

    AR_API
    virtual bool
    HasSpecAndField(const SdfPath& path, const TfToken& fieldName, VtValue* value, SdfSpecType* specType) const;

    AR_API
    virtual VtValue Get(const SdfPath& path, const TfToken& fieldName) const;
    AR_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName, const VtValue& value);
    AR_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName, const SdfAbstractDataConstValue& value);
    AR_API
    virtual void Erase(const SdfPath& path, const TfToken& fieldName);
    AR_API
    virtual std::vector<TfToken> List(const SdfPath& path) const;

    AR_API
    virtual std::set<double> ListAllTimeSamples() const;

    AR_API
    virtual std::set<double> ListTimeSamplesForPath(const SdfPath& path) const;

    AR_API
    virtual bool GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    AR_API
    virtual size_t GetNumTimeSamplesForPath(const SdfPath& path) const;

    AR_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path, double time, double* tLower, double* tUpper) const;

    AR_API
    virtual bool QueryTimeSample(const SdfPath& path, double time, SdfAbstractDataValue* optionalValue) const;
    AR_API
    virtual bool QueryTimeSample(const SdfPath& path, double time, VtValue* value) const;

    AR_API
    virtual void SetTimeSample(const SdfPath& path, double time, const VtValue& value);

    AR_API
    virtual void EraseTimeSample(const SdfPath& path, double time);

protected:
    // SdfAbstractData overrides
    AR_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    // Backing storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    struct _SpecData
    {
        _SpecData()
            : specType(SdfSpecTypeUnknown)
        {
        }

        SdfSpecType specType;
        std::vector<_FieldValuePair> fields;
    };

    // Hashtable storing _SpecData.
    typedef SdfPath _Key;
    typedef SdfPath::Hash _KeyHash;
    typedef TfHashMap<_Key, _SpecData, _KeyHash> _HashTable;

private:
    void ApplyDelta(
        SdfLayerHandle& layer,
        std::vector<RenderStudioPrimitiveNotice>& notices,
        const SdfPath& path,
        const TfToken& key,
        const VtValue& value,
        SdfSpecType spec);
    void ProcessRemoteUpdates(SdfLayerHandle& layer);
    void AccumulateRemoteUpdate(const _HashTable& deltas, std::size_t sequence);
    _HashTable FetchLocalDeltas();
    void OnLoaded();

    const VtValue* _GetSpecTypeAndFieldValue(const SdfPath& path, const TfToken& field, SdfSpecType* specType) const;

    const VtValue* _GetFieldValue(const SdfPath& path, const TfToken& field) const;

    VtValue* _GetMutableFieldValue(const SdfPath& path, const TfToken& field);

    VtValue* _GetOrCreateFieldValue(const SdfPath& path, const TfToken& field);

    VtValue* _GetOrCreateFieldValueDelta(const SdfPath& path, const TfToken& field);

private:
    friend class RenderStudioFileFormat;

    _HashTable mData;
    _HashTable mLocalDeltas;

    std::set<SdfPath> mUnacknowledgedFields;
    std::mutex mRemoteMutex;
    std::size_t mLatestAppliedSequence = 0;
    std::map<std::size_t, _HashTable> mRemoteDeltasQueue;
    bool mIsLoaded = false;
    bool mIsProcessingRemoteUpdates = false;
};

PXR_NAMESPACE_CLOSE_SCOPE
