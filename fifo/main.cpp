/*
 * From chat:
 * FIFO Buffer Exercise
 *
 * 1) Implement a fixed-size FIFO (ring buffer).
 *    - No dynamic allocation
 *    - O(1) push/pop
 *    - push fails if full, pop fails if empty
 *
 * 2) Make the FIFO thread-safe.
 *    - push/pop may be called from multiple threads
 *    - No busy-waiting (use mutex/condvar or RTOS primitives)
 *
 * 3) Make the FIFO ISR-safe.
 *    - ISR may push while a task pops
 *    - No blocking or mutex use in ISR
 *    - Must be safe under preemption
 *
 * Be ready to explain full/empty handling and race prevention.
 */

#include <iostream>
#include <string>
#include <array>
#include <semaphore>
#include <stdexcept>

template<typename T, size_t N>
class Fifo {
public:
    Fifo() : head_(0), tail_(0), size_(0) {}

    bool push(T item) {
        if (size_ == N) {
            return false;
        }
        arr_[tail_++] = item;
        if (tail_ == N) { tail_ = 0; }
        size_++;
        return true;
    }

    bool pop(T& out) {
        if (size_ == 0) {
            return false;
        }
        size_--;
        out = arr_[head_++];
        if (head_ == N) { head_ = 0; }
        return true;
    }


    size_t size() { return size_; }
    bool empty() { return size_ == 0; }
    bool full() { return size_ == N; }

private:
    std::array<T, N> arr_;
    size_t head_; // front element
    size_t tail_; // insert at tail_ (one past last element)
    size_t size_;
    std::mutex mtx_;
    std::counting_semaphore<N> full_{0}; // num full spaces
    std::counting_semaphore<N> empty_{N};
};

void testcase1();

int main() {
    testcase1();
    return 0;
}

void testcase1() {
    Fifo<int, 5> fifo;
    for (int i = 0; i < 5; i++) {
        fifo.push(i);
    }
    int out;
    if (fifo.pop(out); out != 0) { std::cerr << "failed!" << std::endl; }
    if (fifo.pop(out); out != 1) { std::cerr << "failed!" << std::endl; }
    if (fifo.pop(out); out != 2) { std::cerr << "failed!" << std::endl; }
    if (fifo.pop(out); out != 3) { std::cerr << "failed!" << std::endl; }
    if (fifo.pop(out); out != 4) { std::cerr << "failed!" << std::endl; }
    if (bool success = fifo.pop(out); success != false) { std::cerr << "failed!" << std::endl; }
    if (!fifo.empty()) { std::cerr << "failed!" << std::endl; }
}


