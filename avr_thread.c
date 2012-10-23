#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_thread.h"

#define IDLE_THREAD_STACK_SIZE 64

/*
 * Variables
 */
struct avr_thread_context *avr_thread_active_context;

static struct avr_thread_context *avr_thread_main_context;
static struct avr_thread_context *avr_thread_idle_context;

static struct avr_thread_context *avr_thread_prev_context;

static uint8_t  avr_thread_idle_stack[IDLE_THREAD_STACK_SIZE];

static struct avr_thread_context *avr_thread_run_queue;
static struct avr_thread_context *avr_thread_sleep_queue;

static struct avr_thread_context avr_thread_threads[MAX_NUM_THREADS];

/*
 * Prototypes
 */
void            avr_thread_switch_to(uint8_t * new_stack_pointer);

static void     avr_thread_run_queue_push(struct avr_thread_context *t);

static struct avr_thread_context
               *avr_thread_run_queue_pop(void);

static void     avr_thread_run_queue_remove(struct avr_thread_context *t);

static void     avr_thread_sleep_queue_remove(struct avr_thread_context
                                              *t);

static void     avr_thread_idle_entry(void);

static void     avr_thread_init_thread(struct avr_thread_context *t,
                                       void (*entry) (void),
                                       uint8_t * stack,
                                       uint16_t stack_size,
                                       uint8_t priority);

static void     avr_thread_self_deconstruct(void);

/*
 * Let the current active thread sleep for `ticks' ticks.
 */
