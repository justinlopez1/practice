/*
 * ============================================================
 * PROBLEM: Bounded Blocking Queue with Timeout
 * ============================================================
 *
 * CONTEXT
 * -------
 * You are writing a fixed-capacity FIFO queue that is safe to
 * use from multiple threads simultaneously. This shows up
 * constantly in embedded firmware: an ISR or producer thread
 * fills the queue; a consumer thread drains it. When the queue
 * is full, a producer must wait (not spin, not drop) until a
 * slot opens. When the queue is empty, a consumer must wait
 * until an item arrives. Both waits must be bounded by a
 * caller-supplied timeout so the system can detect stalls and
 * recover gracefully.
 *
 * YOUR TASK
 * ---------
 * Implement the four functions below:
 *
 *   bq_init   -- initialise a queue of the given capacity
 *   bq_push   -- enqueue one int, blocking up to timeout_ms
 *   bq_pop    -- dequeue one int, blocking up to timeout_ms
 *   bq_destroy -- free all resources
 *
 * FUNCTION SIGNATURES (do not change these)
 *
 *   int bq_init   (BoundedQueue *q, size_t capacity);
 *   int bq_push   (BoundedQueue *q, int value, int timeout_ms);
 *   int bq_pop    (BoundedQueue *q, int *out,  int timeout_ms);
 *   void bq_destroy(BoundedQueue *q);
 *
 * RETURN VALUES
 *
 *   bq_init   : 0 on success, -1 on allocation failure
 *   bq_push   : 0 on success, -1 if timed out before a slot opened,
 *               -2 if the queue has been destroyed
 *   bq_pop    : 0 on success, -1 if timed out before an item appeared,
 *               -2 if the queue has been destroyed while waiting
 *   bq_destroy: void; must unblock any threads waiting in push/pop
 *               so they can return -2 and exit cleanly
 *
 * TIMEOUT SEMANTICS
 *
 *   timeout_ms == 0   : non-blocking — fail immediately if
 *                        the queue is full/empty right now
 *   timeout_ms  > 0   : block for up to that many milliseconds,
 *                        then return -1
 *   timeout_ms == -1  : block forever (no timeout)
 *
 * CONSTRAINTS
 *
 *   - capacity >= 1
 *   - Multiple threads may call push/pop concurrently
 *   - You may use: pthread_mutex_t, pthread_cond_t, and the
 *     standard POSIX clock/time APIs (clock_gettime, etc.)
 *   - No busy-waiting. A blocked thread must sleep on a condvar.
 *   - The internal buffer must be heap-allocated
 *   - Destroying the queue while threads are blocked must not
 *     deadlock or crash; those threads must return -2
 *
 * HINTS (read only if stuck)
 *
 *   1. You need two condition variables: one that producers
 *      signal when they add an item (wakes consumers), and one
 *      that consumers signal when they remove an item (wakes
 *      producers). A single condvar works too but is trickier.
 *
 *   2. For timed waits, use pthread_cond_timedwait. It takes an
 *      absolute timespec (not a relative delta). To compute the
 *      deadline:
 *
 *        struct timespec deadline;
 *        clock_gettime(CLOCK_REALTIME, &deadline);
 *        deadline.tv_sec  += timeout_ms / 1000;
 *        deadline.tv_nsec += (timeout_ms % 1000) * 1000000L;
 *        if (deadline.tv_nsec >= 1000000000L) {
 *            deadline.tv_sec++;
 *            deadline.tv_nsec -= 1000000000L;
 *        }
 *
 *   3. pthread_cond_timedwait returns ETIMEDOUT when the
 *      deadline passes. Check errno or the return value.
 *
 *   4. Add a `destroyed` flag to your struct. bq_destroy sets
 *      it, then broadcasts on both condvars so sleeping threads
 *      wake up, see the flag, and return -2.
 *
 *   5. The classic circular-buffer trick: maintain head, tail,
 *      and count. push writes to buf[tail] and increments tail
 *      mod capacity. pop reads from buf[head] and increments
 *      head mod capacity.
 *
 * ============================================================
 * YOUR IMPLEMENTATION GOES BELOW
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <assert.h>
#include <semaphore.h>

/* ---------- data structure ---------- */

