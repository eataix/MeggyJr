/*-
 * Copyright (c) 2002-2004  Brian S. Dean <bsd@bdmicro.com>
 * Copyright (c) 2012       Meitian Huang <_@freeaddr.info>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "avr_thread.h"

#define IDLE_THREAD_STACK_SIZE 60

#define MAX_READER 10

/**
 * Data structures
 * ===============
 * Here are the _definition_ of data structures.
 */

/*
 * States of a thread
 */
enum avr_thread_state {
    ats_invalid,                /* Cannot be run (stopped or
                                 * uninitialised) */
    ats_runnable,               /* Can be run */
    ats_paused,                 /* Pausing, is not runnable until
                                 * resume() */
    ats_sleeping,               /* Sleeping, not runnable until timer
                                 * runs off */
    ats_joined,                 /* Joined to another thread, is not
                                 * runnable until that thread stops */
    ats_cancelled,              /* Cancelle by a thread */
    ats_waiting                 /* Waiting to lock a mutex or a
                                 * semaphore */
};

struct avr_thread {
    /*
     * I am WARNING you: DO NOT TOUCH THESE MEMBERS in your program.
     */
    volatile enum avr_thread_state state;
    uint8_t        *stack;      /* Debug purpose */
    uint16_t        stack_size; /* Debug purpose */
    uint8_t        *sp;         /* Saved stack pointer */
    volatile uint8_t ticks;     /* Timer */
    volatile uint8_t quantum;
    volatile enum avr_thread_priority priority;
    volatile uint16_t sleep_ticks;      /* Timer for sleeping */
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
     *     ... // This is _not_ a busy waiting!
     * }
     *
     * Hence, `locked' must be `volatile'.
     *
     */
    volatile uint8_t locked;
    volatile struct avr_thread *wait_queue;
};

struct avr_thread_semaphore {
    /*
     * I know you may not listen, yet do NOT touch these members.
     */
    volatile struct avr_thread_mutex *mutex;
    volatile uint8_t lock_count;
    volatile struct avr_thread *wait_queue;
};

struct avr_thread_rwlock {
    /*
     * I know you may not listen, yet do NOT touch these members.
     */
    volatile struct avr_thread_mutex *mutex;
    volatile struct avr_thread_semaphore *writer_sem;
    volatile struct avr_thread_semaphore *reader_sem;
    volatile int8_t num_reader;
    volatile int8_t num_waiting_reader;
};

/**
 * Variables
 * =========
 */
volatile uint8_t avr_thread_initialised = 0;

static struct avr_thread *avr_thread_main_thread;

static struct avr_thread *avr_thread_idle_thread;

static volatile struct avr_thread *avr_thread_active_thread;

static volatile struct avr_thread *avr_thread_prev_thread;

static volatile struct avr_thread *avr_thread_run_queue;

static volatile struct avr_thread *avr_thread_sleep_queue;

static uint8_t  avr_thread_idle_stack[IDLE_THREAD_STACK_SIZE];

/*
 * Note:
 * These two `functions' are actually assembly procedures. There are
 * in `avr_thread_switch.S'. I put a prototype here so that the compiler
 * can check the type for me.
 */
void            avr_thread_switch_to(uint8_t * new_stack_pointer);
void            avr_thread_switch_to_without_save(uint8_t *
                                                  new_stack_pointer);

/**
 * Prototypes
 * ==========
 */
static void     avr_thread_run_queue_push(volatile struct avr_thread
                                          *t);

static volatile struct avr_thread *avr_thread_run_queue_pop(void);

static void     avr_thread_run_queue_remove(volatile
                                            struct avr_thread *t);

static void     avr_thread_sleep_queue_remove(volatile
                                              struct avr_thread *t);

static void     avr_thread_idle_thread_entry(void);

/*
 * Initialise a particular thread. Do not confuse this with
 * avr_thread_init(), which is used to initialise the whole library.
 */
static void     avr_thread_init_thread(volatile
                                       struct avr_thread *t,
                                       void (*entry) (void),
                                       uint8_t * stack,
                                       uint16_t stack_size,
                                       enum avr_thread_priority
                                       priority);

static void     avr_thread_self_deconstruct(void);

static int8_t   avr_thread_atomic_add(int8_t x, int8_t delta);

/**
 * Implementations
 * ===============
 */

/*
 * Does nothing except yielding.
 */
