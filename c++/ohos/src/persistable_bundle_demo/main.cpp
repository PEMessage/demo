#include <iostream>
#include "parcel.h"
#include "xxd.h"
#include "string.h"

using namespace OHOS;


#include <iostream>
#include "persistable_bundle.h"

using namespace OHOS;

void PrintBundleInfo(const PersistableBundle& bundle) {
    std::cout << "Bundle size: " << bundle.Size() << std::endl;

    std::string name;
    if (bundle.GetString("name", name)) {
        std::cout << "name: " << name << std::endl;
    }

    int32_t version;
    if (bundle.GetInt32("version", version)) {
        std::cout << "version: " << version << std::endl;
    }

    bool enabled;
    if (bundle.GetBool("enabled", enabled)) {
        std::cout << "enabled: " << (enabled ? "true" : "false") << std::endl;
    }
}

void CopyParcel(const OHOS::Parcel& source, OHOS::Parcel& target)
{
    OHOS::Parcel* dest = new OHOS::Parcel();

    size_t dataSize = source.GetDataSize();
    const uint8_t* srcData = reinterpret_cast<const uint8_t*>(source.GetData());

    if (dataSize > 0 && srcData != nullptr) {
        uint8_t* buffer = new uint8_t[dataSize];
        memcpy(buffer, srcData, dataSize);

        target.ParseFrom(reinterpret_cast<uintptr_t>(buffer), dataSize);

    }
}


int main() {
    std::cout << "\n=== Create PersistableBundle ===" << std::endl;

    // Create a bundle
    PersistableBundle bundle;

    // Put some values
    bundle.PutString("name", "OpenHarmony");
    bundle.PutInt32("version", 4);
    bundle.PutBool("enabled", true);

    PrintBundleInfo(bundle);

    // Serialize to parcel
    Parcel parcel;
    if (!bundle.Marshalling(parcel)) {
        std::cerr << "Failed to marshal bundle!" << std::endl;
        return 1;
    }

    std::cout << "Parcel data size: " << parcel.GetDataSize() << " bytes" << std::endl;


    std::cout << "\n===  After restore ===" << std::endl;
    // Deserialize from parcel
    Parcel parcel2;
    CopyParcel(parcel, parcel2);
    parcel2.ParseFrom(parcel.GetData(), parcel.GetDataSize());

    PersistableBundle* restoredBundle = PersistableBundle::Unmarshalling(parcel2);
    if (restoredBundle == nullptr) {
        std::cerr << "Failed to unmarshal bundle!" << std::endl;
        return 1;
    }

    PrintBundleInfo(*restoredBundle);

    std::cout << "\n===  erasing key 'enable' ===" << std::endl;
    // Test erasing a key
    restoredBundle->Erase("enabled");
    std::cout << "After erasing 'enabled':" << std::endl;
    std::cout << "Bundle size: " << restoredBundle->Size() << std::endl;

    bool exists;
    if (!restoredBundle->GetBool("enabled", exists)) {
        std::cout << "'enabled' key no longer exists" << std::endl;
    }
    PrintBundleInfo(*restoredBundle);

    // Test getting all keys
    auto stringKeys = restoredBundle->GetStringKeys();
    if (!stringKeys.empty()) {
        std::cout << "String keys: ";
        for (const auto& key : stringKeys) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
    }

    delete restoredBundle;

    std::cout << "\n=== Demo Completed ===" << std::endl;
    return 0;
}
