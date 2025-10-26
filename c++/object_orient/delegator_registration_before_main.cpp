#include <iostream>
#include <map>

// Forward declaration
class Delegator;

class Registration {
private:
    static std::map<Delegator*, int> registry;

public:
    static void registerDelegator(Delegator* delegator, int value) {
        registry[delegator] = value;
    }

    static void deregisterDelegator(Delegator* delegator) {
        registry.erase(delegator);
    }

    static void printAll() {
        std::cout << "Registry contents:" << std::endl;
        for (const auto& pair : registry) {
            std::cout << "Delegator*: " << pair.first << " -> Value: " << pair.second << std::endl;
        }
        if (registry.empty()) {
            std::cout << "Registry is empty" << std::endl;
        }
    }

    // Static method to get the size for verification
    static size_t size() {
        return registry.size();
    }
};

// Define the static member
std::map<Delegator*, int> Registration::registry;

class Delegator {
private:
    int value;

public:
    Delegator(int val) : value(val) {
        Registration::registerDelegator(this, value);
        std::cout << "Delegator " << this << " registered with value: " << value << std::endl;
    }

    ~Delegator() {
        std::cout << "Delegator " << this << " with value: " << value << " being destroyed" << std::endl;
        Registration::deregisterDelegator(this);
    }

    int getValue() const { return value; }
};

// Create 2 static instances before main, C++ will run it's constructor before main
static Delegator delegator1(100);
static Delegator delegator2(200);

int main() {
    std::cout << "=== Entering main() ===" << std::endl;

    // Print initial state
    Registration::printAll();
    std::cout << "Registry size: " << Registration::size() << std::endl;

    std::cout << "\n=== Creating local delegator ===" << std::endl;
    {
        Delegator localDelegator(300);
        Registration::printAll();
        std::cout << "Registry size: " << Registration::size() << std::endl;
    }

    std::cout << "\n=== After local delegator destroyed ===" << std::endl;
    Registration::printAll();
    std::cout << "Registry size: " << Registration::size() << std::endl;

    std::cout << "\n=== Exiting main() ===" << std::endl;
    return 0;
}
