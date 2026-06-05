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
    int items = 5'000'000;
    std::vector<uint8_t> validation_array(items, 0);

    {
        ThreadPool tp{4};

        for (int i = 0; i < items; i++) {
            tp.enque_job([i, &validation_array]() {
                validation_array[i] = 1;
            });
        }
    }

    int missing = 0;
    for (int i = 0; i < items; i++) {
        if (validation_array[i] == 0) {
            missing++;
        }
    }

    std::cout << "Lost " << missing << " Elements in 5 million push and pops" << std::endl;
}