#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>

// =======================
// Bounded Blocking Queue
// =======================
//
// Implement a thread-safe bounded queue of bytes.
//
// The queue must support:
//   - multiple producers
//   - multiple consumers
//   - blocking push when the queue is full
//   - blocking pop when the queue is empty
//   - timeout for both push and pop
//
// API requirements:
//
// int queue_init(Queue* q);
// int queue_destroy(Queue* q);
//
// int queue_push(Queue* q, uint8_t value, int timeout_ms);
// int queue_pop(Queue* q, uint8_t* value, int timeout_ms);
//
// Return values:
//   - return 1 on success
//   - return 0 on timeout or failure
//
// Behavior:
//   - queue_push:
//       if the queue is full, wait until space becomes available
//       if timeout expires first, return 0
//
//   - queue_pop:
//       if the queue is empty, wait until an item becomes available
//       if timeout expires first, return 0
//
// Constraints:
//   - bounded capacity
//   - preserve FIFO order
//   - safe for multiple producers and multiple consumers
//   - no busy-wait spinning
//
// Timeout semantics:
//   - timeout_ms is the maximum total time allowed for the operation
//   - this includes waiting for the queue condition and any mutex reacquisition
//   - timeout_ms == 0 means do not block
//
// Suggested implementation approach:
//   - ring buffer for storage
//   - one mutex protecting queue state
//   - either:
//       * two condition variables: not_empty and not_full
//       * or two semaphores plus a mutex
//
// Queue state will likely include:
//   - data array
//   - head index
//   - tail index
//   - count
//
// Capacity for this exercise:
//   - use 8
//
// Notes:
//   - if using condition variables, use a while loop around waits
//   - handle spurious wakeups correctly
//   - FIFO order must still hold under concurrency

#define QUEUE_CAPACITY 8
#define STRESS_ITEMS 10000

typedef struct
{
    uint8_t data[QUEUE_CAPACITY];
    size_t head;
    size_t tail;



} Queue;

int queue_init(Queue* q)
{
    return 0;
}

int queue_destroy(Queue* q)
{
    return 0;
}

int queue_push(Queue* q, uint8_t value, int timeout_ms)
{
    return 0;
}

int queue_pop(Queue* q, uint8_t* value, int timeout_ms)
{
    return 0;
}

void print_queue_state(const Queue* q)
{
    printf("  head=%zu tail=%zu\n", q->head, q->tail);
}

void run_push_test(const char* name, Queue* q, uint8_t value, int timeout_ms, int expected_ok)
{
    int ok = queue_push(q, value, timeout_ms);
    printf("%s: push(%u, timeout=%d) expected=%d got=%d %s\n",
           name, value, timeout_ms, expected_ok, ok, (ok == expected_ok) ? "PASS" : "FAIL");
    print_queue_state(q);
    printf("\n");
}

void run_pop_test(const char* name, Queue* q, int timeout_ms, int expected_ok, uint8_t expected_value)
{
    uint8_t value = 0;
    int ok = queue_pop(q, &value, timeout_ms);

    printf("%s: pop(timeout=%d) expected_ok=%d got_ok=%d",
           name, timeout_ms, expected_ok, ok);

    if (ok) {
        printf(" expected_value=%u got_value=%u", expected_value, value);
    }

    if (ok != expected_ok) {
        printf(" FAIL\n");
    } else if (ok && value != expected_value) {
        printf(" FAIL\n");
    } else {
        printf(" PASS\n");
    }

    print_queue_state(q);
    printf("\n");
}

// =======================
// Concurrency test helpers
// =======================

typedef struct
{
    Queue* q;
    int start;
    int count;
    int timeout_ms;
    volatile int push_failures;
} ProducerArgs;

typedef struct
{
    Queue* q;
    int count;
    int timeout_ms;
    volatile int pop_failures;
    uint8_t* out;
} ConsumerArgs;

void* producer_thread(void* arg)
{
    ProducerArgs* a = (ProducerArgs*)arg;

    for (int i = 0; i < a->count; i++) {
        uint8_t v = (uint8_t)((a->start + i) & 0xFF);
        if (!queue_push(a->q, v, a->timeout_ms)) {
            a->push_failures++;
        }
    }

    return NULL;
}

void* consumer_thread(void* arg)
{
    ConsumerArgs* a = (ConsumerArgs*)arg;

    for (int i = 0; i < a->count; i++) {
        uint8_t v = 0;
        if (queue_pop(a->q, &v, a->timeout_ms)) {
            a->out[i] = v;
        } else {
            a->pop_failures++;
        }
    }

    return NULL;
}

void run_spsc_blocking_test(void)
{
    Queue q;
    queue_init(&q);

    uint8_t out[64] = {0};

    ProducerArgs pa = { .q = &q, .start = 0, .count = 64, .timeout_ms = 1000, .push_failures = 0 };
    ConsumerArgs ca = { .q = &q, .count = 64, .timeout_ms = 1000, .pop_failures = 0, .out = out };

    pthread_t pt, ct;
    pthread_create(&pt, NULL, producer_thread, &pa);
    pthread_create(&ct, NULL, consumer_thread, &ca);

    pthread_join(pt, NULL);
    pthread_join(ct, NULL);

    int ok = 1;
    for (int i = 0; i < 64; i++) {
        uint8_t expected = (uint8_t)(i & 0xFF);
        if (out[i] != expected) {
            ok = 0;
            printf("SPSC mismatch at i=%d expected=%u got=%u\n", i, expected, out[i]);
            break;
        }
    }

    if (pa.push_failures != 0 || ca.pop_failures != 0) {
        ok = 0;
        printf("SPSC unexpected timeouts push_failures=%d pop_failures=%d\n",
               pa.push_failures, ca.pop_failures);
    }

    printf("SPSC blocking test: %s\n", ok ? "PASS" : "FAIL");
    print_queue_state(&q);
    printf("\n");

    queue_destroy(&q);
}