static void
avr_thread_idle_thread_entry(void)
{
    for (;;) {
        avr_thread_yield();
    }
}

/*
 * Deconstructs the active thread.
 * This function will be called if a thread `return' in its entry function
 * or a thread calls avr_thread_exit().
 */
static void
avr_thread_self_deconstruct(void)
{
    volatile struct avr_thread *t;

    avr_thread_run_queue_remove(avr_thread_active_thread);
    avr_thread_sleep_queue_remove(avr_thread_active_thread);

    t = avr_thread_active_thread->next_joined;

    /*
     * Marks the threads that are joined to the current thread active
     * and push the threads to the run queue.
     */
    while (t != NULL) {
        /*
         * The thread that joined to the current thread may be cancelled
         * by another thread...
         */
        if (t->state == ats_joined) {
            t->state = ats_runnable;
            avr_thread_run_queue_push(t);
        }
        t = t->next_joined;
    }

    avr_thread_active_thread->state = ats_invalid;
    avr_thread_yield();
}

struct avr_thread *
avr_thread_init(uint16_t main_stack_size,
                enum avr_thread_priority main_priority)
{
    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    avr_thread_run_queue = NULL;
    avr_thread_sleep_queue = NULL;

    avr_thread_main_thread = avr_thread_create(NULL, NULL,
                                               main_stack_size,
                                               main_priority);

    avr_thread_idle_thread =
        avr_thread_create(avr_thread_idle_thread_entry,
                          avr_thread_idle_stack,
                          sizeof(avr_thread_idle_stack), atp_normal);

    if (avr_thread_idle_thread == NULL ||
        avr_thread_main_thread == NULL) {
        goto error;
    }

    avr_thread_prev_thread = avr_thread_main_thread;
    avr_thread_active_thread = avr_thread_idle_thread;

    /*
     * Force a context switch so that the data in the main thread is
     * set properly.
     */
    avr_thread_switch_to(avr_thread_active_thread->sp);

    avr_thread_initialised = 1;
    SREG |= ints;
    return avr_thread_main_thread;

  error:
    avr_thread_initialised = 0;
    SREG |= ints;
    return NULL;
}

struct avr_thread *
avr_thread_create(void (*entry) (void), uint8_t * stack,
                  uint16_t stack_size,
                  enum avr_thread_priority priority)
{
    uint8_t         ints;
    struct avr_thread *t;

    ints = SREG & 0x80;
    cli();

    if (stack_size < 36) {
        SREG |= ints;
        return NULL;
    }

    t = malloc(sizeof(struct avr_thread));

    if (t == NULL) {
        SREG |= ints;
        return NULL;
    }

    avr_thread_init_thread(t, entry, stack, stack_size, priority);
    t->state = ats_runnable;
    avr_thread_run_queue_push(t);

    /*
     * if (t->priority > avr_thread_active_thread->priority) {
     * avr_thread_yield(); } 
     */

    SREG |= ints;
    return t;
}

static void
avr_thread_init_thread(volatile struct avr_thread *t,
                       void (*entry) (void), uint8_t * stack,
                       uint16_t stack_size,
                       enum avr_thread_priority priority)
{
    uint8_t         i;

    if (stack_size <= MIN_STACK_SIZE) {
        return;
    }

    if (stack == NULL) {
        stack = (uint8_t *) (SP - stack_size - 2);
    }

    t->stack = stack;
    t->stack_size = stack_size;
    t->sp = &(t->stack[stack_size - 1]);

    /*
     * So that when the thread return from `entry' function, it will
     * go to avr_thread_self_deconstruct() function.
     */
    *t->sp = (uint8_t) (uint16_t) avr_thread_self_deconstruct;
    --(t->sp);
    *t->sp =
        (uint8_t) (((uint16_t) avr_thread_self_deconstruct) >> 8);
    --(t->sp);

    /*
     * This part is mainly from http://www.bdmicro.com/code/threads/.
     */
    *t->sp = (uint8_t) (uint16_t) entry;
    --(t->sp);
    *t->sp = (uint8_t) (((uint16_t) entry) >> 8);
    --(t->sp);
    for (i = 0; i < 33; ++i) {
        if (i == 1) {
            *t->sp = SREG;
        } else {
            *t->sp = 0;
        }
        --(t->sp);
    }

