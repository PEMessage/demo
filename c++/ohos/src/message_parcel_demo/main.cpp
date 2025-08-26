#include "message_parcel.h"
#include <iostream>
#include <string>

using namespace OHOS;

int main() {
    // Create a MessageParcel
    MessageParcel parcel;
    
    // Write some basic data types
    parcel.WriteInt32(42);
    parcel.WriteString16(u"Hello, IPC_SINGLE!");
    parcel.WriteBool(true);
    
    // Write interface token
    parcel.WriteInterfaceToken(u"test.interface.token");
    
    // Write no exception
    parcel.WriteNoException();
    
    // Reset read position to beginning
    parcel.RewindRead(0);
    
    // Read back the data
    int32_t value = parcel.ReadInt32();
    std::u16string str = parcel.ReadString16();
    bool flag = parcel.ReadBool();
    
    // Read interface token
    std::u16string token = parcel.ReadInterfaceToken();
    
    // Read exception (should be 0 for no exception)
    int32_t exception = parcel.ReadException();
    
    // Print results
    std::cout << "Value: " << value << std::endl;
    std::cout << "String: " << std::string(str.begin(), str.end()) << std::endl;
    std::cout << "Flag: " << std::boolalpha << flag << std::endl;
    std::cout << "Token: " << std::string(token.begin(), token.end()) << std::endl;
    std::cout << "Exception: " << exception << std::endl;
    
    return 0;
}
