#include <iostream>
#include <vector>
#include "thread_safe_queue.h"

int main() {
    ThreadSafeQueue<int> tsq{2};
    int t = 32;
    int k = 1000;
    vector<bool> completed(k, false);
    vector<thread> tasks;
    
    for (int i = 0; i < t; i++) {
        tasks.push_back(thread{
            [i, &tsq, t, k]() {
                for (int q = i; q < k; q += t) {
                    while (!tsq.push(q)) {
                        this_thread::yield();
                    }
                    // cout << "Pushed " << i + 1 << endl;
                }
            }
        });
    }

    for (int i = 0; i < t; i++) {
        tasks.push_back(thread{
            [&, i, t]() {
                for (int q_ = i; q_ < k; q_ += t) {
                    int q = 0;
                    while (!tsq.pop(q)) {
                        this_thread::yield();
                    }
                    completed[q] = true;
                    // cout << "Popped " << q << endl;
                }
            }
        });
    }

    for (thread& task: tasks) {
        task.join();
    }

    for (int i = 0; i < k; i++) {
        cout << "Completed " << i << ": " << completed[i] << endl;
    }
}