typedef struct {
    int            *buf;
    size_t          capacity;
    size_t          head;       /* next read  index */
    size_t          tail;       /* next write index */
    int             destroyed;  /* set by bq_destroy */

    pthread_mutex_t mtx;
    sem_t full_spaces;
    sem_t empty_spaces;
} BoundedQueue;

/* ---------- API ---------- */

int  bq_init   (BoundedQueue *q, size_t capacity) {
    if (capacity < 1) return -1;
    q->capacity = capacity;
    q->buf = (int*)malloc(capacity * sizeof(int));
    q->head = 0;
    q->tail = 0;
    q->destroyed = 0;
    pthread_mutex_init(&q->mtx, NULL);
    sem_init(&q->full_spaces, 0, 0);
    sem_init(&q->empty_spaces, 0, capacity);
    return 0;
}

int  bq_push   (BoundedQueue *q, int value,  int timeout_ms) {
    // wait on empty space
    if (timeout_ms == -1) {
        sem_wait(&q->empty_spaces);
    }
    else if (timeout_ms == 0) {
        if (sem_trywait(&q->empty_spaces) != 0) {
            return -1;
        }
    }
    else {
        // Step 1: get the current absolute time
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        // Step 2: add your timeout as a delta
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;

        // Step 3: normalize — tv_nsec must stay below 1 billion
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec  += 1;
            ts.tv_nsec -= 1000000000L;
        }

        if (sem_timedwait(&q->empty_spaces, &ts) != 0) {
            return -1;
        }
    }


    // get mtx
    pthread_mutex_lock(&q->mtx);

    if (q->destroyed) {
        // pass the wake forward
        sem_post(&q->empty_spaces);
        pthread_mutex_unlock(&q->mtx);
        return -2;
    }

    // add thing
    q->buf[q->tail++] = value;
    q->tail %= q->capacity;

    // release mtx
    pthread_mutex_unlock(&q->mtx);

    // post to full space
    sem_post(&q->full_spaces);

    return 0;
}

int  bq_pop    (BoundedQueue *q, int *out,   int timeout_ms) {
    // wait on full space
    if (timeout_ms == -1) {
        sem_wait(&q->full_spaces);
    }
    else if (timeout_ms == 0) {
        if (sem_trywait(&q->full_spaces) != 0) {
            return -1;
        }
    }
    else {
        // Step 1: get the current absolute time
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        // Step 2: add your timeout as a delta
        ts.tv_sec  += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000L;

        // Step 3: normalize — tv_nsec must stay below 1 billion
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec  += 1;
            ts.tv_nsec -= 1000000000L;
        }

        if (sem_timedwait(&q->full_spaces, &ts) != 0) {
            return -1;
        }
    }

    // get mtx
    pthread_mutex_lock(&q->mtx);

    if (q->destroyed) {
        // pass the wake forward
        sem_post(&q->full_spaces);
        pthread_mutex_unlock(&q->mtx);
        return -2;
    }

    // pop thing
    *out = q->buf[q->head++];
    q->head %= q->capacity;

    // release mtx
    pthread_mutex_unlock(&q->mtx);

    // post to full space
    sem_post(&q->empty_spaces);

    return 0;
}

void bq_destroy(BoundedQueue *q) {
    q->destroyed = 1;

    // pass it forward structure in threads
    sem_post(&q->empty_spaces);
    sem_post(&q->full_spaces);
}

/*
 * TODO: implement bq_init, bq_push, bq_pop, bq_destroy here.
 * The test harness below calls them directly.
 * Do not modify anything below the "TEST HARNESS" marker.
 */


