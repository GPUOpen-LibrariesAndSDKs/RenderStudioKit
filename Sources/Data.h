#pragma once

#include <pxr/pxr.h>
#include <pxr/usd/sdf/api.h>
#include <pxr/usd/sdf/abstractData.h>
#include <pxr/usd/sdf/data.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(RenderStudioData);

class RenderStudioData : public SdfData
{
public:
    RenderStudioData() {}

    SDF_API
    virtual ~RenderStudioData();

    /// SdfAbstractData overrides

    SDF_API
    virtual bool StreamsData() const;

    SDF_API
    virtual void CreateSpec(const SdfPath& path, 
                            SdfSpecType specType);
    SDF_API
    virtual bool HasSpec(const SdfPath& path) const;
    SDF_API
    virtual void EraseSpec(const SdfPath& path);
    SDF_API
    virtual void MoveSpec(const SdfPath& oldPath, 
                          const SdfPath& newPath);
    SDF_API
    virtual SdfSpecType GetSpecType(const SdfPath& path) const;

    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken &fieldName,
                     SdfAbstractDataValue* value) const;
    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     VtValue *value = NULL) const;
    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    SdfAbstractDataValue *value, SdfSpecType *specType) const;

    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    VtValue *value, SdfSpecType *specType) const;

    SDF_API
    virtual VtValue Get(const SdfPath& path, 
                        const TfToken& fieldName) const;
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const VtValue & value);
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    SDF_API
    virtual void Erase(const SdfPath& path, 
                       const TfToken& fieldName);
    SDF_API
    virtual std::vector<TfToken> List(const SdfPath& path) const;

    SDF_API
    virtual std::set<double>
    ListAllTimeSamples() const;
    
    SDF_API
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    SDF_API
    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path, 
                                    double time,
                                    double* tLower, double* tUpper) const;

    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    SdfAbstractDataValue *optionalValue) const;
    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time, 
                    VtValue *value) const;

    SDF_API
    virtual void
    SetTimeSample(const SdfPath& path, double time, 
                  const VtValue & value);

    SDF_API
    virtual void
    EraseTimeSample(const SdfPath& path, double time);

protected:
    // SdfAbstractData overrides
    SDF_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    const VtValue* _GetSpecTypeAndFieldValue(const SdfPath& path,
                                             const TfToken& field,
                                             SdfSpecType* specType) const;

    const VtValue* _GetFieldValue(const SdfPath& path,
                                  const TfToken& field) const;

    VtValue* _GetMutableFieldValue(const SdfPath& path,
                                   const TfToken& field);

    VtValue* _GetOrCreateFieldValue(const SdfPath& path,
                                    const TfToken& field);

private:
    // Backing storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    struct _SpecData {
        _SpecData() : specType(SdfSpecTypeUnknown) {}
        
        SdfSpecType specType;
        std::vector<_FieldValuePair> fields;
    };

    // Hashtable storing _SpecData.
    typedef SdfPath _Key;
    typedef SdfPath::Hash _KeyHash;
    typedef TfHashMap<_Key, _SpecData, _KeyHash> _HashTable;

    _HashTable _data;
};

PXR_NAMESPACE_CLOSE_SCOPE
