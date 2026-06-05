#include <iostream>
#include <vector>
#include <functional>
#include "thread_safe_queue.h"

using namespace std;

class ThreadPool {
private:
    ThreadSafeQueue<function<void()>> tsq;
    std::atomic<int> jobs{0};
    std::vector<std::thread> threads;
    std::atomic<bool> stop_signal{false};
public:
    ThreadPool(int num_threads): tsq{128} {
        for (int i = 0; i < num_threads; i++) {
            threads.push_back(std::thread{
                [this]() {
                    while (true) {
                        jobs.wait(0);
                        function<void()> fn;
                        if (tsq.pop(fn)) {
                            jobs--;
                            fn();
                        } else if (tsq.len == 0 && stop_signal) {
                            break;
                        }
                    }
                }
            });
        }
    }

    void enque_job(function<void()> fn) {
        while (!tsq.push(fn)) { this_thread::yield(); }
        jobs++;
        jobs.notify_one();
    }

    ~ThreadPool() {
        jobs++;
        stop_signal.store(true);
        for (thread& t: threads) {
            t.join();
        }
    }
};

int main() {
    ThreadPool tp{4};

    for (int i = 0; i < 20000; i++) {
        tp.enque_job([i]() {
            cout << "Job " << i << endl;
        });
    }
}