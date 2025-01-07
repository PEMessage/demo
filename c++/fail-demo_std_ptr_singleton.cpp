#include <mutex>
#include <memory>
#include <iostream>



class Singleton {
    public:
        static std::shared_ptr<Singleton> GetInstance() {
            if ( instance_ == nullptr ) {
                static std::mutex mutex;
                std::lock_guard<std::mutex> lock(mutex);
                if (instance_ == nullptr) {
                    instance_ = std::make_shared<Singleton>();
                }
            }
            return instance_;
        }
    private:
        static inline std::shared_ptr<Singleton> instance_ = nullptr;
        Singleton() {
            std::cout << "create" << std::endl;
            
        }
        ~Singleton() {
            std::cout << "destroy" << std::endl;
        }
};

int main (int argc, char *argv[]) {
    Singleton::GetInstance();
    return 0;
}
