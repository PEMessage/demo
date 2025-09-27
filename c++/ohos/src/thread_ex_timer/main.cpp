#include <iostream>
#include <unistd.h>
#include <queue>
#include <functional>
#include <map>
#include <atomic>
#include <chrono>
#include "thread_ex.h"

using namespace OHOS;
using namespace std;

// Timer callback function type
using TimerCallback = function<void()>;

// Timer information structure
struct TimerInfo {
    int timerId;
    int intervalMs;
    TimerCallback callback;
    bool repeat;
    chrono::steady_clock::time_point nextTrigger;
};

// API message types
enum class TimerMessageType {
    CREATE_TIMER,
    STOP_TIMER,
    EXIT_DAEMON
};

// Message structure for thread communication
struct TimerMessage {
    TimerMessageType type;
    int timerId;
    int intervalMs;
    TimerCallback callback;
    bool repeat;
};

// Timer Daemon Thread class
class TimerDaemonThread : public Thread {
private:
    queue<TimerMessage> msgQueue;
    map<int, TimerInfo> timers;
    mutex queueMutex;
    mutex timerMutex;
    condition_variable queueCV;
    atomic<int> nextTimerId;
    atomic<bool> shutdownRequested;

public:
    TimerDaemonThread() : nextTimerId(1), shutdownRequested(false) {}

protected:
    bool Run() override {
        ProcessMessageQueue();
        ProcessTimers();
        return !shutdownRequested;
    }

    bool ReadyToWork() override {
        cout << "TimerDaemonThread is ready to work!" << endl;
        return true;
    }

    void NotifyExitAsync() override {
        shutdownRequested = true;
        // Send exit message to wake up the queue
        SendMessage(TimerMessageType::EXIT_DAEMON);
    }

private:
    void ProcessMessageQueue() {
        unique_lock<mutex> lock(queueMutex);

        if (msgQueue.empty()) {
            // Wait for message with timeout to allow timer processing
            queueCV.wait_for(lock, chrono::milliseconds(100));
            return;
        }

        while (!msgQueue.empty()) {
            TimerMessage msg = msgQueue.front();
            msgQueue.pop();
            lock.unlock();

            HandleMessage(msg);

            lock.lock();
        }
    }

    void HandleMessage(const TimerMessage& msg) {
        switch (msg.type) {
            case TimerMessageType::CREATE_TIMER:
                CreateTimerInternal(msg.timerId, msg.intervalMs, msg.callback, msg.repeat);
                break;

            case TimerMessageType::STOP_TIMER:
                StopTimerInternal(msg.timerId);
                break;

            case TimerMessageType::EXIT_DAEMON:
                shutdownRequested = true;
                break;
        }
    }

    void ProcessTimers() {
        auto now = chrono::steady_clock::now();
        vector<int> timersToTrigger;

        {
            lock_guard<mutex> lock(timerMutex);

            for (auto& pair : timers) {
                if (now >= pair.second.nextTrigger) {
                    timersToTrigger.push_back(pair.first);

                    if (pair.second.repeat) {
                        // Schedule next trigger for repeating timers
                        pair.second.nextTrigger = now + chrono::milliseconds(pair.second.intervalMs);
                    } else {
                        // One-shot timer will be removed after triggering
                        pair.second.nextTrigger = chrono::steady_clock::time_point::max();
                    }
                }
            }
        }

        // Trigger callbacks outside the lock
        for (int timerId : timersToTrigger) {
            TriggerTimer(timerId);
        }

        // Clean up expired one-shot timers
        CleanupExpiredTimers();
    }

    void TriggerTimer(int timerId) {
        TimerCallback callback;
        bool isOneShot = false;

        {
            lock_guard<mutex> lock(timerMutex);
            auto it = timers.find(timerId);
            if (it != timers.end()) {
                callback = it->second.callback;
                isOneShot = !it->second.repeat;
            }
        }

        if (callback) {
            try {
                cout << "Triggering timer " << timerId << endl;
                callback();
            } catch (const exception& e) {
                cerr << "Timer callback error: " << e.what() << endl;
            }
        }

        if (isOneShot) {
            RemoveTimer(timerId);
        }
    }

    void CleanupExpiredTimers() {
        auto now = chrono::steady_clock::now();
        vector<int> timersToRemove;

        {
            lock_guard<mutex> lock(timerMutex);
            for (const auto& pair : timers) {
                if (!pair.second.repeat &&
                    pair.second.nextTrigger == chrono::steady_clock::time_point::max()) {
                    timersToRemove.push_back(pair.first);
                }
            }
        }

        for (int timerId : timersToRemove) {
            RemoveTimer(timerId);
        }
    }