/* ============================================================
 * TEST HARNESS
 * ============================================================
 *
 * Compile:
 *   gcc -Wall -Wextra -pthread -o bq_test bounded_queue.c && ./bq_test
 *
 * Each test prints PASS or FAIL with a short reason.
 * The final line prints a summary: "X / Y tests passed."
 *
 * Tests are grouped:
 *   T01–T07  single-threaded correctness
 *   T08–T12  timeout and non-blocking edge cases
 *   T13–T18  multi-threaded stress / ordering
 *   T19–T22  bq_destroy and wake-up semantics
 *   T23–T26  adversarial / hell-mode edge cases
 * ============================================================ */

static int passed = 0;
static int total  = 0;

#define CHECK(label, cond) do {          \
    total++;                             \
    if (cond) { passed++;                \
        printf("PASS  %s\n", label); }   \
    else      printf("FAIL  %s\n", label); \
} while (0)

/* ---- helpers ---- */

static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ============================================================
 * T01–T07  SINGLE-THREADED CORRECTNESS
 * ============================================================ */

static void test_single_threaded(void) {
    BoundedQueue q;

    /* T01: init succeeds */
    CHECK("T01 init returns 0", bq_init(&q, 4) == 0);

    /* T02: push into empty queue succeeds immediately */
    CHECK("T02 push into empty", bq_push(&q, 10, 0) == 0);

    /* T03: pop returns the value that was pushed */
    int v = 0;
    bq_pop(&q, &v, 0);
    CHECK("T03 pop returns pushed value", v == 10);

    /* T04: FIFO ordering preserved across 4 pushes */
    for (int i = 1; i <= 4; i++) bq_push(&q, i * 100, 0);
    int order_ok = 1;
    for (int i = 1; i <= 4; i++) {
        bq_pop(&q, &v, 0);
        if (v != i * 100) order_ok = 0;
    }
    CHECK("T04 FIFO order", order_ok);

    /* T05: push onto full queue with timeout_ms==0 returns -1 immediately */
    for (int i = 0; i < 4; i++) bq_push(&q, i, 0);
    CHECK("T05 push on full (nonblocking) returns -1", bq_push(&q, 99, 0) == -1);

    /* T06: pop from empty queue with timeout_ms==0 returns -1 immediately */
    while (bq_pop(&q, &v, 0) == 0);   /* drain */
    CHECK("T06 pop on empty (nonblocking) returns -1", bq_pop(&q, &v, 0) == -1);

    /* T07: capacity-1 pushes then 1 pop then 1 push (boundary wrap) */
    for (int i = 0; i < 3; i++) bq_push(&q, i, 0);
    bq_pop(&q, &v, 0);
    CHECK("T07 wraparound push succeeds", bq_push(&q, 77, 0) == 0);

    bq_destroy(&q);
}

/* ============================================================
 * T08–T12  TIMEOUT AND NON-BLOCKING EDGE CASES
 * ============================================================ */

static void test_timeouts(void) {
    BoundedQueue q;
    bq_init(&q, 2);
    int v;

    /* T08: pop from empty with 100 ms timeout takes ≥ 90 ms, ≤ 400 ms */
    long t0 = now_ms();
    int r = bq_pop(&q, &v, 100);
    long elapsed = now_ms() - t0;
    CHECK("T08 pop timeout returns -1",  r == -1);
    CHECK("T08 pop timeout timing", elapsed >= 90 && elapsed <= 400);

    /* T09: push to full with 100 ms timeout takes ≥ 90 ms, ≤ 400 ms */
    bq_push(&q, 1, 0);
    bq_push(&q, 2, 0);
    t0 = now_ms();
    r = bq_push(&q, 3, 100);
    elapsed = now_ms() - t0;
    CHECK("T09 push timeout returns -1",  r == -1);
    CHECK("T09 push timeout timing", elapsed >= 90 && elapsed <= 400);

    /* T10: timeout_ms == -1 (infinite) push succeeds when space is freed
     *      We free a slot from a second thread after 50 ms */
    /* (tested below in multi-threaded section as T13) */

    /* T11: zero-capacity queue must be rejected by bq_init */
    BoundedQueue q2;
    CHECK("T11 capacity 0 rejected", bq_init(&q2, 0) == -1);

    /* T12: capacity 1 queue – push then push (full) non-blocking returns -1 */
    BoundedQueue q3;
    bq_init(&q3, 1);
    bq_push(&q3, 42, 0);
    CHECK("T12 cap-1 second push nonblocking", bq_push(&q3, 43, 0) == -1);
    bq_pop(&q3, &v, 0);
    CHECK("T12 cap-1 value correct", v == 42);
    bq_destroy(&q3);

    bq_destroy(&q);
}

