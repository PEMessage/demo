#include <iostream>
#include <vector>
#include <chrono>

// 操作基类
// 虚表：每个类一个虚表。寻址时 this -> 虚表 -> 虚函数
// 此方法：每个对象一个函数指针。寻址时 this -> 函数指针，少了一次寻址
struct Operation {
    using func_type = void(*)(Operation*); 
    func_type func_; // 保存普通的函数指针
    Operation* next_; // next这个是asio中使用的，同时需要处理的异步调用可能有多个，有next就可以形成一个链表
    
    Operation(func_type func) : func_(func), next_(nullptr) {}
    
    void perform() {
        func_(this);
    }
};

struct VirtualOperation {
    virtual void perform_virtual() = 0;
};


// 第一个具体操作
class TimerOperation : public Operation, public VirtualOperation {
public:
    TimerOperation() : Operation(&TimerOperation::impl) {}

    void start_timer() {
        std::cout << "TimerOperation: 3 seconds elapsed\n";
    }

    void perform_virtual() {
        start_timer();
    }

private:
    // static函数可以保存为普通的函数指针
    static void impl(Operation* op) {
        static_cast<TimerOperation*>(op)->start_timer();
    }
};

// 第二个具体操作
class NetworkOperation : public Operation, public VirtualOperation {
public:
    NetworkOperation() : Operation(&NetworkOperation::impl) {}

    void send_data() {
        std::cout << "NetworkOperation: Data sent successfully\n";
    }

    void perform_virtual() {
        send_data();
    }

private:
    static void impl(Operation* op) {
        static_cast<NetworkOperation*>(op)->send_data();
    }
};


template<typename Func, typename FuncArgT>
std::string benchmark(const std::string& name,
               Func func,
               const std::vector<FuncArgT*> &operations) {
    auto start = std::chrono::high_resolution_clock::now();
    func(operations);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return  name + " took " +  std::to_string(duration.count()) + " us";
}

void test_func_pointer(const std::vector<Operation*> &operations) {
    for ( auto x : operations ) {
        x->perform();
    }
}

void test_virtual_func(const std::vector<VirtualOperation*> &operations) {
    for ( auto x : operations ) {
        x->perform_virtual();
    }
}



int main() {
    // 创建不同类型的操作对象
    std::vector<Operation*>        operations;
    std::vector<VirtualOperation*> voperations;
    for ( int i = 0 ; i < 50000 ; i++ ) {
        if ( i % 2 == 0 ) {
            operations.push_back(new TimerOperation());
            voperations.push_back(new TimerOperation());
        } else {
            operations.push_back(new NetworkOperation());
            voperations.push_back(new NetworkOperation());
        }
    }

    std::string result1 = benchmark("Func pointer  ", test_func_pointer, operations);
    std::string result2 = benchmark("Virt Function ", test_virtual_func, voperations);

    std::cout << result1 << std::endl;
    std::cout << result2 << std::endl;

    return 0;
}

