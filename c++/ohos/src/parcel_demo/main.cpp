#include <iostream>
#include "parcel.h"
#include "xxd.h"

using namespace OHOS;

#define DUMP_PARCEL(parcel) xxd_color2(parcel.GetDataSize(), (unsigned char*)parcel.GetData())

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
    printf("------------------------------------\n");
    printf("Empty parcel\n");
    Parcel parcel;
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");
    


    printf("------------------------------------\n");
    printf("Write Int32: 42\n");
    // Write primitive types
    parcel.WriteInt32(42);
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");



    printf("------------------------------------\n");
    printf("Write float: 3.14f\n");
    parcel.WriteFloat(3.14f);
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");



    printf("------------------------------------\n");
    printf("Write std::string: Hello, Parcel!\n");
    printf("| size(4b) + data(14) + padding(2b)\n");
    parcel.WriteString("Hello, Parcel!");
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");
    
    printf("------------------------------------\n");
    printf("Write std::string: Hello, Parcel!xx\n");
    printf("| size(4b) + data(16b) + padding(4b), must have padding for string event if already align\n");
    parcel.WriteString("Hello, Parcel!xx");
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");


    // Write a vector
    printf("------------------------------------\n");
    printf("Write vector<int32_t>: 1 - 8\n");
    std::vector<int32_t> numbers = {1, 2, 3, 4, 5};
    parcel.WriteInt32Vector(numbers);
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");
    


    // Write a Parcelable object
    printf("------------------------------------\n");
    printf("Write MyParcelable: 100\n");
    MyParcelable parcelable(100);
    parcel.WriteParcelable(&parcelable);
    DUMP_PARCEL(parcel);
    printf("------------------------------------\n\n\n");
    
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
    

    std::string strValue2 = parcel.ReadString();
    std::cout << "Read string: " << strValue2 << std::endl;

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
