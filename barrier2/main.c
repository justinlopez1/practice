#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

// =======================
// Reusable Barrier Problem
// =======================
//
// Implement a reusable barrier for N threads.
//
// Requirements:
//
//   - The barrier is initialized with a fixed number of threads: threshold
//   - Each thread calls barrier_wait() when it reaches the barrier
//   - No thread may return from barrier_wait() until exactly threshold
//     threads have arrived
//   - Once threshold threads have arrived, all of them may proceed
//   - The barrier must be reusable, meaning it should work correctly
//     again on the next round
//
// Example:
//
//   threshold = 3
//
//   Round 1:
//     thread A calls barrier_wait()   -> blocks
//     thread B calls barrier_wait()   -> blocks
//     thread C calls barrier_wait()   -> now A, B, C all proceed
//
//   Round 2:
//     thread A calls barrier_wait()   -> blocks
//     thread B calls barrier_wait()   -> blocks
//     thread C calls barrier_wait()   -> now A, B, C all proceed
//
// Constraints:
//
//   - multiple threads may arrive in any order
//   - no busy-wait spinning
//   - barrier must not break across rounds
//   - do not let threads from round 2 leak into round 1 release logic
//
// Suggested API:
//
// typedef struct
// {
//     // add synchronization state here
// } Barrier;
//
// void barrier_init(Barrier* b, int threshold);
// void barrier_wait(Barrier* b);
//
// Notes:
//   - think carefully about what state must reset each round
//   - think carefully about whether one phase is enough
//   - use pthread synchronization primitives
//   - condition variables are one possible approach
//   - semaphores are also possible
//
// Test expectation:
//   - threads should print "before" first, block at the barrier,
//     and only print "after" once a full group has arrived

typedef struct
{
    // add synchronization fields here
    int threshold;

    size_t waiting_count;

    pthread_mutex_t mtx;
    sem_t sem;

} Barrier;

void barrier_init(Barrier* b, int threshold)
{
    b->threshold = threshold;
    b->waiting_count = 0;
    pthread_mutex_init(&b->mtx, NULL);
    sem_init(&b->sem, 0, 0);
}

void barrier_wait(Barrier* b)
{
    // acquire mutex
    pthread_mutex_lock(&b->mtx);

    b->waiting_count++;

    // check count
    // if reached then set to 0 and wakeup other blocked threads
    if (b->waiting_count >= b->threshold) {
        // set to 0
        b->waiting_count = 0;

        // broadcast
        for (size_t i = 0; i < b->threshold; i++) {
            sem_post(&b->sem);
        }

    }

    pthread_mutex_unlock(&b->mtx);

    // if not yet reached just increment and block
    sem_wait(&b->sem);
}

#define THREADS 4
#define ROUNDS  2000

static pthread_mutex_t check_mtx = PTHREAD_MUTEX_INITIALIZER;
static int before_count[ROUNDS];
static int after_count[ROUNDS];
static int failures = 0;

typedef struct
{
    Barrier* barrier;
    int id;
    int rounds;
} ThreadArg;

static void random_delay(void)
{
    usleep(rand() % 200);
}

void* worker_thread(void* arg)
{
    ThreadArg* a = (ThreadArg*)arg;

    for (int r = 0; r < a->rounds; r++) {
        random_delay();

        pthread_mutex_lock(&check_mtx);
        before_count[r]++;
        pthread_mutex_unlock(&check_mtx);

        barrier_wait(a->barrier);

        pthread_mutex_lock(&check_mtx);

        // If barrier is correct, all THREADS must have reached "before"
        // for this round before anyone can get to "after".
        if (before_count[r] != THREADS) {
            failures++;
            printf("FAIL: thread %d got past barrier on round %d with before_count=%d\n",
                   a->id, r, before_count[r]);
        }

        after_count[r]++;
        pthread_mutex_unlock(&check_mtx);

        random_delay();
    }

    return NULL;
}

int main(void)
{
    srand((unsigned)time(NULL));

    Barrier b;
    barrier_init(&b, THREADS);

    pthread_t threads[THREADS];
    ThreadArg args[THREADS];

    for (int i = 0; i < THREADS; i++) {
        args[i].barrier = &b;
        args[i].id = i;
        args[i].rounds = ROUNDS;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int round_count_failures = 0;
    for (int r = 0; r < ROUNDS; r++) {
        if (before_count[r] != THREADS || after_count[r] != THREADS) {
            round_count_failures++;
            printf("Round %d counts wrong: before=%d after=%d\n",
                   r, before_count[r], after_count[r]);
        }
    }

    if (failures == 0 && round_count_failures == 0) {
        printf("No failures observed\n");
    } else {
        printf("Observed failures=%d round_count_failures=%d\n",
               failures, round_count_failures);
    }

    return 0;
}
