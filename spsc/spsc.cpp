//
// Created by justin on 2/1/26.
//

#include <iostream>
#include <atomic>
#include <array>
#include <optional>
#include <thread>

// lock free spsc fifo buffer
template<typename T, std::size_t S>
class SPSC {
public:
    bool push(T x) { // return false on full
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_relaxed);

        if ((t+1) % S == h) { // if full
            return false;
        }

        arr_[t] = x;
        tail_.store((t+1)%S, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);

        if (h == t) return std::nullopt;

        T temp = arr_[h];

        head_.store((h+1)%S, std::memory_order_release);

        return temp;
    }

    bool empty() {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

private:
    std::array<T, S> arr_{};
    std::atomic<size_t> head_{}; // pop at the head
    std::atomic<size_t> tail_{}; // push at the tail
};

SPSC<int, 20> queue;
std::atomic<bool> finished = false;

void Producer() {
   for (int i = 0; i < 100000; i++) {
       while (!queue.push(i)) {}
   }
    finished.store(true);
}

void Consumer() {
    size_t count{};
    while(!finished.load() || !queue.empty()) {
        std::optional<int> x = queue.pop();
        if (x.has_value()) {
            std::cout << x.value() << std::endl;
            if (count != x.value()) exit(-1);
            count++;
        }
    }
}

int main() {
    std::thread t1(&Producer);
    std::thread t2(&Consumer);

    t1.join();
    t2.join();

    return 0;
}