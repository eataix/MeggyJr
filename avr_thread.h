#ifndef __THREAD_H
#define __THREAD_H

#include <inttypes.h>
#include <stddef.h>

/*
 * Maximum number of valid threads 
 */
#define MAX_NUM_THREADS 32
/*
 * Quantum size 
 */
#define DEFAULT_QUANTUM         3
/*
 * The number of ticks per second 
 */
#define FIRE_PER_SEC    15
/*
 * 32 GP + 1 SREG 
 */
#define REGISTERS_SIZE  (32 + 1)
/*
 * 2 for self-deconstruct and 2 for entry 
 */
#define RETURN_SIZE      4
/*
 * The stack should at least be able to store the registers and the return 
 * address 
 */
#define MIN_STACK_SIZE  (REGISTERS_SIZE + RETURN_SIZE)

/*
 * States of a thread
 */
enum avr_thread_state {
    ats_invalid,                /* Cannot be run (stopped or
                                 * uninitialised) */
    ats_runnable,               /* Can be run */
    ats_paused,                 /* Pausing, is not runnable until resume() 
                                 */
    ats_sleeping,               /* Sleeping, not runnable until timer runs 
                                 * off */
    ats_joined,                 /* Joined to another thread, is not
                                 * runnable until that thread stops */
    ats_cancelled,              /* Cancelle by a thread */
    ats_waiting                 /* Waiting to lock a mutex or a semaphore */
};

/*
 * Priority of a thread
 *
 * WARNING: If one wants to use this, please make sure starvation is
 * not possible by using avr_thread_sleep() or avr_thread_yield().
 */
enum avr_thread_priority {
    atp_normal,
    atp_important,
    atp_critical
};

struct avr_thread;

struct avr_thread_mutex;

struct avr_thread_semaphore;

struct avr_thread_rwlock;

struct avr_thread *avr_thread_init(uint16_t main_stack_size,
                                   enum avr_thread_priority main_priority);

struct avr_thread *avr_thread_create(void (*entry) (void),
                                     uint8_t * stack,
                                     uint16_t stack_size,
                                     enum avr_thread_priority priority);

void            avr_thread_sleep(uint16_t ticks);

void            avr_thread_yield(void);

void            avr_thread_pause(struct avr_thread *t);

void            avr_thread_resume(struct avr_thread *t);

uint8_t        *avr_thread_tick(uint8_t * saved_sp);

void            avr_thread_save_sp(uint8_t * sp);

int8_t          avr_thread_cancel(struct avr_thread *t);

void            avr_thread_exit(void);

void            avr_thread_join(struct avr_thread *t);

/**
 * Mutual exclusion and semaphore
 */

// Basic mutex
struct avr_thread_mutex *avr_thread_mutex_init(void);

void            avr_thread_mutex_destory(volatile
                                         struct avr_thread_mutex *mutex);

void            avr_thread_mutex_lock(volatile
                                      struct avr_thread_mutex *mutex);

void            avr_thread_mutex_unlock(volatile
                                        struct avr_thread_mutex *mutex);

// Semaphores
struct avr_thread_semaphore *avr_thread_semaphore_init(int value);

void            avr_thread_semaphore_destroy(volatile
                                             struct avr_thread_semaphore
                                             *sem);

void            avr_thread_sem_up(volatile struct avr_thread_semaphore
                                  *sem);

void            avr_thread_sem_down(volatile struct avr_thread_semaphore
                                    *sem);

struct avr_thread_rwlock *avr_thread_rwlock_init(void);

void            avr_thread_rwlock_destroy(volatile struct avr_thread_rwlock
                                          *rwlock);

void            avr_thread_rwlock_rdlock(volatile struct avr_thread_rwlock
                                         *rwlock);

void            avr_thread_rwlock_rdunlock(volatile struct avr_thread_rwlock
                                           *rwlock);

void            avr_thread_rwlock_wrlock(volatile struct avr_thread_rwlock
                                         *rwlock);

void            avr_thread_rwlock_wrunlock(volatile struct avr_thread_rwlock
                                           *rwlock);

#endif