/* ============================================================
 * T13–T18  MULTI-THREADED STRESS / ORDERING
 * ============================================================ */

/* thread args */
typedef struct { BoundedQueue *q; int val; int timeout_ms; int result; } PushArg;
typedef struct { BoundedQueue *q; int out; int timeout_ms; int result; } PopArg;

static void *push_thread(void *arg) {
    PushArg *a = arg;
    a->result = bq_push(a->q, a->val, a->timeout_ms);
    return NULL;
}
static void *pop_thread(void *arg) {
    PopArg *a = arg;
    a->result = bq_pop(a->q, &a->out, a->timeout_ms);
    return NULL;
}

/* T13: producer blocked on full queue; consumer unblocks it */
static void test_unblock_push(void) {
    BoundedQueue q;
    bq_init(&q, 2);
    bq_push(&q, 1, 0);
    bq_push(&q, 2, 0);

    PushArg pa = { &q, 3, 500, 0 };
    pthread_t pt;
    pthread_create(&pt, NULL, push_thread, &pa);

    usleep(50000);  /* let producer block */
    int v;
    bq_pop(&q, &v, 0);  /* free a slot */

    pthread_join(pt, NULL);
    CHECK("T13 blocked push unblocked", pa.result == 0);
    bq_destroy(&q);
}

/* T14: consumer blocked on empty queue; producer unblocks it */
static void test_unblock_pop(void) {
    BoundedQueue q;
    bq_init(&q, 2);

    PopArg pa = { &q, 0, 500, 0 };
    pthread_t pt;
    pthread_create(&pt, NULL, pop_thread, &pa);

    usleep(50000);
    bq_push(&q, 99, 0);

    pthread_join(pt, NULL);
    CHECK("T14 blocked pop unblocked", pa.result == 0);
    CHECK("T14 popped value correct",  pa.out == 99);
    bq_destroy(&q);
}

/* T15: N producers, 1 consumer – all items consumed exactly once */
#define T15_N 64
static atomic_int t15_counter = 0;
static void *t15_producer(void *arg) {
    BoundedQueue *q = arg;
    bq_push(q, 1, 2000);
    return NULL;
}
static void test_many_producers(void) {
    BoundedQueue q;
    bq_init(&q, 8);
    pthread_t pts[T15_N];
    for (int i = 0; i < T15_N; i++)
        pthread_create(&pts[i], NULL, t15_producer, &q);

    int consumed = 0, v;
    while (consumed < T15_N) {
        if (bq_pop(&q, &v, 200) == 0) consumed++;
    }
    for (int i = 0; i < T15_N; i++) pthread_join(pts[i], NULL);
    CHECK("T15 all items consumed", consumed == T15_N);
    bq_destroy(&q);
}

/* T16: 1 producer, N consumers – no item consumed twice */
#define T16_N 32
static atomic_int t16_sum = 0;
static void *t16_consumer(void *arg) {
    BoundedQueue *q = arg;
    int v;
    if (bq_pop(q, &v, 2000) == 0)
        atomic_fetch_add(&t16_sum, v);
    return NULL;
}
static void test_many_consumers(void) {
    BoundedQueue q;
    bq_init(&q, 8);
    pthread_t cts[T16_N];
    for (int i = 0; i < T16_N; i++)
        pthread_create(&cts[i], NULL, t16_consumer, &q);
    usleep(10000);
    for (int i = 0; i < T16_N; i++) bq_push(&q, 1, 2000);
    for (int i = 0; i < T16_N; i++) pthread_join(cts[i], NULL);
    CHECK("T16 no double-consume", atomic_load(&t16_sum) == T16_N);
    bq_destroy(&q);
}

