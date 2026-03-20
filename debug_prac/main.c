/*
 * ============================================================
 * DEBUGGING PRACTICE — Three Buggy Programs
 * ============================================================
 *
 * Each program compiles clean. Each one is broken in a
 * specific, realistic way. For each one:
 *
 *   1. Read the symptom description.
 *   2. Compile and run it to observe the behavior.
 *   3. Find the bug — the specific line(s) and why.
 *   4. Fix it and confirm the fix works.
 *
 * Compile each program by defining which bug to run:
 *
 *   gcc -Wall -Wextra -pthread -DBUG=1 -o debug debug_practice.c && ./debug
 *   gcc -Wall -Wextra -pthread -DBUG=2 -o debug debug_practice.c && ./debug
 *   gcc -Wall -Wextra -pthread -DBUG=3 -o debug debug_practice.c && ./debug
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define BUG 3

/* ============================================================
 * BUG 1 — "The counter is always wrong"
 * ============================================================
 *
 * WHAT IT DOES:
 *   Spawns 8 threads. Each thread increments a shared counter
 *   10000 times. We expect the final value to be 80000.
 *
 * SYMPTOM:
 *   The program always prints a number less than 80000.
 *   The exact value changes every run.
 *
 * YOUR TASKS:
 *   - Identify the exact bug (it is not where you think).
 *   - Explain why it produces a value less than 80000
 *     rather than garbage or a crash.
 *   - Fix it.
 *
 * HINT: the mutex is real and is being used. look closer.
 * ============================================================ */

#if BUG == 1

#define NUM_THREADS  15
#define INCREMENTS   1000000

typedef struct {
    pthread_mutex_t mtx;
    int             value;
} Counter;

static void counter_init(Counter *c) {
    pthread_mutex_init(&c->mtx, NULL);
    c->value = 0;
}

static void counter_increment(Counter *c) {
    pthread_mutex_lock(&c->mtx);
    c->value++;
    pthread_mutex_unlock(&c->mtx);
}

static void *worker(void *arg) {
    Counter *c = (Counter *)arg;
    for (int i = 0; i < INCREMENTS; i++) {
        counter_increment(c);
    }
    return NULL;
}

int main(void) {
    Counter c;
    counter_init(&c);

    pthread_t threads[NUM_THREADS];
    Counter   thread_counters[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        counter_init(&thread_counters[i]);
        pthread_create(&threads[i], NULL, worker, &thread_counters[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        c.value += thread_counters[i].value;
    }

    printf("Final counter value: %d (expected %d)\n",
           c.value, NUM_THREADS * INCREMENTS);
    printf("%s\n", c.value == NUM_THREADS * INCREMENTS ? "PASS" : "FAIL");

    return 0;
}

#endif /* BUG == 1 */


/* ============================================================
 * BUG 2 — "It deadlocks, but only sometimes"
 * ============================================================
 *
 * WHAT IT DOES:
 *   Models two resources (A and B) that threads need to
 *   acquire together to do work. Thread type X locks A then B.
 *   Thread type Y locks B then A. They run concurrently.
 *
 * SYMPTOM:
 *   The program usually completes fine. Occasionally it hangs
 *   forever. The hang is not reliably reproducible but becomes
 *   more likely with more iterations.
 *
 * YOUR TASKS:
 *   - Identify the exact bug and explain why the hang is
 *     non-deterministic (i.e. why it doesn't always happen).
 *   - Describe the exact interleaving that causes the hang.
 *   - Fix it without removing any locking.
 *
 * NOTE: to make the race more likely the program uses
 *   sched_yield() at the dangerous moment. In real code the
 *   same bug would just be rarer.
 * ============================================================ */

#if BUG == 2

#include <sched.h>

#define ITERATIONS 200

pthread_mutex_t lock_a;
pthread_mutex_t lock_b;
static atomic_int work_done = 0;

static void *thread_x(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock_a);
        sched_yield();                  /* yield while holding lock_a */
        pthread_mutex_lock(&lock_b);

        atomic_fetch_add(&work_done, 1);

        pthread_mutex_unlock(&lock_b);
        pthread_mutex_unlock(&lock_a);
    }
    return NULL;
}

