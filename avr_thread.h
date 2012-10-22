#ifndef __THREAD_H
#define __THREAD_H

#include <inttypes.h>

#define MAX_NUM_THREADS 32
#define QUANTUM         10
#define FIRE_PER_SEC    10
#define REGISTERS_SIZE  (32 + 1)        // 32 GP + 1 SREG
#define RETURN_SIZE      4      // 2 + 2
#define MIN_STACK_SIZE  (REGISTERS_SIZE + RETURN_SIZE)

enum avr_thread_state {
    ats_invalid,
    ats_runnable,
    ats_paused,
    ats_sleeping,
    ats_joined,
    ats_cancelled
};

enum avr_thread_priority {
    atp_noromal,
    atp_critical
};

struct avr_thread_context {
    enum avr_thread_state state;
    uint8_t        *stack;
    uint16_t        stack_size;
    uint8_t        *sp;
    uint8_t         ticks;
    uint8_t         priority;
    uint16_t        sleep_ticks;
    struct avr_thread_context *run_queue_prev;
    struct avr_thread_context *run_queue_next;
    struct avr_thread_context *sleep_queue_next;
    struct avr_thread_context *next_waiting;
    struct avr_thread_context *next_joined;
    void           *waiting_for;
};

struct avr_thread_mutex {
    uint8_t         lock_count;
    struct avr_thread_context *owner;
    struct avr_thread_context *waiting;
};

struct avr_thread_basic_mutex {
    uint8_t         locked;
};

struct avr_thread_semaphore {
    uint8_t         lock_count;
    struct avr_thread_basic_mutex *mutex;
};

extern struct avr_thread_context *avr_thread_active_context;

struct avr_thread_context *avr_thread_init(uint16_t main_stack_size,
                                           uint8_t main_priority);

struct avr_thread_context *avr_thread_create(void (*entry) (void),
                                             uint8_t * stack,
                                             uint16_t stack_size,
                                             uint8_t priority);

void            avr_thread_sleep(uint16_t ticks);

void            avr_thread_yield(void);

void            avr_thread_pause(struct avr_thread_context *t);

void            avr_thread_resume(struct avr_thread_context *t);

void            avr_thread_tick(void);

void            avr_thread_save_sp(uint8_t * sp);

void            avr_thread_cancel(struct avr_thread_context *t);

void            avr_thread_exit(void);

void            avr_thread_join(struct avr_thread_context *t);

/**
 * Mutual exclusion and semaphore
 */

// Basic mutex
struct avr_thread_basic_mutex *avr_thread_basic_mutex_create(void);

void            avr_thread_basic_mutex_destory(struct
                                               avr_thread_basic_mutex
                                               *mutex);

void            avr_thread_basic_mutex_acquire(struct
                                               avr_thread_basic_mutex
                                               *mutex);

void            avr_thread_basic_mutex_release(struct
                                               avr_thread_basic_mutex
                                               *mutex);

// Semaphores
struct avr_thread_semaphore *avr_thread_semaphore_create(int value);

void            avr_thread_semaphore_destroy(struct avr_thread_semaphore
                                             *sem);

void            avr_thread_sem_up(struct avr_thread_semaphore *sem);

void            avr_thread_sem_down(struct avr_thread_semaphore *sem);

// Mutex
void            avr_thread_mutex_acquire(struct avr_thread_mutex *mutex);

void            avr_thread_mutex_release(struct avr_thread_mutex *mutex);

struct avr_thread_mutex_rw_lock
               *avr_thread_mutex_rw_create(void);

void            avr_thread_mutex_rw_destroy(struct avr_thread_mutex_rw_lock
                                            *rwlock);

void            avr_thread_mutex_rw_rlock(struct avr_thread_mutex_rw_lock
                                          *rwlock);

void            avr_thread_mutex_rw_runlock(struct avr_thread_mutex_rw_lock
                                            *rwlock);

void            avr_thread_mutex_rw_wlock(struct avr_thread_mutex_rw_lock
                                          *rwlock);

void            avr_thread_mutex_rw_wunlock(struct avr_thread_mutex_rw_lock
                                            *rwlock);

#endif
