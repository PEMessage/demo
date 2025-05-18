#include <iostream>
#include "parcel.h"

using namespace OHOS;

// A simple Parcelable class example
class MyParcelable : public Parcelable {
public:
    MyParcelable() = default;
    explicit MyParcelable(int32_t val) : value_(val) {}
    
    bool Marshalling(Parcel &parcel) const override {
        return parcel.WriteInt32(value_);
    }
    
    static MyParcelable *Unmarshalling(Parcel &parcel) {
        int32_t value;
        if (parcel.ReadInt32(value)) {
            return new MyParcelable(value);
        }
        return nullptr;
    }
    
    int32_t GetValue() const { return value_; }
    
private:
    int32_t value_ = 0;
};

int main() {
    // Create a parcel
    Parcel parcel;
    
    // Write primitive types
    parcel.WriteInt32(42);
    parcel.WriteFloat(3.14f);
    parcel.WriteString("Hello, Parcel!");
    
    // Write a vector
    std::vector<int32_t> numbers = {1, 2, 3, 4, 5};
    parcel.WriteInt32Vector(numbers);
    
    // Write a Parcelable object
    MyParcelable parcelable(100);
    parcel.WriteParcelable(&parcelable);
    
    // Reset read position to beginning
    parcel.RewindRead(0);
    
    // Read back the values
    int32_t intValue;
    parcel.ReadInt32(intValue);
    std::cout << "Read int: " << intValue << std::endl;
    
    float floatValue;
    parcel.ReadFloat(floatValue);
    std::cout << "Read float: " << floatValue << std::endl;
    
    std::string strValue = parcel.ReadString();
    std::cout << "Read string: " << strValue << std::endl;
    
    std::vector<int32_t> readNumbers;
    parcel.ReadInt32Vector(&readNumbers);
    std::cout << "Read vector: ";
    for (auto num : readNumbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    
    // Read Parcelable object
    MyParcelable *readParcelable = MyParcelable::Unmarshalling(parcel);
    if (readParcelable) {
        std::cout << "Read parcelable: " << readParcelable->GetValue() << std::endl;
        // delete readParcelable;
    }
    
    return 0;
}
