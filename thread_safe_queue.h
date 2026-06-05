#include <atomic>
#include <thread>

using namespace std;

template <typename T>
class ThreadSafeQueue {
private:
    struct DataSlot {
        atomic<size_t> expected_assignment{0};
        T data;
    };
    size_t capacity{0};
    DataSlot* data_slots;
    atomic<size_t> head;
    atomic<size_t> tail;
public:

    ThreadSafeQueue(const ThreadSafeQueue& tsq) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue& tsq) = delete;

    ThreadSafeQueue(size_t cap) {
        if (cap < 2) throw invalid_argument("Capacity >= 2");
        capacity = cap;
        data_slots = new DataSlot[cap];
        head.store(0);
        tail.store(0);
        for (int i = 0; i < capacity; i++) {
            data_slots[i].expected_assignment = i;
        }
    }

    bool push(const T& in_data) {
        size_t assigned_slot = tail.load();
        do {
            if (data_slots[assigned_slot % capacity].expected_assignment != assigned_slot) {
                return false;
            }
        } while (!tail.compare_exchange_weak(assigned_slot, assigned_slot+1));
        data_slots[assigned_slot % capacity].data = in_data;
        data_slots[assigned_slot % capacity].expected_assignment.store(assigned_slot+1);
        return true;
    }

    bool push(T&& in_data) {
        size_t assigned_slot = tail.load();
        do {
            if (data_slots[assigned_slot % capacity].expected_assignment != assigned_slot) {
                return false;
            }
        } while (!tail.compare_exchange_weak(assigned_slot, assigned_slot+1));
        data_slots[assigned_slot % capacity].data = std::move(in_data);
        data_slots[assigned_slot % capacity].expected_assignment.store(assigned_slot+1);
        return true;
    }

    bool pop(T& out_data) {
        size_t assigned_slot = head.load();
        do {
            if (data_slots[assigned_slot % capacity].expected_assignment != assigned_slot + 1) {
                return false;
            }
        } while (!head.compare_exchange_weak(assigned_slot, assigned_slot+1));
        out_data = std::move(data_slots[assigned_slot % capacity].data);
        data_slots[assigned_slot % capacity].expected_assignment.store(assigned_slot + capacity);
        return true;
    }

    ~ThreadSafeQueue() {
        delete[] data_slots;
    }
};