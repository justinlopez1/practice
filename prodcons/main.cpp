#include <iostream>
#include <queue>

// 2 Implement a producer–consumer system
// One producer thread generates integers 0..999
// One consumer thread removes and processes them
//
// Part A:
//   - Use a shared queue with NO synchronization
//   - Observe crashes or incorrect behavior
//
// Part B:
//   - Fix using std::mutex only
//   - Consumer may busy-wait when queue is empty
//
// Part C:
//   - Replace busy-waiting with std::condition_variable
//   - Consumer must block efficiently

int main() {
    return 0;
}