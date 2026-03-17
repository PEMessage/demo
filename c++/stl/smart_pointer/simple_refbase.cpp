#include <atomic>
#include <functional>
#include <iostream>

// 简化的引用计数器类
class RefCounter {
private:
    std::atomic<int> strongRef_;   // 强引用计数
    std::atomic<int> weakRef_;     // 弱引用计数
    std::function<void()> deleter_; // 删除器

public:
    RefCounter() : strongRef_(1), weakRef_(0) {}

    // 强引用操作
    void IncStrong() {
        strongRef_.fetch_add(1, std::memory_order_relaxed);
    }

    void DecStrong() {
        if (strongRef_.fetch_sub(1, std::memory_order_release) == 1) {
            // 最后一个强引用被释放，删除对象
            if (deleter_) {
                deleter_();
            }
        }
    }

    int GetStrong() const {
        return strongRef_.load(std::memory_order_relaxed);
    }

    // 弱引用操作
    void IncWeak() {
        weakRef_.fetch_add(1, std::memory_order_relaxed);
    }

    void DecWeak() {
        if (weakRef_.fetch_sub(1, std::memory_order_release) == 1) {
            // 最后一个弱引用被释放，删除计数器
            delete this;
        }
    }

    int GetWeak() const {
        return weakRef_.load(std::memory_order_relaxed);
    }

    // 设置删除器
    void SetDeleter(std::function<void()> deleter) {
        deleter_ = deleter;
    }

    // 尝试提升为强引用（弱转强）
    bool AttemptIncStrong() {
        int count = strongRef_.load(std::memory_order_relaxed);
        while (count > 0) {
            if (strongRef_.compare_exchange_weak(count, count + 1,
                                                  std::memory_order_relaxed)) {
                IncWeak(); // 每个强引用隐含一个弱引用
                return true;
            }
        }
        return false;
    }
};

// 简化的基类，所有能被智能指针管理的类都需要继承自它
class RefBase {
private:
    RefCounter* counter_;

public:
    RefBase() : counter_(new RefCounter()) {
        counter_->SetDeleter([this]() { delete this; });
    }

    virtual ~RefBase() {
        if (counter_->GetWeak() == 0) {
            delete counter_;
        }
    }

    // 禁止拷贝
    RefBase(const RefBase&) = delete;
    RefBase& operator=(const RefBase&) = delete;

    // 强引用操作
    void IncStrong() {
        counter_->IncStrong();
        counter_->IncWeak(); // 每个强引用隐含一个弱引用
    }

    void DecStrong() {
        counter_->DecStrong();
        counter_->DecWeak();
    }

    // 弱引用操作
    void IncWeak() {
        counter_->IncWeak();
    }

    void DecWeak() {
        counter_->DecWeak();
    }

    // 获取计数器
    RefCounter* GetCounter() const { return counter_; }

    // 生命周期扩展（简化版）
    virtual void OnLastStrongRef() {}
    virtual void OnFirstStrongRef() {}
};

// 前向声明
template <typename T>
class wptr;

// 强引用智能指针 (sptr)
template <typename T>
class sptr {
private:
    T* ptr_;

public:
    // 默认构造函数
    sptr() : ptr_(nullptr) {}

