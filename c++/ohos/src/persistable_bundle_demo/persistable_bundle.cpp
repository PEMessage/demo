#include "persistable_bundle.h"
#include <limits>
#include <type_traits>

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
PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_PUT_VECTOR_METHOD)

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
PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_GET_VECTOR_METHOD)

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
PERSISTABLE_BUNDLE_TYPES(IMPLEMENT_GET_VECTOR_KEYS_METHOD)

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
    PERSISTABLE_BUNDLE_TYPES(COUNT_VECTOR_MAP)
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
    PERSISTABLE_BUNDLE_TYPES(CLEAR_VECTOR_MAP)
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
    PERSISTABLE_BUNDLE_TYPES(ERASE_FROM_VECTOR_MAP)
    erased += bundleMap_.erase(key);

    #undef ERASE_FROM_MAP
    #undef ERASE_FROM_VECTOR_MAP

    return erased > 0 ? 1 : 0;
}

bool PersistableBundle::WriteToParcelInner(Parcel &parcel) const
{
    // TODO
}

bool PersistableBundle::ReadFromParcelInner(Parcel &parcel)
{
    // TODO
}

} // namespace OHOS