static void *thread_y(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock_a);
        sched_yield();                  /* yield while holding lock_b */
        pthread_mutex_lock(&lock_b);

        atomic_fetch_add(&work_done, 1);

        pthread_mutex_unlock(&lock_b);
        pthread_mutex_unlock(&lock_a);
    }
    return NULL;
}

int main(void) {
    pthread_mutex_init(&lock_a, NULL);
    pthread_mutex_init(&lock_b, NULL);

    pthread_t tx, ty;
    pthread_create(&tx, NULL, thread_x, NULL);
    pthread_create(&ty, NULL, thread_y, NULL);

    pthread_join(tx, NULL);
    pthread_join(ty, NULL);

    printf("Work done: %d (expected %d)\n",
           atomic_load(&work_done), ITERATIONS * 2);
    printf("%s\n",
           atomic_load(&work_done) == ITERATIONS * 2 ? "PASS" : "FAIL");

    return 0;
}

#endif /* BUG == 2 */


/* ============================================================
 * BUG 3 — "It works fine, then corrupts, then maybe crashes"
 * ============================================================
 *
 * WHAT IT DOES:
 *   A single producer pushes integers into a ring buffer.
 *   A single consumer reads from it. The ring buffer is
 *   designed to be lock-free (SPSC). After all items are
 *   produced and consumed, the program checks that every
 *   value was received exactly once and in order.
 *
 * SYMPTOM:
 *   On a single-core machine or lightly loaded system it
 *   often passes. On a multi-core machine under load it
 *   occasionally produces wrong values or gets stuck.
 *   The corruption is not random — it tends to happen in
 *   bursts when the producer is fast and the consumer is slow.
 *
 * YOUR TASKS:
 *   - Identify the exact line(s) causing the corruption.
 *   - Explain the memory-ordering violation in terms of
 *     what the CPU is allowed to reorder and why that
 *     breaks the consumer's view of the data.
 *   - Fix it using only C11 atomics (no mutex).
 *
 * HINT: the bug is not in the index arithmetic.
 * ============================================================ */

#if BUG == 3

#define RING_SIZE  16       /* must be power of 2 */
#define RING_MASK  (RING_SIZE - 1)
#define N_ITEMS    100000

typedef struct {
    int            buf[RING_SIZE];
    atomic_size_t  head;   /* consumer reads  from buf[head % RING_SIZE] */
    atomic_size_t  tail;   /* producer writes to   buf[tail % RING_SIZE] */
} RingBuffer;

static RingBuffer ring;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < N_ITEMS; i++) {
        /* wait for space */
        size_t tail, head;
        do {
            tail = atomic_load(&ring.tail);
            head = atomic_load(&ring.head);
        } while (tail - head >= RING_SIZE);

        /* write the item, then advance tail */
        ring.buf[tail & RING_MASK] = i;                      /* (A) */
        atomic_store(&ring.tail, tail + 1);                  /* (B) */
    }
    return NULL;
}

static void *consumer(void *arg) {
    (void)arg;
    int errors = 0;
    for (int i = 0; i < N_ITEMS; i++) {
        /* wait for an item */
        size_t head, tail;
        do {
            head = atomic_load(&ring.head);
            tail = atomic_load(&ring.tail);
        } while (head >= tail);

        /* read the item, then advance head */
        int val = ring.buf[head & RING_MASK];                /* (C) */
        atomic_store(&ring.head, head + 1);                  /* (D) */

        if (val != i) {
            errors++;
            if (errors <= 5)
                printf("  mismatch at i=%d got %d\n", i, val);
        }
    }
    return (void *)(intptr_t)errors;
}

int main(void) {
    atomic_init(&ring.head, 0);
    atomic_init(&ring.tail, 0);

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    void *err_ptr;
    pthread_join(cons, &err_ptr);
    int errors = (int)(intptr_t)err_ptr;

    printf("Errors: %d\n", errors);
    printf("%s\n", errors == 0 ? "PASS" : "FAIL");
    return 0;
}

#endif /* BUG == 3 */
