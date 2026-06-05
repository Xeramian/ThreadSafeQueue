#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include "thread_safe_queue.h"

int main() {

    ThreadSafeQueue<int> tsq{20};

    std::vector<std::thread> tasks;

    int items = 5'000'000;
    std::vector<uint8_t> validation_array(items, 0);

    int num_producer = 80;
    int num_consumer = 20;

    std::atomic<int> push_counter = 0;
    std::atomic<int> pop_counter = 0;

    for (int i = 0; i < num_consumer; i++) {
        tasks.push_back(std::thread{[&](){
            while (pop_counter < items) {
                int q;
                if (tsq.pop(q)) {
                    pop_counter++;
                    validation_array[q] = true;
                }
            }
        }});
    }
    
    for (int i = 0; i < num_producer; i++) {
        tasks.push_back(std::thread{[&, i](){
            while (push_counter < items) {
                int to_push = push_counter.fetch_add(1);
                while (!tsq.push(to_push)) {
                    std::this_thread::yield();
                };
            }
        }});
    }

    for (std::thread& task: tasks) {
        task.join();
    }

    int missing = 0;
    for (int i = 0; i < items; i++) {
        if (validation_array[i] == 0) {
            missing++;
        }
    }

    std::cout << "Lost " << missing << " Elements in 5 million push and pops" << std::endl;
}