    // 从原始指针构造
    explicit sptr(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            ptr_->IncStrong();
        }
    }

    // 拷贝构造
    sptr(const sptr<T>& other) : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->IncStrong();
        }
    }

    // 移动构造
    sptr(sptr<T>&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // 析构函数
    ~sptr() {
        if (ptr_) {
            ptr_->DecStrong();
        }
    }

    // 拷贝赋值
    sptr<T>& operator=(const sptr<T>& other) {
        if (this != &other) {
            // 先增加新对象的引用，再减少原对象的引用（防止自赋值问题）
            T* newPtr = other.ptr_;
            if (newPtr) {
                newPtr->IncStrong();
            }
            if (ptr_) {
                ptr_->DecStrong();
            }
            ptr_ = newPtr;
        }
        return *this;
    }

    // 移动赋值
    sptr<T>& operator=(sptr<T>&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                ptr_->DecStrong();
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // 从弱指针提升
    explicit sptr(const wptr<T>& other);

    // 运算符重载
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

    // 获取原始指针
    T* get() const { return ptr_; }

    // 重置
    void reset(T* ptr = nullptr) {
        if (ptr_) {
            ptr_->DecStrong();
        }
        ptr_ = ptr;
        if (ptr_) {
            ptr_->IncStrong();
        }
    }

    // 工厂方法
    template <typename... Args>
    static sptr<T> MakeSptr(Args&&... args) {
        return sptr<T>(new T(std::forward<Args>(args)...));
    }
};

// 弱引用智能指针 (wptr)
template <typename T>
class wptr {
private:
    T* ptr_;
    RefCounter* counter_;

public:
    // 默认构造函数
    wptr() : ptr_(nullptr), counter_(nullptr) {}

    // 从原始指针构造
    wptr(T* ptr) : ptr_(ptr) {
        if (ptr_) {
            counter_ = ptr_->GetCounter();
            counter_->IncWeak();
        } else {
            counter_ = nullptr;
        }
    }

    // 从强指针构造
    wptr(const sptr<T>& sptr) : ptr_(sptr.get()) {
        if (ptr_) {
            counter_ = ptr_->GetCounter();
            counter_->IncWeak();
        } else {
            counter_ = nullptr;
        }
    }

    // 拷贝构造
    wptr(const wptr<T>& other) : ptr_(other.ptr_), counter_(other.counter_) {
        if (counter_) {
            counter_->IncWeak();
        }
    }

    // 移动构造
    wptr(wptr<T>&& other) noexcept
        : ptr_(other.ptr_), counter_(other.counter_) {
        other.ptr_ = nullptr;
        other.counter_ = nullptr;
    }

    // 析构函数
    ~wptr() {
        if (counter_) {
            counter_->DecWeak();
        }
    }

    // 拷贝赋值
    wptr<T>& operator=(const wptr<T>& other) {
        if (this != &other) {
            if (counter_) {
                counter_->DecWeak();
            }
            ptr_ = other.ptr_;
            counter_ = other.counter_;
            if (counter_) {
                counter_->IncWeak();
            }
        }
        return *this;
    }

    // 提升为强指针
    sptr<T> promote() const {
        if (counter_ && counter_->AttemptIncStrong()) {
            return sptr<T>(ptr_);
        }
        return sptr<T>(nullptr);
    }

    // 获取原始指针
    T* get() const { return ptr_; }

    // 检查是否有效
    bool expired() const {
        return !counter_ || counter_->GetStrong() == 0;
    }

    explicit operator bool() const { return ptr_ != nullptr; }
};

// sptr 的弱指针提升构造函数
template <typename T>
inline sptr<T>::sptr(const wptr<T>& other) {
    sptr<T> temp = other.promote();
    ptr_ = temp.ptr_;
    if (ptr_) {
        ptr_->IncStrong(); // 因为 temp 会析构，所以需要增加引用
    }
}

// 使用示例
class MyClass : public RefBase {
public:
    int value;

    explicit MyClass(int v) : value(v) {
        std::cout << "MyClass constructed: " << value << std::endl;
    }

    ~MyClass() {
        std::cout << "MyClass destructed: " << value << std::endl;
    }

    void print() const {
        std::cout << "Value: " << value << std::endl;
    }

    virtual void OnFirstStrongRef() override {
        std::cout << "First strong reference created" << std::endl;
    }

    virtual void OnLastStrongRef() override {
        std::cout << "Last strong reference released" << std::endl;
    }
};

int main() {
    // 测试强指针
    std::cout << "=== Testing sptr ===" << std::endl;
    {
        sptr<MyClass> sp1 = sptr<MyClass>::MakeSptr(42);
        sp1->print();

        sptr<MyClass> sp2 = sp1;  // 拷贝构造
        std::cout << "sp2 use count (via weak counter): "
                  << sp1.get()->GetCounter()->GetStrong() << std::endl;

        sptr<MyClass> sp3;
        sp3 = sp1;  // 拷贝赋值
    }  // 离开作用域时，对象会被自动删除

    // 测试弱指针
    std::cout << "\n=== Testing wptr ===" << std::endl;
    {
        sptr<MyClass> sp = sptr<MyClass>::MakeSptr(100);
        wptr<MyClass> wp(sp);

        std::cout << "Weak pointer created" << std::endl;
        std::cout << "Weak pointer expired? " << wp.expired() << std::endl;

        auto sp2 = wp.promote();  // 提升为强指针
        if (sp2) {
            sp2->print();
        }

        sp.reset();  // 释放强指针

        std::cout << "After reset, weak pointer expired? " << wp.expired() << std::endl;

        auto sp3 = wp.promote();  // 尝试提升已失效的弱指针
        if (!sp3) {
            std::cout << "Promote failed - object already destroyed" << std::endl;
        }
    }  // wp 析构，删除 RefCounter

    return 0;
}