    t->priority = priority;
    t->quantum = DEFAULT_QUANTUM;
    t->ticks = t->quantum;
    t->state = ats_runnable;

    t->next_joined = NULL;
#ifdef SANITY
    t->owning = NULL;
#endif

    return;
}

uint8_t        *
avr_thread_tick(uint8_t * saved_sp)
{
    volatile struct avr_thread *t;

    avr_thread_active_thread->sp = saved_sp;

    /*
     * Check if any sleeping thread can be waken.
     */
    if (avr_thread_sleep_queue != NULL) {
        --(avr_thread_sleep_queue->sleep_ticks);
        while (avr_thread_sleep_queue != NULL &&
               avr_thread_sleep_queue->sleep_ticks == 0) {
            avr_thread_sleep_queue->state = ats_runnable;
            avr_thread_run_queue_push(avr_thread_sleep_queue);
            t = avr_thread_sleep_queue;
            avr_thread_sleep_queue =
                avr_thread_sleep_queue->sleep_queue_next;
            t->sleep_queue_next = NULL;
        }
    }

    /*
     * Peek at the run queue if there is a thread with higher priority,
     * switch to it.
     * Warning: starvation is highly possible.
     */
    if (avr_thread_run_queue->priority >
        avr_thread_active_thread->priority) {
        avr_thread_prev_thread = avr_thread_active_thread;
        avr_thread_active_thread = avr_thread_run_queue_pop();
        avr_thread_run_queue_push(avr_thread_prev_thread);
        avr_thread_prev_thread->ticks =
            avr_thread_prev_thread->quantum;
        avr_thread_active_thread->ticks =
            avr_thread_active_thread->quantum;
    } else {
        --(avr_thread_active_thread->ticks);
        if (avr_thread_active_thread->ticks <= 0) {
            avr_thread_prev_thread = avr_thread_active_thread;
            avr_thread_active_thread = avr_thread_run_queue_pop();
            avr_thread_run_queue_push(avr_thread_prev_thread);
            avr_thread_prev_thread->ticks =
                avr_thread_prev_thread->quantum;
            avr_thread_active_thread->ticks =
                avr_thread_active_thread->quantum;
        }
    }

    return avr_thread_active_thread->sp;
}

void
avr_thread_sleep(uint16_t ticks)
{
    volatile struct avr_thread *t,
                   *p;

    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    /*
     * Sleep queue is empty.
     */
    if (avr_thread_sleep_queue == NULL) {
        avr_thread_sleep_queue = avr_thread_active_thread;
        avr_thread_active_thread->sleep_queue_next = NULL;
        avr_thread_active_thread->sleep_ticks = ticks;
        goto _exit_from_sleep;
    }

    /*
     * Sleep queue is not empty.
     * Find a space
     */
    for (p = NULL, t = avr_thread_sleep_queue; t != NULL;) {
        if (ticks < t->sleep_ticks) {
            t->sleep_ticks -= ticks;
            avr_thread_active_thread->sleep_queue_next = t;
            avr_thread_active_thread->sleep_ticks = ticks;
            if (p != NULL) {
                p->sleep_queue_next = avr_thread_active_thread;
            } else {
                avr_thread_sleep_queue = avr_thread_active_thread;
            }
            goto _exit_from_sleep;
        }
        ticks -= t->sleep_ticks;
        p = t;
        t = t->sleep_queue_next;
    }

    /*
     * Tail of the sleep queue.
     */
    p->sleep_queue_next = avr_thread_active_thread;
    avr_thread_active_thread->sleep_queue_next = NULL;
    avr_thread_active_thread->sleep_ticks = ticks;

  _exit_from_sleep:
    avr_thread_active_thread->state = ats_sleeping;
    avr_thread_yield();
    SREG |= ints;
    return;
}


void
avr_thread_exit(void)
{
    /*
     * Cannot exit the main thread.
     */
    if (avr_thread_active_thread != avr_thread_main_thread) {
        avr_thread_self_deconstruct();
    }
}

/*
 * Cancel a thread.
 * Make it as cancelled and move on.
 */
