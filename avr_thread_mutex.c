#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#define _EXPORT_INTERNAL
#include "avr_thread.h"

static int8_t   avr_thread_atomic_add(int8_t x, int8_t delta);


struct avr_thread_mutex *
avr_thread_mutex_create(void)
{
    struct avr_thread_mutex *mutex;

    mutex = malloc(sizeof(struct avr_thread_mutex));
    if (mutex == NULL) {
        return NULL;
    }
    mutex->locked = 0;
    mutex->wait_queue = NULL;

    return mutex;
}

void
avr_thread_mutex_destory(volatile struct avr_thread_mutex
                               *mutex)
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
avr_thread_mutex_acquire(volatile struct avr_thread_mutex
                               *mutex)
{
    uint8_t         sreg;
    volatile struct avr_thread_context *t,
                   *p;

    if (mutex == NULL) {
        return;
    }

    while (1) {
        sreg = SREG;
        cli();

        if (mutex->locked == 0) {
            mutex->locked = 1;

            SREG = sreg;
            return;
        } else {
            /**
             * //Or, you can do busy waiting here.
             *
             * SREG = sreg;
             * avr_thread_yield();
             * continue;
             *
             */

            t = mutex->wait_queue;
            p = NULL;
            while (t != NULL) {
                p = t;
                t = t->wait_queue_next;
            }
            if (p == NULL) {
                mutex->wait_queue = avr_thread_active_context;
                mutex->wait_queue->wait_queue_next = NULL;
            } else {
                avr_thread_active_context->wait_queue_next =
                    p->wait_queue_next;
                p->wait_queue_next = avr_thread_active_context;
            }
            avr_thread_active_context->state = ats_waiting;

            SREG = sreg;
            avr_thread_yield();
        }
    }
}

void
avr_thread_mutex_release(volatile struct avr_thread_mutex
                               *mutex)
{
    uint8_t         sreg = SREG;
    cli();

    if (mutex == NULL) {
        sei();
        return;
    }

    if (mutex != NULL && mutex->locked == 1) {
        mutex->locked = 0;
    }

    while (mutex->wait_queue != NULL) {
        mutex->wait_queue->state = ats_runnable;
        avr_thread_run_queue_push(mutex->wait_queue);
        mutex->wait_queue = mutex->wait_queue->wait_queue_next;
    }

    SREG = sreg;
    /*
     * You may do an avr_thread_yield() here.
     */
    return;
}

/*
 * avr_thread_semaphore
 */
struct avr_thread_semaphore *
avr_thread_semaphore_create(int value)
{
    struct avr_thread_semaphore *sem;

    if (value < 0) {
        return NULL;
    }

    sem = malloc(sizeof(struct avr_thread_semaphore));
    if (sem == NULL) {
        return NULL;
    }
    sem->lock_count = value;
    sem->mutex = avr_thread_mutex_create();
    if (sem->mutex == NULL) {
        return NULL;
    }
    sem->wait_queue = NULL;

    return sem;
}

/**
 * Consider what will happen if one thread does down() and then destory
 * the semaphore?
 *
 * It would be _your_ fault if you call this function twice on a same
 * semaphore.
 */
void
avr_thread_semaphore_destroy(volatile struct avr_thread_semaphore *sem)
{
    uint8_t         sreg;
    sreg = SREG;
    cli();
    if (sem != NULL) {
        if (sem->mutex != NULL) {
            avr_thread_mutex_destory(sem->mutex);
            sem->mutex = NULL;  /* This will work. */
        }
        free(sem);
        // sem = NULL; /* This will not work */
    }
    SREG = sreg;
    return;
}

void
avr_thread_sem_up(volatile struct avr_thread_semaphore *sem)
{
    if (sem == NULL || sem->mutex == NULL) {
        /*
         * TODO
         * Return error messages.
         * I am thinking of killing the calling thread.
         * It is a pity that I cannot do that.
         */
        return;
    }
    avr_thread_mutex_acquire(sem->mutex);
    ++(sem->lock_count);
    /*
     * Since we have only up() once, we only need to wake up one
     * waiting thread.
     */
    if (sem->wait_queue != NULL) {
        sem->wait_queue->state = ats_runnable;
        avr_thread_run_queue_push(sem->wait_queue);
        sem->wait_queue = sem->wait_queue->wait_queue_next;
    }

    avr_thread_mutex_release(sem->mutex);
}

