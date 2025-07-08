#include <utility>

template <typename T> class unique_ptr {
    private:
        T *ptr;

    public:
        // Step1. Rule of 5
        unique_ptr() noexcept : ptr(nullptr) {}
        explicit unique_ptr(T *p) noexcept : ptr(p) {}

        unique_ptr(unique_ptr const &) = delete;
        unique_ptr &operator=(unique_ptr const &) = delete;

        unique_ptr(unique_ptr &&o) noexcept
            : ptr(std::exchange(o.ptr, nullptr)) {}
        unique_ptr &operator=(unique_ptr &&o) noexcept {
            delete ptr;
            ptr = o.ptr;
            o.ptr = nullptr;
            return *this;
        }

        ~unique_ptr() noexcept { delete ptr; }

        // Step2. The most basic of a 'pointer' de-reference
        T &operator*() const noexcept { return *ptr; }
        T *operator->() const noexcept { return ptr; }

        // Step3. Some helper function
        // Abort ownner ship, and return ptr
        T *release() noexcept {
            T *old = ptr;
            ptr = nullptr;
            return old;
        }
        // Like move assignment, but for raw pointer
        void reset(T *p = nullptr) noexcept {
            delete ptr;
            ptr = p;
        }
        // Like ->
        T* get() const noexcept { return ptr; }
        // for `if (ptr)` usage
        explicit operator bool() const noexcept { return ptr != nullptr; }
};