int8_t
avr_thread_cancel(struct avr_thread *t)
{
    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    /*
     * I do not think a thread should be allowed to cancel itself.
     * I want to minimise the number of ways a thread has to exit.
     * `There should be one-- and preferably only one --obvious way to
     * do it.'
     */
    if (t == NULL || t->state == ats_invalid ||
        t == avr_thread_active_thread) {
        goto error;
    }

    /*
     * Will a sane person cancel a thread that is currently owning
     * a mutex?
     */
#ifdef SANITY
    if (t->owning != NULL) {
        goto error;
    }
#endif

    t->state = ats_cancelled;

    while (t->next_joined != NULL) {
        if (t->next_joined->state == ats_joined) {
            t->next_joined->state = ats_runnable;
            avr_thread_run_queue_push(t->next_joined);
        }
        t->next_joined = t->next_joined->next_joined;
    }

    avr_thread_run_queue_remove(t);
    avr_thread_sleep_queue_remove(t);

    t->state = ats_invalid;
    /*
     * I know Gcc generates warning for the following line of code.
     * 
     * The problem is that if a thread is cancelled, what will happen to
     * its struct?
     */
    free(t);

    SREG |= ints;
    return 0;

  error:
    SREG |= ints;
    return 1;
}

void
avr_thread_pause(struct avr_thread *t)
{
    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    if (t == NULL || t == avr_thread_active_thread) {
        goto exit;
    }

    avr_thread_run_queue_remove(t);
    avr_thread_sleep_queue_remove(t);
    t->state = ats_paused;

  exit:
    SREG |= ints;
    return;
}

void
avr_thread_resume(struct avr_thread *t)
{
    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    if (t == NULL || t->state != ats_paused) {
        goto exit;
    }

    t->state = ats_runnable;
    avr_thread_run_queue_push(t);

    if (t->priority > avr_thread_active_thread->priority) {
        avr_thread_yield();
    }

  exit:
    SREG |= ints;
    return;
}

void
avr_thread_yield(void)
{
    uint8_t         ints;
    volatile struct avr_thread *t;

    ints = SREG & 0x80;
    cli();

    /*
     * This function may be called directly from the active thread.
     * We need to put hte current thraed back to the run queue if the
     * current thread is still runnable (i.e., this function is called
     * by other functions in this library.)
     */
    if (avr_thread_active_thread->state == ats_runnable) {
        avr_thread_run_queue_push(avr_thread_active_thread);
    }

    t = avr_thread_run_queue_pop();

    if (t == avr_thread_active_thread) {
        // Reset the tick
        avr_thread_active_thread->ticks =
            avr_thread_active_thread->quantum;
    } else if (avr_thread_active_thread->state == ats_invalid) {
        /*
         * The active thread is self-deconstructing.
         */
        avr_thread_prev_thread = avr_thread_active_thread;
        avr_thread_active_thread = t;
        free(avr_thread_prev_thread);
        avr_thread_active_thread->ticks =
            avr_thread_active_thread->quantum;
        avr_thread_switch_to_without_save(avr_thread_active_thread->
                                          sp);
    } else {
        avr_thread_prev_thread = avr_thread_active_thread;
        avr_thread_active_thread = t;
        avr_thread_prev_thread->ticks =
            avr_thread_prev_thread->quantum;
        avr_thread_active_thread->ticks =
            avr_thread_active_thread->quantum;
        avr_thread_switch_to(avr_thread_active_thread->sp);
    }

    SREG |= ints;
    return;
}

void
avr_thread_join(struct avr_thread *t)
{
    uint8_t         ints;
    volatile struct avr_thread *r,
                   *p;

    ints = SREG & 0x80;
    cli();

    /*
     * Returns inmeediately if the target thread is not attachable.
     */
    if (t == NULL || t->state == ats_invalid) {
        return;
    }

    p = NULL;
    r = t->next_joined;
    while (r != NULL) {
        p = r;
        r = r->next_joined;
    }

    if (p == NULL) {
        avr_thread_active_thread->next_joined = NULL;
        t->next_joined = avr_thread_active_thread;
    } else {
        avr_thread_active_thread->next_joined = p->next_joined;
        p->next_joined = avr_thread_active_thread;
    }

    avr_thread_active_thread->state = ats_joined;
    avr_thread_yield();

    SREG |= ints;
    return;
}

