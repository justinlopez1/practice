#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
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
    sem_t filled_spaces;
    sem_t empty_spaces;
    pthread_mutex_t mtx;
} Queue;

int queue_init(Queue* q)
{
    q->head = 0;
    q->tail = 0;
    sem_init(&q->filled_spaces, 0, 0);
    sem_init(&q->empty_spaces, 0, QUEUE_CAPACITY);
    pthread_mutex_init(&q->mtx, NULL);
    return 1;
}

int queue_destroy(Queue* q)
{
    sem_destroy(&q->filled_spaces);
    sem_destroy(&q->empty_spaces);
    pthread_mutex_destroy(&q->mtx);
    return 1;
}

int queue_push(Queue* q, uint8_t value)
{
    sem_wait(&q->empty_spaces);
    pthread_mutex_lock(&q->mtx);

    q->data[q->tail] = value;
    q->tail++;
    q->tail %= QUEUE_CAPACITY;

    pthread_mutex_unlock(&q->mtx);
    sem_post(&q->filled_spaces);
    return 1;
}

int queue_pop(Queue* q, uint8_t* value)
{
    sem_wait(&q->filled_spaces);
    pthread_mutex_lock(&q->mtx);

    *value = q->data[q->head];
    q->head++;
    q->head %= QUEUE_CAPACITY;

    pthread_mutex_unlock(&q->mtx);
    sem_post(&q->empty_spaces);
    return 1;
}

void print_queue_state(const Queue* q)
{
    printf("  head=%zu tail=%zu count=%zu\n", q->head, q->tail);
}

// =======================
// Basic Tests
// =======================

void run_fifo_test(void)
{
    Queue q;
    queue_init(&q);

    printf("FIFO test\n");

    queue_push(&q, 10);
    queue_push(&q, 20);
    queue_push(&q, 30);

    uint8_t v;

    queue_pop(&q, &v);
    printf("expected=10 got=%u\n", v);

    queue_pop(&q, &v);
    printf("expected=20 got=%u\n", v);

    queue_pop(&q, &v);
    printf("expected=30 got=%u\n", v);

    print_queue_state(&q);
    printf("\n");

    queue_destroy(&q);
}

// =======================
// Concurrency Tests
// =======================

typedef struct
{
    Queue* q;
    int start;
    int count;
} ProducerArgs;

typedef struct
{
    Queue* q;
    int count;
    uint8_t* out;
} ConsumerArgs;

void* producer_thread(void* arg)
{
    ProducerArgs* a = (ProducerArgs*)arg;

    for (int i = 0; i < a->count; i++) {
        uint8_t v = (uint8_t)((a->start + i) & 0xFF);
        queue_push(a->q, v);
    }

    return NULL;
}

void* consumer_thread(void* arg)
{
    ConsumerArgs* a = (ConsumerArgs*)arg;

    for (int i = 0; i < a->count; i++) {
        queue_pop(a->q, &a->out[i]);
    }

    return NULL;
}

void run_spsc_test(void)
{
    Queue q;
    queue_init(&q);

    uint8_t out[64] = {0};

    ProducerArgs pa = { .q = &q, .start = 0, .count = 64 };
    ConsumerArgs ca = { .q = &q, .count = 64, .out = out };

    pthread_t pt, ct;
    pthread_create(&pt, NULL, producer_thread, &pa);
    pthread_create(&ct, NULL, consumer_thread, &ca);

    pthread_join(pt, NULL);
    pthread_join(ct, NULL);

    int ok = 1;
    for (int i = 0; i < 64; i++) {
        if (out[i] != (uint8_t)i) {
            ok = 0;
            printf("Mismatch at %d expected=%d got=%u\n", i, i, out[i]);
            break;
        }
    }

    printf("SPSC test: %s\n\n", ok ? "PASS" : "FAIL");

    queue_destroy(&q);
}

void run_mpmc_test(void)
{
    Queue q;
    queue_init(&q);

    uint8_t out1[STRESS_ITEMS / 2] = {0};
    uint8_t out2[STRESS_ITEMS / 2] = {0};

    ProducerArgs p1 = { .q = &q, .start = 0, .count = STRESS_ITEMS / 2 };
    ProducerArgs p2 = { .q = &q, .start = STRESS_ITEMS / 2, .count = STRESS_ITEMS / 2 };

    ConsumerArgs c1 = { .q = &q, .count = STRESS_ITEMS / 2, .out = out1 };
    ConsumerArgs c2 = { .q = &q, .count = STRESS_ITEMS / 2, .out = out2 };

    pthread_t pt1, pt2, ct1, ct2;

    pthread_create(&pt1, NULL, producer_thread, &p1);
    pthread_create(&pt2, NULL, producer_thread, &p2);
    pthread_create(&ct1, NULL, consumer_thread, &c1);
    pthread_create(&ct2, NULL, consumer_thread, &c2);

    pthread_join(pt1, NULL);
    pthread_join(pt2, NULL);
    pthread_join(ct1, NULL);
    pthread_join(ct2, NULL);

    printf("MPMC test complete (verify no deadlock / crash)\n");
    print_queue_state(&q);
    printf("\n");

    queue_destroy(&q);
}

int main(void)
{
    run_fifo_test();
    run_spsc_test();
    run_mpmc_test();
    return 0;
}
