#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>

#include "avr_thread.h"

struct avr_thread_mutex_basic *
avr_thread_mutex_basic_create(void)
{
    struct avr_thread_mutex_basic *mutex;
    mutex = malloc(sizeof(struct avr_thread_mutex_basic));
    if (mutex == NULL) {
        return NULL;
    }
    mutex->locked = 0;
    return mutex;
}

void
avr_thread_mutex_basic_destory(struct avr_thread_mutex_basic *mutex)
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
avr_thread_mutex_acquire_basic(struct avr_thread_mutex_basic *mutex)
{
    uint8_t         sreg;
    while (1) {
        sreg = SREG;
        cli();
        if (mutex->locked == 0) {
            mutex->locked = 1;
            break;
        } else {
            SREG = sreg;
            avr_thread_yield();
        }
    }
    SREG = sreg;
    return;
}

void
avr_thread_mutex_release_basic(struct avr_thread_mutex_basic *mutex)
{
    uint8_t         sreg = SREG;
    cli();
    if (mutex->locked == 1) {
        mutex->locked = 0;
    }
    SREG = sreg;
    return;
}

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
    sem->mutex = avr_thread_mutex_basic_create();
    if (sem->mutex == NULL) {
        return NULL;
    }
    return sem;
}

void
avr_thread_semaphore_destroy(struct avr_thread_semaphore *sem)
{
    uint8_t         sreg;
    sreg = SREG;
    cli();
    if (sem != NULL) {
        if (sem->mutex != NULL) {
            free(sem->mutex);
        }
        free(sem);
    }
    SREG = sreg;
    return;
}

void
avr_thread_sem_up(struct avr_thread_semaphore *sem)
{
    avr_thread_mutex_acquire_basic(sem->mutex);
    ++(sem->lock_count);
    avr_thread_mutex_release_basic(sem->mutex);
}

void
avr_thread_sem_down(struct avr_thread_semaphore *sem)
{
    avr_thread_mutex_acquire_basic(sem->mutex);
    while (1) {
        if (sem->lock_count == 0) {
            avr_thread_yield();
        } else {
            --(sem->lock_count);
            break;
        }
    }
    avr_thread_mutex_release_basic(sem->mutex);
}
void
avr_thread_mutex_acquire(struct avr_thread_mutex *mutex)
{
    struct avr_thread_context *t,
                   *p;

    while (1) {
        uint8_t         sreg = SREG;
        cli();
        if (mutex->owner == avr_thread_active_context) {
            mutex->lock_count = 1;
            SREG = sreg;
            return;
        } else if (mutex->lock_count != 0) {
            mutex->lock_count = 1;
            SREG = sreg;
            return;
        } else if (avr_thread_active_context->waiting_for == NULL) {
            if (mutex->waiting == NULL) {
                mutex->waiting = avr_thread_active_context;
            } else {
                p = NULL;
                t = mutex->waiting;
                while (t != NULL) {
                    p = t;
                    t = t->next_waiting;
                }
                p->next_waiting = avr_thread_active_context;
                avr_thread_active_context->next_waiting = NULL;
            }
            avr_thread_active_context->waiting_for = mutex;
        }
        SREG = sreg;
        avr_thread_yield();
    }
}

void
avr_thread_mutex_release(struct avr_thread_mutex *mutex)
{
    uint8_t         sreg = SREG;
    cli();

    if (mutex->owner != avr_thread_active_context) {
        SREG = sreg;
        return;
    }

    mutex->lock_count = 0;

    if (mutex->waiting != NULL) {
        mutex->owner = mutex->waiting;
        if (mutex->waiting->next_waiting != NULL) {
            mutex->waiting = mutex->waiting->next_waiting;
        }
    }

    SREG = sreg;
    return;
}