static void
avr_thread_run_queue_push(volatile struct avr_thread *t)
{
    volatile struct avr_thread *r;

    for (r = avr_thread_run_queue; r != NULL; r = r->run_queue_next) {
        if (t->priority > r->priority) {
            if (r->run_queue_prev != NULL) {
                t->run_queue_prev = r->run_queue_prev;
                r->run_queue_prev->run_queue_next = t;
                r->run_queue_prev = t;
                t->run_queue_next = r;
            } else {
                avr_thread_run_queue = t;
                t->run_queue_next = r;
                t->run_queue_prev = NULL;
                t->run_queue_next->run_queue_prev = t;
            }
            return;
        }

        if (r->run_queue_next == NULL) {
            r->run_queue_next = t;
            t->run_queue_next = NULL;
            t->run_queue_prev = r;
            return;
        }
    }

    avr_thread_run_queue = t;
    t->run_queue_prev = NULL;
    t->run_queue_next = NULL;
}

static volatile struct avr_thread *
avr_thread_run_queue_pop(void)
{
    volatile struct avr_thread *r;

    if (avr_thread_run_queue != NULL) {
        r = avr_thread_run_queue;
        avr_thread_run_queue = r->run_queue_next;
        if (avr_thread_run_queue != NULL) {
            avr_thread_run_queue->run_queue_prev = NULL;
        }
        r->run_queue_next = NULL;
        r->run_queue_prev = NULL;
        return r;
    }

    /*
     * Run the idle thread if the run queue is empty.
     */
    return avr_thread_idle_thread;
}

static void
avr_thread_run_queue_remove(volatile struct avr_thread *t)
{
    volatile struct avr_thread *r;

    if (t == NULL) {
        return;
    }

    for (r = avr_thread_run_queue; r != NULL; r = r->run_queue_next) {
        if (r == t) {
            if (r->run_queue_prev != NULL) {
                r->run_queue_prev->run_queue_next = r->run_queue_next;
            }

            if (r->run_queue_next) {
                r->run_queue_next->run_queue_prev = r->run_queue_prev;
            }
            if (r == avr_thread_run_queue) {
                avr_thread_run_queue = r->run_queue_next;
            }
            r->run_queue_next = NULL;
            r->run_queue_next = NULL;
            return;
        }
    }
    // Not found. Implicit return
}

static void
avr_thread_sleep_queue_remove(volatile struct avr_thread *t)
{
    volatile struct avr_thread *r,
                   *p;

    for (p = NULL, r = avr_thread_sleep_queue; r != NULL;
         p = r, r = r->sleep_queue_next) {
        if (r == t) {
            if (t->sleep_queue_next != NULL) {
                t->sleep_queue_next->sleep_ticks += t->sleep_ticks;
            }
            if (avr_thread_sleep_queue == t) {
                avr_thread_sleep_queue = t->sleep_queue_next;
            }
            if (p != NULL) {
                p->sleep_queue_next = r->sleep_queue_next;
            }
            break;
        }
    }
}

void
avr_thread_save_sp(uint8_t * sp)
{
    /*
     * Do you see why?
     */
    sp += 2;
    avr_thread_prev_thread->sp = sp;
}

/**
 * Mutex
 * =====
 */

struct avr_thread_mutex *
avr_thread_mutex_init(void)
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
avr_thread_mutex_destory(volatile struct avr_thread_mutex *mutex)
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
avr_thread_mutex_lock(volatile struct avr_thread_mutex *mutex)
{
    uint8_t         sreg;
    volatile struct avr_thread *t,
                   *p;

    if (mutex == NULL) {
        return;
    }

    while (1) {
        sreg = SREG;
        cli();

        if (mutex->locked == 0) {
            mutex->locked = 1;
#ifdef SANITY
            avr_thread_active_thread->owning = mutex;
#endif
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
                mutex->wait_queue = avr_thread_active_thread;
                mutex->wait_queue->wait_queue_next = NULL;
            } else {
                avr_thread_active_thread->wait_queue_next =
                    p->wait_queue_next;
                p->wait_queue_next = avr_thread_active_thread;
            }
            avr_thread_active_thread->state = ats_waiting;

            SREG = sreg;
            avr_thread_yield();
        }
    }
}

void
avr_thread_mutex_unlock(volatile struct avr_thread_mutex *mutex)
{
    uint8_t         sreg = SREG;
    cli();

    if (mutex == NULL) {
        SREG = sreg;
        return;
    }

    if (mutex != NULL && mutex->locked == 1) {
        mutex->locked = 0;

#ifdef SANITY
        /*
         * I assume the thread releases the mutex is the one owning the
         * mutex
         */
        avr_thread_active_thread->owning = NULL;
#endif

        while (mutex->wait_queue != NULL) {
            mutex->wait_queue->state = ats_runnable;
            avr_thread_run_queue_push(mutex->wait_queue);
            mutex->wait_queue = mutex->wait_queue->wait_queue_next;
        }
    }

    /*
     * You may do an avr_thread_yield() here.
     */
    return;
}

