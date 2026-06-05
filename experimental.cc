#include <iostream>
#include <vector>
#include <functional>
#include <semaphore>
#include "thread_safe_queue.h"

class ThreadPool {
private:
    ThreadSafeQueue<std::function<void()>> tsq;
    std::counting_semaphore<> jobs{0};
    std::vector<std::thread> threads;
    std::atomic<bool> stop_signal{false};
public:
    ThreadPool(int num_threads): tsq{128} {
        for (int i = 0; i < num_threads; i++) {
            threads.push_back(std::thread{
                [this]() {
                    while (true) {
                        jobs.acquire();
                        std::function<void()> fn;
                        while (!tsq.pop(fn)) { 
                            if (tsq.len == 0 && stop_signal) {
                                return;
                            }
                            std::this_thread::yield();
                        }
                        fn();
                    }
                }
            });
        }
    }

    void enque_job(std::function<void()> fn) {
        if (stop_signal) return;
        while (!tsq.push(fn)) { std::this_thread::yield(); }
        jobs.release();
    }

    ~ThreadPool() {
        stop_signal.store(true);
        jobs.release(threads.size());
        for (std::thread& t: threads) {
            t.join();
        }
    }
};

int main() {
    ThreadPool tp{4};

    for (int i = 0; i < 20000; i++) {
        tp.enque_job([i]() {
            std::cout << "Job " << i << std::endl;
        });
    }
}