#ifndef OHOS_UTILS_PERSISTABLE_BUNDLE_H
#define OHOS_UTILS_PERSISTABLE_BUNDLE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>
#include "parcel.h"
#include "nocopyable.h"
#include "string_ex.h"

namespace OHOS {

/**
 * @brief Provides a container for storing key-value pairs that can be persisted to a parcel.
 */
class PersistableBundle : public virtual Parcelable {
public:
    PersistableBundle() = default;
    virtual ~PersistableBundle() = default;

    bool Marshalling(Parcel &parcel) const override;
    static PersistableBundle *Unmarshalling(Parcel &parcel);

    bool Empty() const;
    size_t Size() const;
    void Clear();
    size_t Erase(const std::string &key);

    // X-macro for generating all type methods
    // type name
    #define PERSISTABLE_BUNDLE_TYPES(X) \
        X(bool, Bool) \
        X(int32_t, Int32) \
        X(int64_t, Int64) \
        X(uint8_t, Uint8) \
        X(uint16_t, Uint16) \
        X(uint32_t, Uint32) \
        X(uint64_t, Uint64) \
        X(float, Float) \
        X(double, Double) \
        X(std::string, String) \
        X(std::u16string, String16)

    #define PERSISTABLE_BUNDLE_VECTOR_TYPES(X) \
        X(bool, Bool) \
        X(int32_t, Int32) \
        X(int64_t, Int64) \
        X(uint8_t, UInt8) \
        X(uint16_t, UInt16) \
        X(uint32_t, UInt32) \
        X(uint64_t, UInt64) \
        X(float, Float) \
        X(double, Double) \
        X(std::string, String) \
        X(std::u16string, String16)

    // Generate Put methods
    #define DECLARE_PUT_METHOD(type, name) \
        void Put##name(const std::string &key, const type &value);

    #define DECLARE_PUT_VECTOR_METHOD(type, name) \
        void Put##name##Vectors(const std::string &key, const std::vector<type> &value);

    PERSISTABLE_BUNDLE_TYPES(DECLARE_PUT_METHOD)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(DECLARE_PUT_VECTOR_METHOD)
    void PutPersistableBundle(const std::string &key, const PersistableBundle &value);

    // Generate Get methods
    #define DECLARE_GET_METHOD(type, name) \
        bool Get##name(const std::string &key, type &value) const;

    #define DECLARE_GET_VECTOR_METHOD(type, name) \
        bool Get##name##Vectors(const std::string &key, std::vector<type> &value) const;

    PERSISTABLE_BUNDLE_TYPES(DECLARE_GET_METHOD)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(DECLARE_GET_VECTOR_METHOD)
    bool GetPersistableBundle(const std::string &key, PersistableBundle &value) const;

    // Generate GetKeys methods
    #define DECLARE_GET_KEYS_METHOD(_, name) \
        std::set<std::string> Get##name##Keys() const;

    #define DECLARE_GET_VECTOR_KEYS_METHOD(_, name) \
        std::set<std::string> Get##name##VectorsKeys() const;

    PERSISTABLE_BUNDLE_TYPES(DECLARE_GET_KEYS_METHOD)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(DECLARE_GET_VECTOR_KEYS_METHOD)
    std::set<std::string> GetPersistableBundleKeys() const;

private:
    // DISALLOW_COPY_AND_MOVE(PersistableBundle);

    // Type identifiers for serialization
    #define ENUM_FIELD(_, name) VAL_##name,
    #define ENUM_FIELD_VECTORS(_, name) VAL_##name##_VECTOR,
    enum ValueType {
        PERSISTABLE_BUNDLE_TYPES(ENUM_FIELD)
        PERSISTABLE_BUNDLE_VECTOR_TYPES(ENUM_FIELD_VECTORS)
        VAL_PERSISTABLE_BUNDLE
    };
    #undef ENUM_FIELD
    #undef ENUM_FIELD_VECTORS

    // Helper templates
    template<typename T>
    bool PutValue(const std::string &key, const T &value, std::map<std::string, T> &map);

    template<typename T>
    bool GetValue(const std::string &key, T &value, const std::map<std::string, T> &map) const;

    template<typename T>
    std::set<std::string> GetKeys(const std::map<std::string, T> &map) const;

    // Storage using unified template approach
    #define DECLARE_MAP(type, name) \
        std::map<std::string, type> name##Map_;

    #define DECLARE_VECTOR_MAP(type, name) \
        std::map<std::string, std::vector<type>> name##VectorMap_;

    PERSISTABLE_BUNDLE_TYPES(DECLARE_MAP)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(DECLARE_VECTOR_MAP)
    std::map<std::string, PersistableBundle> bundleMap_;

    // Internal serialization methods
    bool WriteToParcelInner(Parcel &parcel) const;
    bool ReadFromParcelInner(Parcel &parcel);
};

} // namespace OHOS

#endif // OHOS_UTILS_PERSISTABLE_BUNDLE_H