/**
 * Semaphore
 * =========
 */

struct avr_thread_semaphore *
avr_thread_semaphore_init(int value)
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
    sem->mutex = avr_thread_mutex_init();
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
avr_thread_semaphore_destroy(volatile struct avr_thread_semaphore
                             *sem)
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
        return;
    }
    avr_thread_mutex_lock(sem->mutex);
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

    avr_thread_mutex_unlock(sem->mutex);
}

void
avr_thread_sem_down(volatile struct avr_thread_semaphore *sem)
{
    volatile struct avr_thread *t,
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
        avr_thread_mutex_lock(sem->mutex);
        if (sem->lock_count == 0) {
            t = sem->wait_queue;
            p = NULL;
            while (t != NULL) {
                p = t;
                t = t->wait_queue_next;
            }
            if (p == NULL) {
                sem->wait_queue = avr_thread_active_thread;
                avr_thread_active_thread->wait_queue_next = NULL;
            } else {
                avr_thread_active_thread->wait_queue_next =
                    p->wait_queue_next;
                p->wait_queue_next = avr_thread_active_thread;
            }
            avr_thread_mutex_unlock(sem->mutex);
            avr_thread_active_thread->state = ats_waiting;
            avr_thread_yield();
        } else {
            --(sem->lock_count);
            avr_thread_mutex_unlock(sem->mutex);
            return;
        }
    }
}

/**
 * Readers-writer lock
 * ===================
 */

/*
 * The following implementation was adapted from Google's Go programming
 * language.
 */

/*
 * Simulates the atomic addition operation on other architecture.
 */
static          int8_t
avr_thread_atomic_add(int8_t x, int8_t delta)
{

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        x += delta;
    }
    return x;
}

struct avr_thread_rwlock *
avr_thread_rwlock_init(void)
{
    struct avr_thread_rwlock *r;
    r = malloc(sizeof(struct avr_thread_rwlock));
    if (r == NULL) {
        return NULL;
    }
    r->mutex = avr_thread_mutex_init();
    r->writer_sem = avr_thread_semaphore_init(0);
    r->reader_sem = avr_thread_semaphore_init(0);
    r->num_reader = 0;
    r->num_waiting_reader = 0;
    return r;
}

void
avr_thread_rwlock_destroy(volatile struct avr_thread_rwlock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    avr_thread_mutex_destory(rwlock->mutex);
    avr_thread_semaphore_destroy(rwlock->writer_sem);
    avr_thread_semaphore_destroy(rwlock->reader_sem);
    free(rwlock);
}

void
avr_thread_rwlock_rdlock(volatile struct avr_thread_rwlock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->num_reader, 1) < 0) {
        avr_thread_sem_down(rwlock->reader_sem);
    }
}

void
avr_thread_rwlock_rdunlock(volatile struct avr_thread_rwlock *rwlock)
{
    if (rwlock == NULL) {
        return;
    }
    if (avr_thread_atomic_add(rwlock->num_reader, -1) < 0) {
        if (avr_thread_atomic_add(rwlock->num_waiting_reader, -1) ==
            0) {
            avr_thread_sem_up(rwlock->writer_sem);
        }
    }
}

void
avr_thread_rwlock_wrlock(volatile struct avr_thread_rwlock *rwlock)
{
    int8_t          r;

    avr_thread_mutex_lock(rwlock->mutex);

    r = avr_thread_atomic_add(rwlock->num_reader, -MAX_READER) +
        MAX_READER;
    if (r != 0
        && avr_thread_atomic_add(rwlock->num_waiting_reader,
                                 r) != 0) {
        avr_thread_sem_down(rwlock->writer_sem);
    }
}

void
avr_thread_rwlock_wrunlock(volatile struct avr_thread_rwlock *rwlock)
{
    int8_t          r,
                    i;

    r = avr_thread_atomic_add(rwlock->num_reader, MAX_READER);
    for (i = 0; i < r; ++i) {
        avr_thread_sem_down(rwlock->reader_sem);
    }

    avr_thread_mutex_unlock(rwlock->mutex);
}
