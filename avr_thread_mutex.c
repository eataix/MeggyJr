#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#define _EXPORT_INTERNAL
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
    struct avr_thread_context *t,
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
                // p->wait_queue_next may be NULL. Does not matter.
                avr_thread_active_context->wait_queue_next =
                    p->wait_queue_next;
                p->wait_queue_next = avr_thread_active_context;
            }
#ifdef DEBUG
            avr_thread_active_context->waiting_for = mutex;
#endif
            avr_thread_active_context->state = ats_waiting;

            SREG = sreg;
            avr_thread_yield();
        }
    }
}

void
avr_thread_basic_mutex_release(struct avr_thread_basic_mutex *mutex)
{
    uint8_t         sreg = SREG;
    struct avr_thread_context *p;
    cli();

    if (mutex != NULL && mutex->locked == 1) {
        mutex->locked = 0;
    }

    /*
     * Wake up the waiting thread.
     */
    p = mutex->wait_queue;
    while (p != NULL) {
        p->state = ats_runnable;
        avr_thread_run_queue_push(p);
        p = p->wait_queue_next;
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
            sem->mutex = NULL;  /* This will work. */
        }
        free(sem);
        // sem = NULL; /* This will not work */
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
    /*
     * Since we have only up() once, we only need to wake up one waiting thread.
     */
    if (sem->wait_queue != NULL) {
        sem->wait_queue->state = ats_runnable;
        avr_thread_run_queue_push(sem->wait_queue);
        sem->wait_queue = sem->wait_queue->wait_queue_next;
    }

    avr_thread_basic_mutex_release(sem->mutex);
}

void
avr_thread_sem_down(struct avr_thread_semaphore *sem)
{
    struct avr_thread_context *t,
                   *p;
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
            /*
             * This is an O(n) operation. This can be improved to O(1) by
             * adding a `tail' attribute to the avr_thread_semaphore struct.
             */
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
#ifdef DEBUG
            avr_thread_active_context->waiting_for = sem;
#endif
            avr_thread_basic_mutex_release(sem->mutex);
            avr_thread_active_context->state = ats_waiting;
            avr_thread_yield();
        } else {
            --(sem->lock_count);
            avr_thread_basic_mutex_release(sem->mutex);
            return;
        }
    }
}

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
    r = malloc(sizeof(struct avr_thread_mutex_rw_lock));
    if (r == NULL) {
        return NULL;
    }
    r->mutex = avr_thread_basic_mutex_create();
    r->writeSem = avr_thread_semaphore_create(0);
    r->readerSem = avr_thread_semaphore_create(0);
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
        if (avr_thread_atomic_add(rwlock->readerWait, -1) == 0) {
            avr_thread_sem_up(rwlock->writeSem);
        }
    }
}


void
avr_thread_mutex_rw_wlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    int8_t          r;

    avr_thread_basic_mutex_acquire(rwlock->mutex);

    r = avr_thread_atomic_add(rwlock->readerCount, -rwmutexMaxReaders) +
        rwmutexMaxReaders;
    if (r != 0 && avr_thread_atomic_add(rwlock->readerWait, r) != 0) {
        avr_thread_sem_down(rwlock->writeSem);
    }
}

void
avr_thread_mutex_rw_wunlock(struct avr_thread_mutex_rw_lock *rwlock)
{
    int8_t          r,
                    i;

    r = avr_thread_atomic_add(rwlock->readerCount, rwmutexMaxReaders);
    for (i = 0; i < r; ++i) {
        avr_thread_sem_down(rwlock->readerSem);
    }

    avr_thread_basic_mutex_release(rwlock->mutex);
}