void
avr_thread_sleep(uint16_t ticks)
{
    struct avr_thread_context *t,
                   *p;

    uint8_t         ints;

    ints = SREG & 0x80;
    cli();

    /*
     * Sleep queue is empty.
     */
    if (avr_thread_sleep_queue == NULL) {
        avr_thread_sleep_queue = avr_thread_active_context;
        avr_thread_active_context->sleep_queue_next = NULL;
        avr_thread_active_context->sleep_ticks = ticks;
        goto _exit_from_sleep;
    }

    /*
     * Sleep queue is not empty.
     * Find a space
     */
    for (p = NULL, t = avr_thread_sleep_queue; t != NULL;) {
        if (ticks < t->sleep_ticks) {
            t->sleep_ticks -= ticks;
            avr_thread_active_context->sleep_queue_next = t;
            avr_thread_active_context->sleep_ticks = ticks;
            if (p != NULL) {
                p->sleep_queue_next = avr_thread_active_context;
            } else {
                // Unlikely to happen
                avr_thread_sleep_queue = avr_thread_active_context;
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
    p->sleep_queue_next = avr_thread_active_context;
    avr_thread_active_context->sleep_queue_next = NULL;
    avr_thread_active_context->sleep_ticks = ticks;

  _exit_from_sleep:
    avr_thread_active_context->state = ats_sleeping;
    avr_thread_yield();
    SREG |= ints;
    return;
}

void
avr_thread_tick(void)
{
    uint8_t         should_yield;
    struct avr_thread_context *t;

    should_yield = 0;

    /*
     * Check if any sleeping thread can be waken.
     */
    if (avr_thread_sleep_queue != NULL) {
        --(avr_thread_sleep_queue->sleep_ticks);
        if (avr_thread_sleep_queue->sleep_ticks == 0) {
            while (avr_thread_sleep_queue != NULL &&
                   avr_thread_sleep_queue->sleep_ticks == 0) {
                avr_thread_sleep_queue->state = ats_runnable;
                avr_thread_run_queue_push(avr_thread_sleep_queue);
                /*
                 * If a thread that has higher priority is wakend,
                 * switch to it.
                 */
                if (avr_thread_sleep_queue->priority >
                    avr_thread_active_context->priority) {
                    should_yield = 1;
                }
                t = avr_thread_sleep_queue;
                avr_thread_sleep_queue =
                    avr_thread_sleep_queue->sleep_queue_next;
                t->sleep_queue_next = NULL;
            }
        }
    }

    if (should_yield == 1) {
        avr_thread_active_context->state = ats_runnable;
        avr_thread_yield();
    } else {
        --(avr_thread_active_context->ticks);
        if (avr_thread_active_context->ticks == 0) {
            avr_thread_active_context->state = ats_runnable;
            avr_thread_yield();
        }
    }
}

static void
avr_thread_idle_entry(void)
{
    for (;;) {
        avr_thread_yield();
    }
}

/*
 * Destruct the active thread.
 */
static void
avr_thread_self_deconstruct(void)
{
    struct avr_thread_context *t;

    avr_thread_run_queue_remove(avr_thread_active_context);
    avr_thread_sleep_queue_remove(avr_thread_active_context);

    t = avr_thread_active_context->next_joined;

    while (t != NULL) {
        t->state = ats_runnable;
        avr_thread_run_queue_push(t);
        t = t->next_joined;
    }

    avr_thread_active_context->state = ats_invalid;
    avr_thread_yield();
}

struct avr_thread_context *
avr_thread_init(uint16_t main_stack_size, uint8_t main_priority)
{
    uint8_t         i,
                    ints;

    ints = SREG & 0x80;
    cli();

    avr_thread_run_queue = NULL;
    avr_thread_sleep_queue = NULL;

    for (i = 0; i < MAX_NUM_THREADS; ++i) {
        avr_thread_threads[i].state = ats_invalid;
    }

    // Initialise the idle context.
    avr_thread_idle_context = avr_thread_create(avr_thread_idle_entry,
                                                avr_thread_idle_stack,
                                                sizeof
                                                (avr_thread_idle_stack),
                                                atp_noromal);

    avr_thread_run_queue_remove(avr_thread_idle_context);

    avr_thread_main_context = avr_thread_create(NULL, NULL,
                                                main_stack_size,
                                                main_priority);

    avr_thread_prev_context = avr_thread_main_context;
    avr_thread_active_context = avr_thread_idle_context;

    // Force a context switch.
    avr_thread_switch_to(avr_thread_active_context->sp);

    SREG |= ints;

    return avr_thread_main_context;
}

struct avr_thread_context *
avr_thread_create(void (*entry) (void), uint8_t * stack,
                  uint16_t stack_size, uint8_t priority)
{
    uint8_t         i,
                    ints;
    struct avr_thread_context *t;

    ints = SREG & 0x80;
    cli();

    if (stack_size < 36) {
        SREG |= ints;
        return NULL;
    }

    for (i = 0; i < MAX_NUM_THREADS; ++i) {
        if (avr_thread_threads[i].state == ats_invalid) {
            break;
        }
    }

    if (i == MAX_NUM_THREADS) {
        SREG |= ints;
        return NULL;
    }

    t = &(avr_thread_threads[i]);
    avr_thread_init_thread(t, entry, stack, stack_size, priority);
    avr_thread_run_queue_push(t);
    avr_thread_threads[i].state = ats_runnable;

    if (t->priority > avr_thread_active_context->priority) {
        avr_thread_yield();
    }

    SREG |= ints;
    return t;
}

static void
avr_thread_init_thread(struct avr_thread_context *t, void (*entry) (void),
                       uint8_t * stack, uint16_t stack_size,
                       uint8_t priority)
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
    *t->sp = (uint8_t) (((uint16_t) avr_thread_self_deconstruct) >> 8);
    --(t->sp);

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
    t->ticks = QUANTUM;
    t->state = ats_runnable;

    return;
}

void
avr_thread_run_queue_push(struct avr_thread_context *t)
{
    struct avr_thread_context *r;

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

static struct avr_thread_context *
avr_thread_run_queue_pop(void)
{
    struct avr_thread_context *r;

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
    return avr_thread_idle_context;
}

static void
avr_thread_run_queue_remove(struct avr_thread_context *t)
{
    struct avr_thread_context *r;

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
avr_thread_sleep_queue_remove(struct avr_thread_context *t)
{
    struct avr_thread_context *r,
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
avr_thread_exit(void)
{
    /*
     * Cannot exit the main thread.
     */
    if (avr_thread_active_context != avr_thread_main_context) {
        avr_thread_self_deconstruct();
    }
}

/*
 * Cancel a thread.
 * Make it as cancelled and move on.
 */
void
avr_thread_cancel(struct avr_thread_context *t)
{
    uint8_t         ints;

    /*
     * You cannot cancel a thread that is
     * 1. invalid or
     * 2. the active thread (use avr_thread_exit() instead)
     */
    if (t == NULL || t == avr_thread_active_context) {
        return;
    }

    ints = SREG & 0x80;
    cli();

    t->state = ats_cancelled;

    SREG |= ints;
}

void
avr_thread_pause(struct avr_thread_context *t)
{
    uint8_t         ints;

    if (t == NULL) {
        return;
    }

    ints = SREG & 0x80;
    cli();

    avr_thread_run_queue_remove(t);
    avr_thread_sleep_queue_remove(t);
    t->state = ats_paused;
    if (t == avr_thread_active_context) {
        avr_thread_yield();
    }

    SREG |= ints;
}

void
avr_thread_resume(struct avr_thread_context *t)
{
    uint8_t         ints;

    if (t == NULL || t->state != ats_paused) {
        return;
    }

    ints = SREG & 0x80;
    cli();

    avr_thread_run_queue_push(t);
    t->state = ats_runnable;
    if (t->priority > avr_thread_active_context->priority) {
        avr_thread_yield();
    }

    SREG |= ints;
}

void
avr_thread_yield(void)
{
    uint8_t         ints;
    struct avr_thread_context *t;

    ints = SREG & 0x80;
    cli();

    if (avr_thread_active_context->state == ats_runnable) {
        avr_thread_run_queue_push(avr_thread_active_context);
    }

    t = avr_thread_run_queue_pop();

    if (t->state == ats_cancelled) {
        avr_thread_self_deconstruct();
    }

    if (t == avr_thread_active_context) {
        // Reset the tick
        avr_thread_active_context->ticks = QUANTUM;
    } else {
        avr_thread_prev_context = avr_thread_active_context;
        avr_thread_active_context = t;
        avr_thread_prev_context->ticks = QUANTUM;
        avr_thread_active_context->ticks = QUANTUM;
        avr_thread_switch_to(avr_thread_active_context->sp);
    }

    SREG |= ints;
    return;
}

void
avr_thread_save_sp(uint8_t * sp)
{
    sp += 2;
    avr_thread_prev_context->sp = sp;
}

void
avr_thread_join(struct avr_thread_context *t)
{
    uint8_t         ints;
    struct avr_thread_context *c,
                   *p;

    ints = SREG & 0x80;
    cli();

    /*
     * Returns inmeediately if the target thread is not attachable.
     */
    if (t == NULL || t->state == ats_invalid) {
        return;
    }

    for (p = NULL, c = t->next_joined; c != NULL;
         p = c, c = c->next_joined) {
    }

    if (p == NULL) {
        avr_thread_active_context->next_joined = NULL;
        t->next_joined = avr_thread_active_context;
    } else {
        avr_thread_active_context->next_joined = p->next_joined;
        p->next_joined = avr_thread_active_context;
    }

    avr_thread_active_context->state = ats_joined;
    avr_thread_yield();

    SREG |= ints;
    return;
}
