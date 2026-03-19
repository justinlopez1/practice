#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

// =======================
// Ring Buffer
// =======================
//
// Implement a fixed-size byte ring buffer.
//
// Required operations:
//   - initialize buffer state
//   - push one byte into the buffer
//   - pop one byte from the buffer
//
// Behavior:
//   - if buffer is full, push should fail and return 0
//   - if buffer is empty, pop should fail and return 0
//   - otherwise push/pop should succeed and return 1
//
// Requirements:
//   - use wraparound correctly
//   - do not overwrite unread data
//   - do not read invalid data
//
// Suggested design:
//   - fixed storage array
//   - head index
//   - tail index
//   - count
//
// Notes:
//   - store bytes (uint8_t)
//   - make capacity 8
//   - use count to distinguish full vs empty
//
// API:
//
// void ring_init(RingBuffer* rb);
// int ring_push(RingBuffer* rb, uint8_t value);
// int ring_pop(RingBuffer* rb, uint8_t* value);

#define RING_CAPACITY 9
#define CONCURRENCY_TEST_COUNT 100000

typedef struct
{
    uint8_t data[RING_CAPACITY];
    atomic_size_t head;
    atomic_size_t tail;
} RingBuffer;

void ring_init(RingBuffer* rb)
{
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
}

int ring_push(RingBuffer* rb, uint8_t value)
{
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    size_t next_tail = (tail + 1) % RING_CAPACITY;

    if (next_tail == head) {
        return 0;
    }

    rb->data[tail] = value;

    atomic_store_explicit(&rb->tail, next_tail, memory_order_release);

    return 1;
}

int ring_pop(RingBuffer* rb, uint8_t* value)
{
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);

    if (head == tail) {
        return 0;
    }

    size_t next_head = (head + 1) % RING_CAPACITY;
    *value = rb->data[head];

    atomic_store_explicit(&rb->head, next_head, memory_order_release);

    return 1;
}

void print_buffer_state(const RingBuffer* rb)
{
    printf("  head=%zu tail=%zu", rb->head, rb->tail);
}

void run_push_test(RingBuffer* rb, uint8_t value, int expected_ok)
{
    int ok = ring_push(rb, value);
    printf("push(%u): expected=%d got=%d %s\n",
           value, expected_ok, ok, (ok == expected_ok) ? "PASS" : "FAIL");
    print_buffer_state(rb);
    printf("\n");
}

void run_pop_test(RingBuffer* rb, int expected_ok, uint8_t expected_value)
{
    uint8_t value = 0;
    int ok = ring_pop(rb, &value);

    printf("pop(): expected_ok=%d got_ok=%d", expected_ok, ok);

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

    print_buffer_state(rb);
    printf("\n");
}

// =======================
// Concurrency test helpers
// =======================

typedef struct
{
    RingBuffer* rb;
    int count;
    volatile int done;
    volatile int push_failures;
} ProducerArgs;

typedef struct
{
    RingBuffer* rb;
    int count;
    volatile int done;
    volatile int pop_failures;
    uint8_t* out;
} ConsumerArgs;

void* producer_thread(void* arg)
{
    ProducerArgs* args = (ProducerArgs*)arg;

    for (int i = 0; i < args->count; ) {
        uint8_t value = (uint8_t)(i & 0xFF);

        if (ring_push(args->rb, value)) {
            i++;
        } else {
            args->push_failures++;
            sched_yield();
        }
    }

    args->done = 1;
    return NULL;
}

void* consumer_thread(void* arg)
{
    ConsumerArgs* args = (ConsumerArgs*)arg;

    for (int i = 0; i < args->count; ) {
        uint8_t value;

        if (ring_pop(args->rb, &value)) {
            args->out[i] = value;
            i++;
        } else {
            args->pop_failures++;
            sched_yield();
        }
    }

    args->done = 1;
    return NULL;
}

