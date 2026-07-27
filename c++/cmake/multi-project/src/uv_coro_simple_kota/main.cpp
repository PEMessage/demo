#include <uv.h>
#include <coroutine>
#include <cstdio>
#include <utility>

// ============================================================
//  Task — minimal coroutine return type
// ============================================================
struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};

// ============================================================
//  io_op — the resume primitive
// ============================================================
struct io_op {
    std::coroutine_handle<> parent;
    void complete() { parent.resume(); }
};

// ============================================================
//  event_loop — owns the uv_loop_t, thread-local singleton
// ============================================================
class event_loop {
    uv_loop_t loop_{};
    static thread_local event_loop *current_;

public:
    event_loop()  { uv_loop_init(&loop_); current_ = this; }
    ~event_loop() { uv_loop_close(&loop_); current_ = nullptr; }

    event_loop(const event_loop&) = delete;
    event_loop& operator=(const event_loop&) = delete;

    static event_loop& current() { return *current_; }

    int run()  { return uv_run(&loop_, UV_RUN_DEFAULT); }
    void stop() { uv_stop(&loop_); }

    // Allow passing to uv_* functions:  uv_timer_init(loop, ...)
    operator uv_loop_t*() { return &loop_; }
};

thread_local event_loop *event_loop::current_ = nullptr;

// ============================================================
//  Timer — reusable timer watcher
// ============================================================
struct Timer {
    uv_timer_t handle{};
    io_op *waiter = nullptr;
    int pending = 0;

    Timer(event_loop &loop) { uv_timer_init(loop, &handle); }

    void start(uint64_t ms) {
        pending = 0;
        uv_timer_start(&handle, on_fire, ms, 0);
    }
    void stop() { uv_timer_stop(&handle); }

    static void on_fire(uv_timer_t *h) {
        auto *self = static_cast<Timer *>(h->data);
        if (self->waiter) {
            auto *w = std::exchange(self->waiter, nullptr);
            w->complete();
        } else {
            self->pending += 1;
        }
    }
};

struct TimerAwait : io_op {
    Timer *timer;
    explicit TimerAwait(Timer *t) : timer(t) {}

    bool await_ready() noexcept { return timer && timer->pending > 0; }

    void await_suspend(std::coroutine_handle<> h) noexcept {
        parent = h;
        timer->waiter = this;
        timer->handle.data = timer;
    }

    void await_resume() noexcept {
        if (timer && timer->pending > 0) timer->pending -= 1;
        if (timer) timer->waiter = nullptr;
    }
};

// ============================================================
//  sleep_for — convenience one-shot timer
// ============================================================
auto sleep_for(event_loop &loop, uint64_t ms) {
    struct Awaiter : io_op {
        uv_timer_t *timer;

        Awaiter(event_loop &l, uint64_t m) {
            timer = new uv_timer_t;
            uv_timer_init(l, timer);
            uv_timer_start(timer,
                [](uv_timer_t *t) {
                    static_cast<Awaiter *>(t->data)->complete();
                }, m, 0);
        }

        bool await_ready() { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            parent = h;
            timer->data = this;
        }

        void await_resume() {
            uv_close((uv_handle_t *)timer,
                [](uv_handle_t *h) { delete (uv_timer_t *)h; });
        }
    };
    return Awaiter(loop, ms);
}

// ============================================================
//  Coroutine demos
// ============================================================

Task countdown(event_loop &loop) {
    for (int i = 3; i > 0; i--) {
        printf("%d...\n", i);
        co_await sleep_for(loop, 1000);
    }
    printf("Go!\n");
}

Task greet(event_loop &loop) {
    co_await sleep_for(loop, 500);
    printf("Hello\n");
    co_await sleep_for(loop, 1000);
    printf("World\n");
}

Task reusable_demo(event_loop &loop) {
    printf("=== reusable timer ===\n");
    Timer t(loop);

    t.start(700);
    co_await TimerAwait(&t);
    printf("first tick\n");

    t.start(300);
    co_await TimerAwait(&t);
    printf("second tick (same timer)\n");
}

// ============================================================
//  main
// ============================================================

int main() {
    event_loop loop;

    printf("=== concurrent ===\n");
    countdown(loop);
    greet(loop);

    reusable_demo(loop);

    printf("> entering loop\n");
    loop.run();
    return 0;
}
