#include <uv.h>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <utility>
#include <vector>

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
//  schedule() queues tasks; an idle watcher resumes them during
//  the event loop's idle phase — same as kota.

struct Task;

class event_loop {
    uv_loop_t loop_{};
    uv_idle_t idle_{};
    std::vector<std::coroutine_handle<>> tasks_;
    bool idle_running_ = false;
    static thread_local event_loop *current_;

public:
    event_loop() {
        uv_loop_init(&loop_);
        uv_idle_init(&loop_, &idle_);
        idle_.data = this;
        current_ = this;
    }

    ~event_loop() { uv_loop_close(&loop_); current_ = nullptr; }

    event_loop(const event_loop&) = delete;
    event_loop& operator=(const event_loop&) = delete;

    static event_loop& current() { return *current_; }

    int run()  { return uv_run(&loop_, UV_RUN_DEFAULT); }
    void stop() { uv_stop(&loop_); }

    operator uv_loop_t*() { return &loop_; }

    // ── schedule ────────────────────────────────────────────

    void schedule(Task &&t);
};

thread_local event_loop *event_loop::current_ = nullptr;

// ============================================================
//  Task — lazy, awaitable coroutine return type
// ============================================================
struct Task {
    struct promise_type;
    using handle_t = std::coroutine_handle<promise_type>;

    handle_t coro;

    Task(handle_t h) : coro(h) {}
    Task(Task &&o) : coro(std::exchange(o.coro, nullptr)) {}
    ~Task() { if (coro) coro.destroy(); }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // ── co_await Task ───────────────────────────────────────
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> caller) {
        coro.promise().parent = caller;
        coro.resume();
    }

    void await_resume() {}

    // ── promise_type ────────────────────────────────────────
    struct promise_type {
        std::coroutine_handle<> parent;

        Task get_return_object() {
            return Task{handle_t::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }

        auto final_suspend() noexcept {
            struct FinalAwaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<>
                await_suspend(handle_t h) noexcept {
                    auto parent = h.promise().parent;
                    if (!parent) {
                        h.destroy();
                        return std::noop_coroutine();
                    }
                    return parent;
                }
                void await_resume() noexcept {}
            };
            return FinalAwaiter{};
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

// ============================================================
//  schedule (out-of-line — uses Task definition above)
// ============================================================

void event_loop::schedule(Task &&t) {
    auto h = static_cast<std::coroutine_handle<>>(t.coro);
    t.coro = nullptr;  // release ownership

    if (!idle_running_) {
        idle_running_ = true;
        uv_idle_start(&idle_,
            [](uv_idle_t *idle) {
                auto *self = static_cast<event_loop *>(idle->data);

                while (!self->tasks_.empty()) {
                    auto batch = std::move(self->tasks_);
                    self->tasks_.clear();
                    for (auto h : batch) {
                        h.resume();
                    }
                }

                uv_idle_stop(idle);
                self->idle_running_ = false;
            });
    }

    tasks_.push_back(h);
}

// ============================================================
//  Timer — reusable timer watcher
// ============================================================
struct Timer {
    uv_timer_t handle{};
    io_op *waiter = nullptr;
    int pending = 0;

    Timer(event_loop &loop) { uv_timer_init(loop, &handle); }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    void start(uint64_t ms) {
        pending = 0;
        uv_timer_start(&handle, on_fire, ms, 0);
    }
    void stop() { uv_timer_stop(&handle); }

    void close_and_delete() {
        uv_close((uv_handle_t *)&handle, [](uv_handle_t *h) {
            delete static_cast<Timer *>(h->data);
        });
    }

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

// ============================================================
//  TimerAwait
// ============================================================
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
//  sleep_for
// ============================================================
auto sleep_for(event_loop &loop, uint64_t ms) {
    struct Awaiter : TimerAwait {
        Awaiter(event_loop &l, uint64_t m)
            : TimerAwait(new Timer(l)) {
            timer->start(m);
        }
        void await_resume() noexcept {
            TimerAwait::await_resume();
            timer->close_and_delete();
        }
    };
    return Awaiter(loop, ms);
}

// ============================================================
//  sub-task demo — co_await a Task
// ============================================================
Task say_hello(event_loop &loop) {
    co_await sleep_for(loop, 300);
    printf("Hello from sub-task!\n");
}

Task sequential_demo(event_loop &loop) {
    printf("=== sequential co_await ===\n");
    co_await say_hello(loop);
    co_await say_hello(loop);
    printf("done\n");
}

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
    loop.schedule(countdown(loop));
    loop.schedule(greet(loop));
    loop.schedule(sequential_demo(loop));
    loop.schedule(reusable_demo(loop));

    printf("> entering loop\n");
    loop.run();
    return 0;
}