/* T17: FIFO ordering under concurrency (single producer, single consumer) */
static void *t17_producer(void *arg) {
    BoundedQueue *q = arg;
    for (int i = 0; i < 128; i++) bq_push(q, i, -1);
    return NULL;
}
static void test_fifo_threaded(void) {
    BoundedQueue q;
    bq_init(&q, 8);
    pthread_t pt;
    pthread_create(&pt, NULL, t17_producer, &q);
    int prev = -1, fifo_ok = 1, v;
    for (int i = 0; i < 128; i++) {
        bq_pop(&q, &v, 2000);
        if (v != prev + 1) fifo_ok = 0;
        prev = v;
    }
    pthread_join(pt, NULL);
    CHECK("T17 FIFO under concurrency", fifo_ok);
    bq_destroy(&q);
}

/* T18: hammering – 8 producers, 8 consumers, 1000 items total */
#define T18_ITEMS 1000
#define T18_THREADS 8
static atomic_int t18_produced = 0;
static atomic_int t18_consumed = 0;
static void *t18_prod(void *arg) {
    BoundedQueue *q = arg;
    while (1) {
        atomic_int i = atomic_fetch_add(&t18_produced, 1);
        if (i >= T18_ITEMS) break;
        bq_push(q, i, -1);
    }
    return NULL;
}
static void *t18_cons(void *arg) {
    BoundedQueue *q = arg;
    int v;
    while (atomic_load(&t18_consumed) < T18_ITEMS) {
        if (bq_pop(q, &v, 50) == 0)
            atomic_fetch_add(&t18_consumed, 1);
    }
    return NULL;
}
static void test_hammer(void) {
    BoundedQueue q;
    bq_init(&q, 16);
    pthread_t prods[T18_THREADS], cons[T18_THREADS];
    for (int i = 0; i < T18_THREADS; i++) {
        pthread_create(&prods[i], NULL, t18_prod, &q);
        pthread_create(&cons[i],  NULL, t18_cons, &q);
    }
    for (int i = 0; i < T18_THREADS; i++) {
        pthread_join(prods[i], NULL);
        pthread_join(cons[i],  NULL);
    }
    CHECK("T18 hammer all items consumed", atomic_load(&t18_consumed) == T18_ITEMS);
    bq_destroy(&q);
}

/* ============================================================
 * T19–T22  BQ_DESTROY AND WAKE-UP SEMANTICS
 * ============================================================ */

/* T19: destroy wakes a blocked pop with return value -2 */
static void *t19_pop(void *arg) {
    BoundedQueue *q = arg;
    int v;
    return (void*)(intptr_t)bq_pop(q, &v, -1);
}
static void test_destroy_wakes_pop(void) {
    BoundedQueue q;
    bq_init(&q, 4);
    pthread_t pt;
    pthread_create(&pt, NULL, t19_pop, &q);
    usleep(30000);
    bq_destroy(&q);
    void *ret;
    pthread_join(pt, &ret);
    CHECK("T19 destroy wakes blocked pop", (intptr_t)ret == -2);
}

/* T20: destroy wakes a blocked push with return value -2 */
static void *t20_push(void *arg) {
    BoundedQueue *q = arg;
    return (void*)(intptr_t)bq_push(q, 1, -1);
}
static void test_destroy_wakes_push(void) {
    BoundedQueue q;
    bq_init(&q, 2);
    bq_push(&q, 1, 0);
    bq_push(&q, 2, 0);
    pthread_t pt;
    pthread_create(&pt, NULL, t20_push, &q);
    usleep(30000);
    bq_destroy(&q);
    void *ret;
    pthread_join(pt, &ret);
    CHECK("T20 destroy wakes blocked push", (intptr_t)ret == -2);
}

