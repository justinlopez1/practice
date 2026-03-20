#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

// =======================
// H2O Formation Problem
// =======================
//
// You are given two functions:
//
//   void releaseHydrogen();
//   void releaseOxygen();
//
// These functions output "H" and "O" respectively.
//
// Implement synchronization so that:
//
//   - Exactly TWO hydrogen threads and ONE oxygen thread
//     are grouped together to form one water molecule.
//
//   - Threads must block until they are actually used in a molecule.
//
//   - A hydrogen thread may only call releaseHydrogen() if it is part
//     of a complete group of 2 H + 1 O.
//
//   - An oxygen thread may only call releaseOxygen() if it is part
//     of a complete group of 2 H + 1 O.
//
// Valid output for one molecule includes any ordering of:
//   HHO
//   HOH
//   OHH
//
// But invalid examples include:
//   HHH
//   OO
//   HO
//
// Requirements:
//   - multiple hydrogen and oxygen threads may arrive in any order
//   - threads that cannot yet form a complete molecule must block
//   - once enough threads are available, exactly 2 H and 1 O should proceed
//   - after participating in one molecule, a thread returns
//
// Suggested API:
//
// typedef struct
// {
//     // add synchronization state here
// } H2O;
//
// void h2o_init(H2O* h);
// void hydrogen(H2O* h);
// void oxygen(H2O* h);
//
// Notes:
//   - use pthread synchronization primitives
//   - semaphores are a natural fit
//   - condition variables are also possible
//   - think carefully about grouping and reset behavior between molecules

typedef struct
{
    // add synchronization fields here
    sem_t hydrogens;
    sem_t kill_hydrogens;
    pthread_mutex_t mtx;
} H2O;

void releaseHydrogen(void)
{
    printf("H");
}

void releaseOxygen(void)
{
    printf("O");
}

void h2o_init(H2O* h)
{
    sem_init(&h->hydrogens, 0, 0);
    sem_init(&h->kill_hydrogens, 0, 0);
    pthread_mutex_init(&h->mtx, NULL);
}

void hydrogen(H2O* h)
{
    sem_post(&h->hydrogens);

    // wait for other thread to tell us to die
    sem_wait(&h->kill_hydrogens);

    releaseHydrogen();
}

void oxygen(H2O* h)
{
    // wait for two hydrogens to become ready
    pthread_mutex_lock(&h->mtx);
    sem_wait(&h->hydrogens);
    sem_wait(&h->hydrogens);
    pthread_mutex_unlock(&h->mtx);

    // alert that two hydrogens are ready to be killed
    sem_post(&h->kill_hydrogens);
    sem_post(&h->kill_hydrogens);

    releaseOxygen();
}

// =======================
// Test Harness
// =======================

typedef struct
{
    H2O* h2o;
    int is_hydrogen;
} ThreadArg;

void* hydrogen_thread(void* arg)
{
    ThreadArg* a = (ThreadArg*)arg;
    hydrogen(a->h2o);
    return NULL;
}

void* oxygen_thread(void* arg)
{
    ThreadArg* a = (ThreadArg*)arg;
    oxygen(a->h2o);
    return NULL;
}

void run_test(const char* name, int hydrogen_count, int oxygen_count)
{
    printf("%s: ", name);

    H2O h;
    h2o_init(&h);

    int total = hydrogen_count + oxygen_count;
    pthread_t threads[128];
    ThreadArg args[128];

    int idx = 0;

    for (int i = 0; i < hydrogen_count; i++) {
        args[idx].h2o = &h;
        args[idx].is_hydrogen = 1;
        pthread_create(&threads[idx], NULL, hydrogen_thread, &args[idx]);
        idx++;
    }

    for (int i = 0; i < oxygen_count; i++) {
        args[idx].h2o = &h;
        args[idx].is_hydrogen = 0;
        pthread_create(&threads[idx], NULL, oxygen_thread, &args[idx]);
        idx++;
    }

    int molecules = hydrogen_count / 2;
    if (oxygen_count < molecules) {
        molecules = oxygen_count;
    }

    int expected_completed_threads = molecules * 3;

    for (int i = 0; i < expected_completed_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("  expected completed threads: %d", expected_completed_threads);
    printf("\n");
}

int main(void)
{
    run_test("1 molecule", 2, 1);
    printf("\n");

    run_test("2 molecules", 4, 2);
    printf("\n");

    // These intentionally have extra threads that should block forever
    // unless you later add a shutdown mechanism.
    //
    // Uncomment only if you want to observe the behavior manually.
    //
    // run_test("extra hydrogen", 3, 1);
    // printf("\n");
    //
    // run_test("extra oxygen", 2, 2);
    // printf("\n");

    return 0;
}