void run_timeout_push_test(void)
{
    Queue q;
    queue_init(&q);

    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        (void)queue_push(&q, (uint8_t)i, 0);
    }

    run_push_test("full queue immediate timeout", &q, 99, 0, 0);

    queue_destroy(&q);
}

void run_timeout_pop_test(void)
{
    Queue q;
    queue_init(&q);

    run_pop_test("empty queue immediate timeout", &q, 0, 0, 0);

    queue_destroy(&q);
}

void run_fifo_single_thread_test(void)
{
    Queue q;
    queue_init(&q);

    run_push_test("push 10", &q, 10, 0, 1);
    run_push_test("push 20", &q, 20, 0, 1);
    run_push_test("push 30", &q, 30, 0, 1);

    run_pop_test("pop 10", &q, 0, 1, 10);
    run_pop_test("pop 20", &q, 0, 1, 20);
    run_pop_test("pop 30", &q, 0, 1, 30);

    run_pop_test("empty pop", &q, 0, 0, 0);

    queue_destroy(&q);
}

void run_fill_then_drain_test(void)
{
    Queue q;
    queue_init(&q);

    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        char name[64];
        snprintf(name, sizeof(name), "fill push %d", i);
        run_push_test(name, &q, (uint8_t)(i + 1), 0, 1);
    }

    run_push_test("push when full", &q, 99, 0, 0);

    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        char name[64];
        snprintf(name, sizeof(name), "drain pop %d", i);
        run_pop_test(name, &q, 0, 1, (uint8_t)(i + 1));
    }

    run_pop_test("pop when empty", &q, 0, 0, 0);

    queue_destroy(&q);
}

void run_wraparound_test(void)
{
    Queue q;
    queue_init(&q);

    for (int i = 0; i < 5; i++) {
        (void)queue_push(&q, (uint8_t)(10 + i), 0);
    }

    for (int i = 0; i < 3; i++) {
        run_pop_test("wraparound initial pops", &q, 0, 1, (uint8_t)(10 + i));
    }

    for (int i = 0; i < 6; i++) {
        char name[64];
        snprintf(name, sizeof(name), "wraparound push %d", i);
        run_push_test(name, &q, (uint8_t)(20 + i), 0, 1);
    }

    run_push_test("wraparound push when full", &q, 200, 0, 0);

    run_pop_test("wrap pop 13", &q, 0, 1, 13);
    run_pop_test("wrap pop 14", &q, 0, 1, 14);
    run_pop_test("wrap pop 20", &q, 0, 1, 20);
    run_pop_test("wrap pop 21", &q, 0, 1, 21);
    run_pop_test("wrap pop 22", &q, 0, 1, 22);
    run_pop_test("wrap pop 23", &q, 0, 1, 23);
    run_pop_test("wrap pop 24", &q, 0, 1, 24);
    run_pop_test("wrap pop 25", &q, 0, 1, 25);

    queue_destroy(&q);
}

void run_mp_mc_smoke_test(void)
{
    Queue q;
    queue_init(&q);

    uint8_t out1[STRESS_ITEMS / 2] = {0};
    uint8_t out2[STRESS_ITEMS / 2] = {0};

    ProducerArgs p1 = { .q = &q, .start = 0, .count = STRESS_ITEMS / 2, .timeout_ms = 1000, .push_failures = 0 };
    ProducerArgs p2 = { .q = &q, .start = STRESS_ITEMS / 2, .count = STRESS_ITEMS / 2, .timeout_ms = 1000, .push_failures = 0 };

    ConsumerArgs c1 = { .q = &q, .count = STRESS_ITEMS / 2, .timeout_ms = 1000, .pop_failures = 0, .out = out1 };
    ConsumerArgs c2 = { .q = &q, .count = STRESS_ITEMS / 2, .timeout_ms = 1000, .pop_failures = 0, .out = out2 };

    pthread_t pt1, pt2, ct1, ct2;
    pthread_create(&pt1, NULL, producer_thread, &p1);
    pthread_create(&pt2, NULL, producer_thread, &p2);
    pthread_create(&ct1, NULL, consumer_thread, &c1);
    pthread_create(&ct2, NULL, consumer_thread, &c2);

    pthread_join(pt1, NULL);
    pthread_join(pt2, NULL);
    pthread_join(ct1, NULL);
    pthread_join(ct2, NULL);

    int total_push_failures = p1.push_failures + p2.push_failures;
    int total_pop_failures = c1.pop_failures + c2.pop_failures;

    printf("MPMC smoke test:\n");
    printf("  producer timeouts=%d consumer timeouts=%d\n", total_push_failures, total_pop_failures);
    print_queue_state(&q);
    printf("  NOTE: for a correct blocking implementation with generous timeout,\n");
    printf("        these should usually be 0.\n\n");

    queue_destroy(&q);
}

int main(void)
{
    run_fifo_single_thread_test();
    run_fill_then_drain_test();
    run_wraparound_test();

    run_timeout_push_test();
    run_timeout_pop_test();

    run_spsc_blocking_test();
    run_mp_mc_smoke_test();

    return 0;
}