void run_spsc_test(int count)
{
    RingBuffer rb;
    ring_init(&rb);

    uint8_t* consumed = (uint8_t*)malloc((size_t)count);
    if (!consumed) {
        printf("malloc failed\n");
        return;
    }

    ProducerArgs pargs = {
        .rb = &rb,
        .count = count,
        .done = 0,
        .push_failures = 0
    };

    ConsumerArgs cargs = {
        .rb = &rb,
        .count = count,
        .done = 0,
        .pop_failures = 0,
        .out = consumed
    };

    pthread_t prod;
    pthread_t cons;

    pthread_create(&prod, NULL, producer_thread, &pargs);
    pthread_create(&cons, NULL, consumer_thread, &cargs);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    int order_ok = 1;
    for (int i = 0; i < count; i++) {
        uint8_t expected = (uint8_t)(i & 0xFF);
        if (consumed[i] != expected) {
            order_ok = 0;
            printf("SPSC order mismatch at i=%d expected=%u got=%u\n",
                   i, expected, consumed[i]);
            break;
        }
    }

    printf("SPSC test (%d items): %s\n", count, order_ok ? "PASS" : "FAIL");
    printf("  producer retries=%d consumer retries=%d\n",
           pargs.push_failures, cargs.pop_failures);
    print_buffer_state(&rb);
    printf("\n\n");

    free(consumed);
}

void run_fill_and_drain_cycles_test(int cycles)
{
    RingBuffer rb;
    ring_init(&rb);

    int ok = 1;

    for (int c = 0; c < cycles; c++) {
        for (int i = 0; i < RING_CAPACITY - 1; i++) {
            if (!ring_push(&rb, (uint8_t)(i + c))) {
                ok = 0;
                printf("cycle %d: unexpected push failure at i=%d\n", c, i);
                break;
            }
        }

        if (ring_push(&rb, 123)) {
            ok = 0;
            printf("cycle %d: expected full-buffer push failure\n", c);
            break;
        }

        for (int i = 0; i < RING_CAPACITY - 1; i++) {
            uint8_t value;
            if (!ring_pop(&rb, &value)) {
                ok = 0;
                printf("cycle %d: unexpected pop failure at i=%d\n", c, i);
                break;
            }
        }

        {
            uint8_t value;
            if (ring_pop(&rb, &value)) {
                ok = 0;
                printf("cycle %d: expected empty-buffer pop failure\n", c);
                break;
            }
        }
    }

    printf("fill/drain cycles test (%d cycles): %s\n", cycles, ok ? "PASS" : "FAIL");
    print_buffer_state(&rb);
    printf("\n\n");
}

void run_interleaved_wraparound_test(void)
{
    RingBuffer rb;
    ring_init(&rb);

    int ok = 1;
    uint8_t value;

    for (int i = 0; i < 50; i++) {
        if (!ring_push(&rb, (uint8_t)i)) {
            ok = 0;
            printf("wraparound test: push failed at %d\n", i);
            break;
        }

        if (!ring_pop(&rb, &value)) {
            ok = 0;
            printf("wraparound test: pop failed at %d\n", i);
            break;
        }

        if (value != (uint8_t)i) {
            ok = 0;
            printf("wraparound test: expected=%d got=%u\n", i, value);
            break;
        }
    }

    printf("interleaved wraparound test: %s\n", ok ? "PASS" : "FAIL");
    print_buffer_state(&rb);
    printf("\n\n");
}

int main()
{
    RingBuffer rb;
    ring_init(&rb);

    printf("Initial state:\n");
    print_buffer_state(&rb);
    printf("\n\n");

    run_pop_test(&rb, 0, 0);

    run_push_test(&rb, 10, 1);
    run_push_test(&rb, 20, 1);
    run_push_test(&rb, 30, 1);

    run_pop_test(&rb, 1, 10);
    run_pop_test(&rb, 1, 20);

    run_push_test(&rb, 40, 1);
    run_push_test(&rb, 50, 1);
    run_push_test(&rb, 60, 1);
    run_push_test(&rb, 70, 1);
    run_push_test(&rb, 80, 1);
    run_push_test(&rb, 90, 1);
    run_push_test(&rb, 100, 1);

    run_push_test(&rb, 110, 0);

    run_pop_test(&rb, 1, 30);
    run_pop_test(&rb, 1, 40);
    run_pop_test(&rb, 1, 50);
    run_pop_test(&rb, 1, 60);
    run_pop_test(&rb, 1, 70);
    run_pop_test(&rb, 1, 80);
    run_pop_test(&rb, 1, 90);
    run_pop_test(&rb, 1, 100);

    run_pop_test(&rb, 0, 0);

    printf("==== extra single-thread stress tests ====\n\n");
    run_fill_and_drain_cycles_test(10000);
    run_interleaved_wraparound_test();

    printf("==== concurrency tests ====\n\n");
    run_spsc_test(10000);
    run_spsc_test(50000);
    run_spsc_test(CONCURRENCY_TEST_COUNT);

    return 0;
}
