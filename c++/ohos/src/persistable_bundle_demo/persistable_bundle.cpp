#include "persistable_bundle.h"
#include <limits>
#include <type_traits>
#include "string_ex.h"

namespace OHOS {

// Implementation of template methods
template<typename T>
bool PersistableBundle::PutValue(const std::string &key, const T &value, std::map<std::string, T> &map)
{
    Erase(key);
    map[key] = value;
    return true;
}

template<typename T>
bool PersistableBundle::GetValue(const std::string &key, T &value, const std::map<std::string, T> &map) const
{
    auto it = map.find(key);
    if (it == map.end()) {
        return false;
    }
    value = it->second;
    return true;
}

template<typename T>
std::set<std::string> PersistableBundle::GetKeys(const std::map<std::string, T> &map) const
{
    std::set<std::string> keys;
    for (const auto &pair : map) {
        keys.insert(pair.first);
    }
    return keys;
}


// Generate Put method implementations using X-macro
#define IMPLEMENT_PUT_METHOD(type, name) \
void PersistableBundle::Put##name(const std::string &key, const type &value) \
{ \
    PutValue(key, value, name##Map_); \
}

#define IMPLEMENT_PUT_VECTOR_METHOD(type, name) \
void PersistableBundle::Put##name##Vectors(const std::string &key, const std::vector<type> &value) \
{ \
    PutValue(key, value, name##VectorMap_); \
}

PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_PUT_METHOD)
PERSISTABLE_BUNDLE_VECTOR_TYPES(IMPLEMENT_PUT_VECTOR_METHOD)

void PersistableBundle::PutPersistableBundle(const std::string &key, const PersistableBundle &value)
{
    PutValue(key, value, bundleMap_);
}

// Generate Get method implementations
#define IMPLEMENT_GET_METHOD(type, name) \
bool PersistableBundle::Get##name(const std::string &key, type &value) const \
{ \
    return GetValue(key, value, name##Map_); \
}

#define IMPLEMENT_GET_VECTOR_METHOD(type, name) \
bool PersistableBundle::Get##name##Vectors(const std::string &key, std::vector<type> &value) const \
{ \
    return GetValue(key, value, name##VectorMap_); \
}

PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_GET_METHOD)
PERSISTABLE_BUNDLE_VECTOR_TYPES(IMPLEMENT_GET_VECTOR_METHOD)

bool PersistableBundle::GetPersistableBundle(const std::string &key, PersistableBundle &value) const
{
    return GetValue(key, value, bundleMap_);
}

// Generate GetKeys method implementations
#define IMPLEMENT_GET_KEYS_METHOD(_, name) \
std::set<std::string> PersistableBundle::Get##name##Keys() const \
{ \
    return GetKeys(name##Map_); \
}

#define IMPLEMENT_GET_VECTOR_KEYS_METHOD(_, name) \
std::set<std::string> PersistableBundle::Get##name##VectorsKeys() const \
{ \
    return GetKeys(name##VectorMap_); \
}

PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_GET_KEYS_METHOD)
PERSISTABLE_BUNDLE_VECTOR_TYPES(IMPLEMENT_GET_VECTOR_KEYS_METHOD)

std::set<std::string> PersistableBundle::GetPersistableBundleKeys() const
{
    return GetKeys(bundleMap_);
}

// Main methods
bool PersistableBundle::Marshalling(Parcel &parcel) const
{
    return WriteToParcelInner(parcel);
}

PersistableBundle *PersistableBundle::Unmarshalling(Parcel &parcel)
{
    auto bundle = new (std::nothrow) PersistableBundle();
    if (bundle != nullptr && !bundle->ReadFromParcelInner(parcel)) {
        delete bundle;
        bundle = nullptr;
    }
    return bundle;
}

bool PersistableBundle::Empty() const
{
    return Size() == 0;
}

size_t PersistableBundle::Size() const
{
    size_t total = 0;

    // Count all maps using X-macro
    #define COUNT_MAP(_, name) total += name##Map_.size();
    #define COUNT_VECTOR_MAP(_, name) total += name##VectorMap_.size();

    PERSISTABLE_BUNDLE_TYPES(COUNT_MAP)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(COUNT_VECTOR_MAP)
    total += bundleMap_.size();

    #undef COUNT_MAP
    #undef COUNT_VECTOR_MAP

    return total;
}

void PersistableBundle::Clear()
{
    #define CLEAR_MAP(type, name) name##Map_.clear();
    #define CLEAR_VECTOR_MAP(type, name) name##VectorMap_.clear();

    PERSISTABLE_BUNDLE_TYPES(CLEAR_MAP)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(CLEAR_VECTOR_MAP)
    bundleMap_.clear();

    #undef CLEAR_MAP
    #undef CLEAR_VECTOR_MAP
}

size_t PersistableBundle::Erase(const std::string &key)
{
    size_t erased = 0;

    // Erase from all maps using X-macro
    #define ERASE_FROM_MAP(type, name) erased += name##Map_.erase(key);
    #define ERASE_FROM_VECTOR_MAP(type, name) erased += name##VectorMap_.erase(key);

    PERSISTABLE_BUNDLE_TYPES(ERASE_FROM_MAP)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(ERASE_FROM_VECTOR_MAP)
    erased += bundleMap_.erase(key);

    #undef ERASE_FROM_MAP
    #undef ERASE_FROM_VECTOR_MAP

    return erased > 0 ? 1 : 0;
}

bool PersistableBundle::WriteToParcelInner(Parcel &parcel) const
{
    // Write the total number of entries across all maps
    size_t totalEntries = Size();
    if (!parcel.WriteUint32(static_cast<uint32_t>(totalEntries))) {
        return false;
    }

    // Define helper macros for processing different types of maps
    #define WRITE_MAP_ENTRIES(type, name, write_func) \
    do { \
        for (const auto &pair : name##Map_) { \
            if (!parcel.WriteString16(Str8ToStr16(pair.first))) { \
                return false; \
            } \
            if (!parcel.WriteInt32(VAL_##name)) { \
                return false; \
            } \
            if (!parcel.write_func(pair.second)) { \
                return false; \
            } \
        } \
    } while(0); \



    #define WRITE_VECTOR_MAP_ENTRIES(type, name, write_func) \
    do { \
        for (const auto &pair : name##VectorMap_) { \
            if (!parcel.WriteString16(Str8ToStr16(pair.first))) { \
                return false; \
            } \
            if (!parcel.WriteInt32(VAL_##name##_VECTOR)) { \
                return false; \
            } \
            if (!parcel.write_func(pair.second)) { \
                return false; \
            } \
        } \
    } while(0);

    // Write entries from all maps using X-macro
    #define WRITE_MAP(type, name) WRITE_MAP_ENTRIES(type, name, Write##name)
    #define WRITE_VECTOR_MAP(type, name) WRITE_VECTOR_MAP_ENTRIES(type, name, Write##name##Vector)
    PERSISTABLE_BUNDLE_TYPES(WRITE_MAP)
    PERSISTABLE_BUNDLE_VECTOR_TYPES(WRITE_VECTOR_MAP)
    #undef WRITE_MAP
    #undef WRITE_VECTOR_MAP

    // Write PersistableBundle entries
    for (const auto &pair : bundleMap_) {
        if (!parcel.WriteString16(Str8ToStr16(pair.first))) {
            return false;
        }
        if (!parcel.WriteInt32(VAL_PERSISTABLE_BUNDLE)) {
            return false;
        }
        if (!parcel.WriteParcelable(&pair.second)) {
            return false;
        }
    }

    #undef WRITE_MAP_ENTRIES
    #undef WRITE_VECTOR_MAP_ENTRIES

    return true;
}

bool PersistableBundle::ReadFromParcelInner(Parcel &parcel)
{
    // Read the total number of entries
    uint32_t totalEntries = 0;
    if (!parcel.ReadUint32(totalEntries)) {
        return false;
    }

    // Clear existing data in case this bundle is being reused
    Clear();

    // Loop through each entry
    for (uint32_t i = 0; i < totalEntries; ++i) {
        // Read the key
        std::u16string key16;
        if (!parcel.ReadString16(key16)) {
            return false;
        }
        std::string key = Str16ToStr8(key16);

        // Read the value type
        int32_t valueType = 0;
        if (!parcel.ReadInt32(valueType)) {
            return false;
        }

        // Based on the value type, read the appropriate value
        switch (valueType) {
#define CASE_READ_VALUE(type, name, read_func) \
            case VAL_##name: { \
                type value = parcel.read_func(); \
                PutValue(key, value, name##Map_); \
                break; \
            }

#define CASE_READ_VECTOR_VALUE(type, name, read_func) \
            case VAL_##name##_VECTOR: { \
                std::vector<type> value; \
                if (!parcel.read_func(&value)) { \
                    return false; \
                } \
                PutValue(key, value, name##VectorMap_); \
                break; \
            }

            #define PROCESS_READ_VALUE(type, name) CASE_READ_VALUE(type, name, Read##name)
            #define PROCESS_READ_VECTOR_VALUE(type, name) CASE_READ_VECTOR_VALUE(type, name, Read##name##Vector)
            PERSISTABLE_BUNDLE_TYPES(PROCESS_READ_VALUE)
            PERSISTABLE_BUNDLE_VECTOR_TYPES(PROCESS_READ_VECTOR_VALUE)
            #undef PROCESS_READ_VALUE
            #undef PROCESS_READ_VECTOR_VALUE

            case VAL_PERSISTABLE_BUNDLE: {
                PersistableBundle *bundle = parcel.ReadParcelable<PersistableBundle>();
                if (bundle == nullptr) {
                    return false;
                }
                PutValue(key, *bundle, bundleMap_);
                delete bundle;  // Transfer ownership to the map
                break;
            }

            default:
                // Unsupported type
                return false;
        }
    }

    return true;
}

} // namespace OHOS