/* T21: push/pop after destroy return -2 immediately */
static void test_ops_after_destroy(void) {
    BoundedQueue q;
    bq_init(&q, 4);
    bq_destroy(&q);
    int v;
    CHECK("T21 push after destroy returns -2", bq_push(&q, 1, 0) == -2);
    CHECK("T21 pop  after destroy returns -2", bq_pop(&q, &v, 0) == -2);
}

/* T22: destroy wakes multiple blocked threads simultaneously */
#define T22_WAITERS 8
static atomic_int t22_woken = 0;
static void *t22_waiter(void *arg) {
    BoundedQueue *q = arg;
    int v;
    int r = bq_pop(q, &v, -1);
    if (r == -2) atomic_fetch_add(&t22_woken, 1);
    return NULL;
}
static void test_destroy_wakes_many(void) {
    BoundedQueue q;
    bq_init(&q, 4);
    pthread_t ts[T22_WAITERS];
    for (int i = 0; i < T22_WAITERS; i++)
        pthread_create(&ts[i], NULL, t22_waiter, &q);
    usleep(50000);
    bq_destroy(&q);
    for (int i = 0; i < T22_WAITERS; i++) pthread_join(ts[i], NULL);
    CHECK("T22 all waiters woken on destroy", atomic_load(&t22_woken) == T22_WAITERS);
}

/* ============================================================
 * T23–T26  ADVERSARIAL / HELL-MODE EDGE CASES
 * ============================================================ */

/* T23: spurious wakeup resilience
 *      pthread_cond_timedwait may return 0 even if the condition
 *      is not satisfied. A correct implementation re-checks the
 *      condition in a while loop, not an if. We simulate pressure
 *      by hammering broadcast from a third thread; if the impl
 *      uses `if` instead of `while` it may return success early. */
static atomic_int t23_broadcast_stop = 0;
static void *t23_spammer(void *arg) {
    /* We can't call internal broadcast externally, so instead
     * we flood push/pop with zero-timeout calls on a second
     * queue to create scheduling noise. The real check is that
     * the popping thread must not return success from an empty q. */
    (void)arg;
    return NULL;
}
static void test_spurious_wakeup_guard(void) {
    /* Pop from empty queue with 200 ms timeout must return -1,
     * even if the condvar fires spuriously inside. */
    BoundedQueue q;
    bq_init(&q, 4);
    int v;
    long t0 = now_ms();
    int r = bq_pop(&q, &v, 200);
    long el = now_ms() - t0;
    /* must time out correctly, not return 0 from empty queue */
    CHECK("T23 spurious wakeup guard: must return -1", r == -1);
    CHECK("T23 spurious wakeup guard: timing sane",    el >= 180);
    bq_destroy(&q);
}

/* T24: push exactly at capacity boundary repeatedly
 *      Exercises head/tail wrap-around at the array boundary. */
static void test_wraparound_stress(void) {
    BoundedQueue q;
    bq_init(&q, 3);
    int ok = 1, v;
    /* push 3, pop 1, push 1 – repeat 200 times */
    for (int cycle = 0; cycle < 200; cycle++) {
        for (int i = 0; i < 3; i++)
            if (bq_push(&q, cycle * 3 + i, 0) != 0) { ok = 0; break; }
        if (bq_pop(&q, &v, 0) != 0) { ok = 0; break; }
        if (bq_push(&q, -1, 0) == -1) { ok = 0; break; } /* must find one slot */
        /* drain for next cycle */
        while (bq_pop(&q, &v, 0) == 0);
    }
    CHECK("T24 wraparound stress", ok);
    bq_destroy(&q);
}

/* T25: zero-ms timeout must NEVER block, even under contention
 *      Spin 4 threads doing zero-timeout pushes onto a full queue;
 *      all must return quickly. */
