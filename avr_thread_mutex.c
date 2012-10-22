#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "avr_thread.h"

static int8_t   avr_thread_atomic_add(int8_t x, int8_t delta);

struct avr_thread_basic_mutex *
avr_thread_basic_mutex_create(void)
{
    struct avr_thread_basic_mutex *mutex;
    mutex = malloc(sizeof(struct avr_thread_basic_mutex));
    if (mutex == NULL) {
        return NULL;
    }
    mutex->locked = 0;
    return mutex;
}

/*
 * If a mutex is destoryed and some threads are waiting to acquire it,
 * there will be a deadlock.
 */
void
avr_thread_basic_mutex_destory(struct avr_thread_basic_mutex *mutex)
{
    uint8_t         sreg;
    sreg = SREG;
    cli();
    if (mutex != NULL) {
        free(mutex);
    }
    sreg = SREG;
    return;
}

void
avr_thread_basic_mutex_acquire(struct avr_thread_basic_mutex *mutex)
{
    uint8_t         sreg;
    while (1) {
        /*
         * Yes, I decide to attack this problem by using busy waiting.
         * This, however, may not be as `busy' as one thinks.
         * avr_thread_yield() always switches away from the current
         * thread (unless the active thread is the only runnable one).
         * Hopefully, other threads will release the mutex before this
         * thread is switched to.
         */
        sreg = SREG;
        cli();

        if (mutex == NULL) {
            break;
        }

        if (mutex->locked == 0) {
            mutex->locked = 1;
            break;
        }
        SREG = sreg;
        avr_thread_yield();
    }
    SREG = sreg;
    return;
}

void
avr_thread_basic_mutex_release(struct avr_thread_basic_mutex *mutex)
{
    uint8_t         sreg = SREG;
    cli();

    if (mutex != NULL && mutex->locked == 1) {
        mutex->locked = 0;
    }

    SREG = sreg;
    // avr_thread_yield(); // optional
    return;
}

/*
 * avr_thread_semaphore
 */
struct avr_thread_semaphore *
avr_thread_semaphore_create(int value)
{
    struct avr_thread_semaphore *sem;

    /*
     * Semaphore is always non-negative.
     */
    if (value < 0) {
        return NULL;
    }

    sem = malloc(sizeof(struct avr_thread_semaphore));
    if (sem == NULL) {
        return NULL;
    }
    sem->lock_count = value;
    sem->mutex = avr_thread_basic_mutex_create();
    if (sem->mutex == NULL) {
        return NULL;
    }
    return sem;
}

/*
 * It would be _your_ fault if you call this function twice on a same
 * semaphore.
 */
void
avr_thread_semaphore_destroy(struct avr_thread_semaphore *sem)
{
    uint8_t         sreg;
    sreg = SREG;
    cli();
    if (sem != NULL) {
        if (sem->mutex != NULL) {
            avr_thread_basic_mutex_destory(sem->mutex);
        }
        free(sem);
    }
    SREG = sreg;
    return;
}

void
avr_thread_sem_up(struct avr_thread_semaphore *sem)
{
    if (sem == NULL || sem->mutex == NULL) {
        /*
         * TODO
         * Return error messages.
         */
        return;
    }
    avr_thread_basic_mutex_acquire(sem->mutex);
    ++(sem->lock_count);
    avr_thread_basic_mutex_release(sem->mutex);
}

void
avr_thread_sem_down(struct avr_thread_semaphore *sem)
{
    if (sem == NULL || sem->mutex == NULL) {
        /*
         * TODO
         * Return error messages.
         */
        return;
    }

    /*
     * acquire the mutex -> check the value ->
     * a. if the value is 0, release the mutex (so that other threads
     * can acquire the mutex and change the value) -> start over.
     * b. otherwise, decrement the value (this opearting is safe as we
     * are holding the mutex -> release the mutex -> return
     */
    while (1) {
        avr_thread_basic_mutex_acquire(sem->mutex);
        if (sem->lock_count == 0) {
            avr_thread_basic_mutex_release(sem->mutex);
            avr_thread_yield();
        } 
        --(sem->lock_count);
        break;
    }
    avr_thread_basic_mutex_release(sem->mutex);
}

struct avr_thread_mutex_rw_lock {
    struct avr_thread_basic_mutex *mutex;
    struct avr_thread_semaphore *writeSem;
    struct avr_thread_semaphore *readerSem;
    int8_t          readerCount;
    int8_t          readerWait;
};

static          int8_t
avr_thread_atomic_add(int8_t x, int8_t delta)
{
    int             i;
    i = SREG & 0x80;
    cli();
    x += delta;
    SREG = i;
    return x;
}

#define rwmutexMaxReaders 10

struct avr_thread_mutex_rw_lock *
avr_thread_mutex_rw_create(void)
{
    struct avr_thread_mutex_rw_lock *r;
    r = malloc(sizeof(struct avr_thread_mutex));

    r->mutex = avr_thread_basic_mutex_create();
    r->writeSem = avr_thread_semaphore_create(rwmutexMaxReaders);
    r->readerSem = avr_thread_semaphore_create(rwmutexMaxReaders);
    r->readerCount = 0;
    r->readerWait = 0;
    return r;
}

void
avr_thread_mutex_rw_destroy(struct avr_thread_mutex_rw_lock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    avr_thread_basic_mutex_destory(rwlock->mutex);
    avr_thread_semaphore_destroy(rwlock->writeSem);
    avr_thread_semaphore_destroy(rwlock->readerSem);
    free(rwlock);
}

void
avr_thread_mutex_rw_rlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->readerCount, 1) < 0) {
        avr_thread_sem_down(rwlock->readerSem);
    }
}

void
avr_thread_mutex_rw_runlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->readerCount, -1) < 0) {
        if (avr_thread_atomic_add(rwlock->readerWait, 1) < 0) {
            avr_thread_sem_up(rwlock->readerSem);
        }
    }
}


void
avr_thread_mutex_rw_wlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    avr_thread_basic_mutex_acquire(rwlock->mutex);
    int8_t          r =
        avr_thread_atomic_add(rwlock->readerCount, -rwmutexMaxReaders)
        + rwmutexMaxReaders;
    if (r != 0 && avr_thread_atomic_add(rwlock->readerWait, r) != 0) {
        avr_thread_sem_down(rwlock->writeSem);
    }
}

void
avr_thread_mutex_rw_wunlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    int8_t          r =
        avr_thread_atomic_add(rwlock->readerCount, rwmutexMaxReaders);
    uint8_t         i;
    for (i = 0; i < r; ++i) {
        avr_thread_sem_down(rwlock->writeSem);
    }
    avr_thread_basic_mutex_release(rwlock->mutex);
}

