#ifndef __THREAD_H
#define __THREAD_H

#include <inttypes.h>
#include <stddef.h>

/* Maximum number of valid threads */
#define MAX_NUM_THREADS 32 
/* Quantum size */
#define DEFAULT_QUANTUM         3  
/* The number of ticks per second */
#define FIRE_PER_SEC    15 
/* 32 GP + 1 SREG */
#define REGISTERS_SIZE  (32 + 1)
/* 2 for self-deconstruct and 2 for entry */
#define RETURN_SIZE      4
/* The stack should at least be able to store the registers and the
 * return address */
#define MIN_STACK_SIZE  (REGISTERS_SIZE + RETURN_SIZE)

/*
 * States of a thread
 */
typedef enum {
    ats_invalid,    /* Cannot be run (stopped or uninitialised)*/
    ats_runnable,   /* Can be run */
    ats_paused,     /* Pausing, is not runnable until resume() */
    ats_sleeping,   /* Sleeping, not runnable until timer runs off */
    ats_joined,     /* Joined to another thread, is not runnable until
                       that thread stops */
    ats_cancelled,  /* Cancelle by a thread*/
    ats_waiting     /* Waiting to lock a mutex or a semaphore */
} avr_thread_state;

/*
 * Priority of a thread
 *
 * WARNING: If one wants to use this, please make sure starvation is
 * not possible by using avr_thread_sleep() or avr_thread_yield().
 */
typedef enum {
    atp_normal,
    atp_important,
    atp_critical
} avr_thread_priority;

typedef struct avr_thread {
    /*
     * I am WARNING you: DO NOT TOUCH THESE MEMBERS.
     */
    volatile avr_thread_state state;
    uint8_t        *stack;      /* Debug purpose */
    uint16_t        stack_size; /* Debug purpose */
    uint8_t        *sp;         /* Saved stack pointer */
    volatile uint8_t ticks;     /* Timer */
    volatile uint8_t quantum;
    avr_thread_priority priority;
    volatile uint16_t sleep_ticks; /* Timer for sleeping */
    volatile struct avr_thread *run_queue_prev;
    volatile struct avr_thread *run_queue_next;
    volatile struct avr_thread *sleep_queue_next;
    volatile struct avr_thread *wait_queue_next;
    volatile struct avr_thread *next_joined;
#ifdef SANITY
    void           *owning;
#endif
} avr_thread;

/*
 * I know you may not listen, yet do NOT touch these members.
 */
typedef struct{
    /*
     * There will be a loop of this form:
     *
     * while (locked == 1) {
     *     ... // This is _not_ a busy waiting!
     * }
     *
     * Hence, `locked' must be `volatile'.
     *
     */
    volatile uint8_t locked;
    volatile avr_thread *wait_queue;
} avr_thread_mutex ;

typedef struct avr_thread_semaphore{
    /*
     * I know you may not listen, yet do NOT touch these members.
     */
    volatile avr_thread_mutex *mutex;
    volatile uint8_t lock_count;
    volatile avr_thread *wait_queue;
} avr_thread_semaphore ;

typedef struct avr_thread_rwlock {
    /*
     * I know you may not listen, yet do NOT touch these members.
     */
    volatile avr_thread_mutex *mutex;
    volatile avr_thread_semaphore *writer_sem;
    volatile avr_thread_semaphore *reader_sem;
    volatile int8_t num_reader;
    volatile int8_t num_waiting_reader;
} avr_thread_rwlock;

avr_thread *avr_thread_init(uint16_t main_stack_size,
                                   uint8_t main_priority);

avr_thread *avr_thread_create(void (*entry) (void),
                                     uint8_t * stack,
                                     uint16_t stack_size,
                                     uint8_t priority);
#ifdef _EXPORT_INTERNAL
extern volatile avr_thread *avr_thread_active_thread;

void            avr_thread_run_queue_push(volatile struct
                                          avr_thread *t);
#endif

void            avr_thread_sleep(uint16_t ticks);

void            avr_thread_yield(void);

void            avr_thread_pause(avr_thread *t);

void            avr_thread_resume(avr_thread *t);

uint8_t        *avr_thread_tick(uint8_t * saved_sp);

void            avr_thread_save_sp(uint8_t * sp);

int8_t          avr_thread_cancel(avr_thread *t);

void            avr_thread_exit(void);

void            avr_thread_join(avr_thread *t);

/**
 * Mutual exclusion and semaphore
 */

// Basic mutex
avr_thread_mutex *avr_thread_mutex_init(void);

void            avr_thread_mutex_destory(volatile 
                                         avr_thread_mutex
                                         *mutex);

void            avr_thread_mutex_lock(volatile 
                                      avr_thread_mutex
                                      *mutex);

void            avr_thread_mutex_unlock(volatile 
                                        avr_thread_mutex
                                        *mutex);

// Semaphores
avr_thread_semaphore *avr_thread_semaphore_init(int value);

void            avr_thread_semaphore_destroy(volatile 
                                             avr_thread_semaphore *sem);

void            avr_thread_sem_up(volatile avr_thread_semaphore
                                  *sem);

void            avr_thread_sem_down(volatile avr_thread_semaphore
                                    *sem);

avr_thread_rwlock
               *avr_thread_rwlock_init(void);

void            avr_thread_rwlock_destroy(volatile avr_thread_rwlock
                                          *rwlock);

void            avr_thread_rwlock_rdlock(volatile avr_thread_rwlock
                                         *rwlock);

void            avr_thread_rwlock_rdunlock(volatile avr_thread_rwlock
                                           *rwlock);

void            avr_thread_rwlock_wrlock(volatile avr_thread_rwlock
                                         *rwlock);

void            avr_thread_rwlock_wrunlock(volatile avr_thread_rwlock
                                           *rwlock);

#endif
