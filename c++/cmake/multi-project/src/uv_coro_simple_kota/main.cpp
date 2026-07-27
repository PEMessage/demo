#include <uv.h>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <utility>

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

    operator uv_loop_t*() { return &loop_; }
};

thread_local event_loop *event_loop::current_ = nullptr;

// ============================================================
//  Task — lazy, awaitable coroutine return type
// ============================================================
//
//  initial_suspend = always  → coroutine doesn't start until
//    it is either co_await'ed or schedule()'d.
//
//  final_suspend resumes the parent (co_await case) or
//  self-destructs (root / scheduled case).

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
        coro.promise().parent = caller;  // tell child who to wake
        coro.resume();                   // start the child
    }

    void await_resume() {
        // child finished normally (void return)
    }

    // ── promise_type ────────────────────────────────────────
    struct promise_type {
        std::coroutine_handle<> parent;  // who to resume when we finish

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
                        h.destroy();  // root / scheduled: self-destruct
                        return std::noop_coroutine();
                    }
                    return parent;    // co_await child: resume parent
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
//  event_loop::schedule — launch a root / concurrent task
// ============================================================
//  Takes ownership, releases the Task so its destructor is a
//  no-op.  The coroutine self-destructs on completion.

void schedule(Task &&t) {
    auto h = t.coro;
    t.coro = nullptr;  // release — ~Task() won't destroy
    h.resume();         // start (initial_suspend → body)
}

// ============================================================
//  Timer — reusable timer watcher (embedded uv_timer_t)
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
    co_await say_hello(loop);   // wait for sub-task to finish
    co_await say_hello(loop);   // then run it again
    printf("done\n");
}

// ============================================================
//  concurrent demo — schedule() fires-and-forgets
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

    // concurrent — fire-and-forget via schedule()
    printf("=== concurrent ===\n");
    // schedule(countdown(loop));
    // schedule(greet(loop));

    // sequential — co_await a sub-task
    schedule(sequential_demo(loop));

    // reusable timer — also scheduled
    // schedule(reusable_demo(loop));

    printf("> entering loop\n");
    loop.run();
    return 0;
}
