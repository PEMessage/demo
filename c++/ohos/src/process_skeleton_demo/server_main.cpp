#include <iostream>
#include <string>
#include "process_skeleton.h"

using namespace OHOS;

class TestRemoteObject : public IRemoteObject {
public:
    TestRemoteObject(): IRemoteObject(u"test.descriptor") {}
    ~TestRemoteObject() override = default;
    
    int32_t GetObjectRefCount() override { return 1; }
    int SendRequest(uint32_t code, MessageParcel &data, 
                   MessageParcel &reply, MessageOption &option) override {
        return 0;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override {
        std::cout << "RemoveDeathRecipient called" << std::endl;
        return true;
    }
    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override {
        std::cout << "AddDeathRecipient called" << std::endl;
        return true;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override { 
        return 0;
    }
};

int main() {
    std::cout << "ProcessSkeleton API Demo" << std::endl;
    
    // Get the singleton instance
    ProcessSkeleton* processSkeleton = ProcessSkeleton::GetInstance();
    if (processSkeleton == nullptr) {
        std::cerr << "Failed to get ProcessSkeleton instance" << std::endl;
        return -1;
    }
    
    // Demo 1: Object management
    std::cout << "\n1. Object management demo:" << std::endl;
    sptr<TestRemoteObject> testObject = new TestRemoteObject();
    std::u16string descriptor = u"test.descriptor";
    
    // Attach object
    if (processSkeleton->AttachObject(testObject.GetRefPtr(), descriptor, true)) {
        std::cout << "Object attached successfully" << std::endl;
    }
    
    // Query object
    sptr<IRemoteObject> queriedObject = processSkeleton->QueryObject(descriptor, true);
    if (queriedObject != nullptr) {
        std::cout << "Object queried successfully" << std::endl;
    }
    
    // Check if contains object
    if (processSkeleton->IsContainsObject(testObject.GetRefPtr())) {
        std::cout << "Object is contained in skeleton" << std::endl;
    }
    
    // Demo 2: Registry object
    std::cout << "\n2. Registry object demo:" << std::endl;
    sptr<IRemoteObject> registryObj = processSkeleton->GetRegistryObject();
    if (registryObj == nullptr) {
        std::cout << "No registry object set" << std::endl;
    }
    
    // Demo 3: Invoker process info
    std::cout << "\n3. Invoker process info demo:" << std::endl;
    InvokerProcInfo localInfo = {123, 123, 1000, 9999, 8888, "test_sid", 1};
    if (processSkeleton->AttachInvokerProcInfo(true, localInfo)) {
        std::cout << "Local invoker info attached" << std::endl;
    }
    
    InvokerProcInfo queriedInfo;
    if (processSkeleton->QueryInvokerProcInfo(true, queriedInfo)) {
        std::cout << "Queried local invoker PID: " << queriedInfo.pid << std::endl;
    }
    
    // Demo 4: Thread management
    std::cout << "\n4. Thread management demo:" << std::endl;
    std::cout << "Thread stop flag: " << processSkeleton->GetThreadStopFlag() << std::endl;
    
    // Demo 5: IPC proxy limit
    std::cout << "\n5. IPC proxy limit demo:" << std::endl;
    auto callback = [](uint64_t num) {
        std::cout << "IPC proxy limit reached: " << num << std::endl;
    };
    
    if (processSkeleton->SetIPCProxyLimit(1000, callback)) {
        std::cout << "IPC proxy limit set to 1000" << std::endl;
    }
    
    // Cleanup
    processSkeleton->DetachObject(testObject.GetRefPtr(), descriptor);
    processSkeleton->DetachInvokerProcInfo(true);
    
    std::cout << "\nDemo completed successfully!" << std::endl;
    return 0;
}