    void CreateTimerInternal(int timerId, int intervalMs, const TimerCallback& callback, bool repeat) {
        lock_guard<mutex> lock(timerMutex);

        TimerInfo info;
        info.timerId = timerId;
        info.intervalMs = intervalMs;
        info.callback = callback;
        info.repeat = repeat;
        info.nextTrigger = chrono::steady_clock::now() + chrono::milliseconds(intervalMs);

        timers[timerId] = info;
        cout << "Timer " << timerId << " created: interval=" << intervalMs
             << "ms, repeat=" << repeat << endl;
    }

    void StopTimerInternal(int timerId) {
        lock_guard<mutex> lock(timerMutex);
        auto it = timers.find(timerId);
        if (it != timers.end()) {
            timers.erase(it);
            cout << "Timer " << timerId << " stopped" << endl;
        } else {
            cout << "Timer " << timerId << " not found" << endl;
        }
    }

    void RemoveTimer(int timerId) {
        lock_guard<mutex> lock(timerMutex);
        timers.erase(timerId);
        cout << "Timer " << timerId << " removed" << endl;
    }

    void SendMessage(TimerMessageType type, int timerId = -1, int intervalMs = 0,
                    const TimerCallback& callback = nullptr, bool repeat = false) {
        lock_guard<mutex> lock(queueMutex);

        TimerMessage msg;
        msg.type = type;
        msg.timerId = timerId;
        msg.intervalMs = intervalMs;
        msg.callback = callback;
        msg.repeat = repeat;

        msgQueue.push(msg);
        queueCV.notify_one();
    }

public:
    // Public API methods (thread-safe)
    int CreateTimer(int intervalMs, const TimerCallback& callback, bool repeat = true) {
        int timerId = nextTimerId++;
        SendMessage(TimerMessageType::CREATE_TIMER, timerId, intervalMs, callback, repeat);
        return timerId;
    }

    void StopTimer(int timerId) {
        SendMessage(TimerMessageType::STOP_TIMER, timerId);
    }

    void Shutdown() {
        NotifyExitAsync();
    }

    int GetActiveTimerCount() {
        lock_guard<mutex> lock(timerMutex);
        return timers.size();
    }
};

// Demo usage
int main() {
    cout << "=== Timer Daemon Thread Demo ===" << endl;

    TimerDaemonThread timerDaemon;

    // Start the timer daemon thread
    ThreadStatus status = timerDaemon.Start("TimerDaemon");
    if (status != ThreadStatus::OK) {
        cerr << "Failed to start timer daemon: " << static_cast<int>(status) << endl;
        return -1;
    }

    // Give the daemon time to start
    sleep(1);

    cout << "\n1. Creating repeating timer (every 1 second)" << endl;
    int timer1 = timerDaemon.CreateTimer(1000, []() {
        static int count = 0;
        cout << "Repeating timer triggered! Count: " << ++count << endl;
    }, true);

    cout << "2. Creating one-shot timer (after 3 seconds)" << endl;
    int timer2 = timerDaemon.CreateTimer(3000, []() {
        cout << "One-shot timer triggered! This should only happen once." << endl;
    }, false);

    cout << "3. Creating fast repeating timer (every 500ms)" << endl;
    int timer3 = timerDaemon.CreateTimer(500, []() {
        static int fastCount = 0;
        cout << "Fast timer: " << ++fastCount << endl;
    }, true);

    // Let timers run for 5 seconds
    cout << "\nLetting timers run for 5 seconds..." << endl;
    sleep(5);

    cout << "\n4. Stopping the fast timer" << endl;
    timerDaemon.StopTimer(timer3);

    cout << "Active timers: " << timerDaemon.GetActiveTimerCount() << endl;

    // Let timers run for another 3 seconds
    cout << "\nLetting remaining timers run for 3 seconds..." << endl;
    sleep(3);

    cout << "\n5. Creating another one-shot timer" << endl;
    int timer4 = timerDaemon.CreateTimer(2000, []() {
        cout << "Final one-shot timer triggered!" << endl;
    }, false);

    // Wait for the final timer
    sleep(3);

    cout << "\n6. Demonstrating timer with captured variables" << endl;
    string userName = "TimerUser";
    int userCounter = 0;

    int timer5 = timerDaemon.CreateTimer(1500, [&userName, &userCounter]() {
        cout << "Hello " << userName << "! Counter: " << ++userCounter << endl;
    }, true);

    sleep(4);

    cout << "\n7. Shutting down timer daemon..." << endl;
    timerDaemon.Shutdown();

    // Wait for daemon to shutdown
    int waitCount = 0;
    while (timerDaemon.IsRunning() && waitCount < 10) {
        cout << "Waiting for daemon to shutdown..." << endl;
        sleep(1);
        waitCount++;
    }

    if (!timerDaemon.IsRunning()) {
        cout << "Timer daemon shutdown successfully" << endl;
    } else {
        cerr << "Timer daemon didn't shutdown properly" << endl;
        timerDaemon.NotifyExitSync();
    }

    cout << "\n=== Demo completed ===" << endl;
    return 0;
}
