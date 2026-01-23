#include <iostream>
#include <thread>
#include <atomic>

// 1 Two threads increment a counter 1M times
// First with no lock (watch it break)
// Then with mutex (fixed)
// Then with atomic (fixed)

size_t total{};

std::atomic<size_t> total_atomic{}; // how does this work?

std::mutex mtx;
size_t total_mtx;

void worker(size_t num) {
    for (size_t i = 0; i < num; i++) {
        total++;

        total_atomic.fetch_add(1);

        std::lock_guard<std::mutex> guard(mtx); // aquires and releases on deconstruction
        total_mtx++;
    }
}

int main() {

    size_t x = 10000;

    std::thread thread1(worker, x);
    std::thread thread2(worker, x);
    std::thread thread3(worker, x);
    std::thread thread4(worker, x);
    std::thread thread5(worker, x);

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    thread5.join();

    std::cout << total << std::endl;
    std::cout << total_atomic << std::endl;
    std::cout << total_mtx << std::endl;

    return 0;
}