void
avr_thread_sem_down(volatile struct avr_thread_semaphore *sem)
{
    volatile struct avr_thread_context *t,
                   *p;
    if (sem == NULL || sem->mutex == NULL) {
        /*
         * Return error messages.
         */
        return;
    }

    /*
     * We do not need to cei() here because the reading/writing to the
     * value of semaphore are protected by its mutex.
     */

    /*
     * The flow is:
     *
     * acquire the mutex -> check the value of the semaphore ->
     *
     * a. if the value is 0, release the mutex (so that other threads
     * can acquire the mutex and change the value) -> loop.
     *
     * b. otherwise, decrement the value (this opearting is safe as we
     * are holding the mutex -> release the mutex -> return
     */
    while (1) {
        avr_thread_mutex_acquire(sem->mutex);
        if (sem->lock_count == 0) {
            t = sem->wait_queue;
            p = NULL;
            while (t != NULL) {
                p = t;
                t = t->wait_queue_next;
            }
            if (p == NULL) {
                sem->wait_queue = avr_thread_active_context;
                avr_thread_active_context->wait_queue_next = NULL;
            } else {
                avr_thread_active_context->wait_queue_next =
                    p->wait_queue_next;
                p->wait_queue_next = avr_thread_active_context;
            }
            avr_thread_mutex_release(sem->mutex);
            avr_thread_active_context->state = ats_waiting;
            avr_thread_yield();
        } else {
            --(sem->lock_count);
            avr_thread_mutex_release(sem->mutex);
            return;
        }
    }
}

/*
 * Simulates the atomic addition operation on other architecture.
 */
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

/*
 * The following implementation was adapted from Google's Go programming
 * language.
 */
#define rwmutexMaxReaders 10

struct avr_thread_mutex_rw_lock *
avr_thread_mutex_rw_create(void)
{
    struct avr_thread_mutex_rw_lock *r;
    r = malloc(sizeof(struct avr_thread_mutex_rw_lock));
    if (r == NULL) {
        return NULL;
    }
    r->mutex = avr_thread_mutex_create();
    r->writeSem = avr_thread_semaphore_create(0);
    r->readerSem = avr_thread_semaphore_create(0);
    r->readerCount = 0;
    r->readerWait = 0;
    return r;
}

void
avr_thread_mutex_rw_destroy(volatile struct avr_thread_mutex_rw_lock
                            *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    avr_thread_mutex_destory(rwlock->mutex);
    avr_thread_semaphore_destroy(rwlock->writeSem);
    avr_thread_semaphore_destroy(rwlock->readerSem);
    free(rwlock);
}

void
avr_thread_mutex_rw_rlock(volatile struct avr_thread_mutex_rw_lock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->readerCount, 1) < 0) {
        avr_thread_sem_down(rwlock->readerSem);
    }
}

void
avr_thread_mutex_rw_runlock(volatile struct avr_thread_mutex_rw_lock
                            *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->readerCount, -1) < 0) {
        if (avr_thread_atomic_add(rwlock->readerWait, -1) == 0) {
            avr_thread_sem_up(rwlock->writeSem);
        }
    }
}

void
avr_thread_mutex_rw_wlock(volatile struct avr_thread_mutex_rw_lock *rwlock)
{
    int8_t          r;

    avr_thread_mutex_acquire(rwlock->mutex);

    r = avr_thread_atomic_add(rwlock->readerCount, -rwmutexMaxReaders) +
        rwmutexMaxReaders;
    if (r != 0 && avr_thread_atomic_add(rwlock->readerWait, r) != 0) {
        avr_thread_sem_down(rwlock->writeSem);
    }
}

void
avr_thread_mutex_rw_wunlock(volatile struct avr_thread_mutex_rw_lock
                            *rwlock)
{
    int8_t          r,
                    i;

    r = avr_thread_atomic_add(rwlock->readerCount, rwmutexMaxReaders);
    for (i = 0; i < r; ++i) {
        avr_thread_sem_down(rwlock->readerSem);
    }

    avr_thread_mutex_release(rwlock->mutex);
}
