#ifndef __THREAD_H
#define __THREAD_H

#include <inttypes.h>
#include <stddef.h>

#define MAX_NUM_THREADS 32
#define QUANTUM         3
#define FIRE_PER_SEC    15
#define REGISTERS_SIZE  (32 + 1)        // 32 GP + 1 SREG
#define RETURN_SIZE      4      // 2 + 2
#define MIN_STACK_SIZE  (REGISTERS_SIZE + RETURN_SIZE)

enum avr_thread_state {
    ats_invalid,
    ats_runnable,
    ats_paused,
    ats_sleeping,
    ats_joined,
    ats_cancelled,
    ats_waiting
};

enum avr_thread_priority {
    atp_noromal,
    atp_critical
};

struct avr_thread {
    volatile enum avr_thread_state state;
    uint8_t        *stack;
    uint16_t        stack_size;
    uint8_t        *sp;
    volatile uint8_t ticks;
    enum avr_thread_priority priority;
    volatile uint16_t sleep_ticks;
    volatile struct avr_thread *run_queue_prev;
    volatile struct avr_thread *run_queue_next;
    volatile struct avr_thread *sleep_queue_next;
    volatile struct avr_thread *wait_queue_next;
    volatile struct avr_thread *next_joined;
#ifdef SANITY
    void           *owning;
#endif
};

struct avr_thread_mutex {
    /*
     * There will be a loop of this form:
     *
     * while (locked == 1) {
     *     ... // This is _not_ a busy waiting!!!
     * }
     *
     * Hence, `locked' must be `volatile'.
     *
     */
    volatile uint8_t locked;
    volatile struct avr_thread *wait_queue;
};

struct avr_thread_semaphore {
    volatile struct avr_thread_mutex *mutex;
    volatile uint8_t lock_count;
    volatile struct avr_thread *wait_queue;
};

struct avr_thread_rwlock {
    volatile struct avr_thread_mutex *mutex;
    volatile struct avr_thread_semaphore *writer_sem;
    volatile struct avr_thread_semaphore *reader_sem;
    volatile int8_t num_reader;
    volatile int8_t num_waiting_reader;
};

struct avr_thread *avr_thread_init(uint16_t main_stack_size,
                                   uint8_t main_priority);

struct avr_thread *avr_thread_create(void (*entry) (void),
                                     uint8_t * stack,
                                     uint16_t stack_size,
                                     uint8_t priority);
#ifdef _EXPORT_INTERNAL
extern volatile struct avr_thread *avr_thread_active_thread;

void            avr_thread_run_queue_push(volatile struct
                                          avr_thread *t);
#endif

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

void            avr_thread_mutex_destory(volatile struct
                                         avr_thread_mutex
                                         *mutex);

void            avr_thread_mutex_lock(volatile struct
                                      avr_thread_mutex
                                      *mutex);

void            avr_thread_mutex_unlock(volatile struct
                                        avr_thread_mutex
                                        *mutex);

// Semaphores
struct avr_thread_semaphore *avr_thread_semaphore_init(int value);

void            avr_thread_semaphore_destroy(volatile struct
                                             avr_thread_semaphore *sem);

void            avr_thread_sem_up(volatile struct avr_thread_semaphore
                                  *sem);

void            avr_thread_sem_down(volatile struct avr_thread_semaphore
                                    *sem);

struct avr_thread_rwlock
               *avr_thread_rwlock_init(void);

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
