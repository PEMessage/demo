#include <uv.h>
#include <coroutine>
#include <iostream>

// ─── Task: minimal coroutine return type ───────────────────────────
//
// Every C++ coroutine needs a return type with a nested promise_type.
// initial_suspend = suspend_never -> coroutine starts eagerly.
// final_suspend = suspend_never -> coroutine destroys itself on finish.

struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

// ─── TimerAwaiter ──────────────────────────────────────────────────
//
// The "awaiter" is the object that co_await operates on.
// ┌──────────────┐     ┌─────────────────────┐
// │ await_ready()│─no─▶│ await_suspend(h)    │  … libuv callback …
// │              │     │   save coro handle  │      then
// │              │yes  │   start libuv timer │──▶ coro.resume()
// └──────┬───────┘     └─────────────────────┘
//        ▼
// ┌──────────────┐
// │await_resume()│  (coroutine continues synchronously)
// └──────────────┘

struct TimerAwaiter {
    uv_timer_t *timer = nullptr;

    TimerAwaiter(uv_loop_t *loop, uint64_t timeout_ms) {
        timer = new uv_timer_t;
        uv_timer_init(loop, timer);
        uv_timer_start(timer,
            [](uv_timer_t *t) {
                auto h = std::coroutine_handle<>::from_address(t->data);
                h.resume();  // <-- wake up the suspended coroutine
            },
            timeout_ms, 0 /* non-repeating */);
    }

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        timer->data = h.address(); // stash handle so callback can find it
    }

    void await_resume() {
        uv_close((uv_handle_t *)timer, [](uv_handle_t *h) {
            delete (uv_timer_t *)h;
        });
    }
};

// Factory helper so callers write:  co_await sleep_for(loop, 500);
auto sleep_for(uv_loop_t *loop, uint64_t ms) {
    return TimerAwaiter(loop, ms);
}

// ─── Coroutine demos ───────────────────────────────────────────────

Task countdown(uv_loop_t *loop) {
    for (int i = 3; i > 0; i--) {
        std::cout << i << "..." << std::endl;
        co_await sleep_for(loop, 1000);
    }
    std::cout << "Go!" << std::endl;
}

Task greet(uv_loop_t *loop) {
    co_await sleep_for(loop, 500);
    std::cout << "Hello" << std::endl;
    co_await sleep_for(loop, 1000);
    std::cout << "World" << std::endl;
}

// Two coroutines run concurrently — interleaved by the event loop.
Task concurrent_demo(uv_loop_t *loop) {
    std::cout << "=== concurrent coroutine demo ===" << std::endl;
    countdown(loop);   // starts immediately (initial_suspend is never)
    greet(loop);       // starts immediately
    co_return;          // falls off, but the other two are still running
}

// ─── main ──────────────────────────────────────────────────────────

int main() {
    auto *loop = uv_default_loop();
    concurrent_demo(loop);
    std::cout << "> entering event loop" << std::endl;
    uv_run(loop, UV_RUN_DEFAULT);
    uv_loop_close(loop);
    return 0;
}