#define T25_THREADS 4
static atomic_int t25_done = 0;
static void *t25_nonblock(void *arg) {
    BoundedQueue *q = arg;
    long t0 = now_ms();
    /* try 100 non-blocking pushes onto a full queue */
    for (int i = 0; i < 100; i++) bq_push(q, i, 0);
    long el = now_ms() - t0;
    if (el < 200) atomic_fetch_add(&t25_done, 1);  /* should be near-instant */
    return NULL;
}
static void test_zero_timeout_never_blocks(void) {
    BoundedQueue q;
    bq_init(&q, 2);
    bq_push(&q, 0, 0);
    bq_push(&q, 1, 0);   /* fill it */
    pthread_t ts[T25_THREADS];
    for (int i = 0; i < T25_THREADS; i++)
        pthread_create(&ts[i], NULL, t25_nonblock, &q);
    for (int i = 0; i < T25_THREADS; i++) pthread_join(ts[i], NULL);
    CHECK("T25 zero-timeout never blocks under contention",
          atomic_load(&t25_done) == T25_THREADS);
    bq_destroy(&q);
}

/* T26: capacity == 1 under heavy concurrency (pathological case)
 *      With only one slot, every push/pop pair is a rendezvous.
 *      Run 32 producers and 32 consumers for 500 ms and confirm
 *      no data is lost or duplicated. */
#define T26_N 32
static atomic_int t26_produced = 0;
static atomic_int t26_consumed = 0;
static atomic_int t26_stop     = 0;
static void *t26_prod(void *arg) {
    BoundedQueue *q = arg;
    while (!atomic_load(&t26_stop)) {
        if (bq_push(q, 1, 50) == 0)
            atomic_fetch_add(&t26_produced, 1);
    }
    return NULL;
}
static void *t26_cons(void *arg) {
    BoundedQueue *q = arg;
    int v;
    while (!atomic_load(&t26_stop)) {
        if (bq_pop(q, &v, 50) == 0)
            atomic_fetch_add(&t26_consumed, 1);
    }
    return NULL;
}
static void test_capacity_one_stress(void) {
    BoundedQueue q;
    bq_init(&q, 1);
    pthread_t ps[T26_N], cs[T26_N];
    for (int i = 0; i < T26_N; i++) {
        pthread_create(&ps[i], NULL, t26_prod, &q);
        pthread_create(&cs[i], NULL, t26_cons, &q);
    }
    usleep(500000);  /* run for 500 ms */
    atomic_store(&t26_stop, 1);
    bq_destroy(&q);
    for (int i = 0; i < T26_N; i++) {
        pthread_join(ps[i], NULL);
        pthread_join(cs[i], NULL);
    }
    int p = atomic_load(&t26_produced);
    int c = atomic_load(&t26_consumed);
    /* produced and consumed should be within 1 of each other
     * (at most one item might be in the queue when we stop) */
    int delta = p - c;
    if (delta < 0) delta = -delta;
    CHECK("T26 cap-1 stress: no items lost or duplicated", delta <= 1);
    printf("      (produced=%d consumed=%d delta=%d)\n", p, c, delta);
}

/* ============================================================
 * MAIN
 * ============================================================ */

int main(void) {
    printf("\n=== Bounded Blocking Queue – Test Suite ===\n\n");

    printf("-- Single-threaded correctness (T01-T07) --\n");
    test_single_threaded();

    printf("\n-- Timeout edge cases (T08-T12) --\n");
    test_timeouts();

    printf("\n-- Multi-threaded (T13-T18) --\n");
    test_unblock_push();
    test_unblock_pop();
    test_many_producers();
    test_many_consumers();
    test_fifo_threaded();
    test_hammer();

    printf("\n-- Destroy semantics (T19-T22) --\n");
    test_destroy_wakes_pop();
    test_destroy_wakes_push();
    test_ops_after_destroy();
    test_destroy_wakes_many();

    printf("\n-- Adversarial hell-mode (T23-T26) --\n");
    test_spurious_wakeup_guard();
    test_wraparound_stress();
    test_zero_timeout_never_blocks();
    test_capacity_one_stress();

    printf("\n=== %d / %d tests passed ===\n\n", passed, total);
    return (passed == total) ? 0 : 1;
}
