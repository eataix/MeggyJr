/*-
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

#ifndef __THREAD_H
#define __THREAD_H

#include <inttypes.h>
#include <stddef.h>

/**
 * Constants & Parameters
 * =====================+
 */

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
#define FIRE_PER_SEC    20
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

/**
 * Data structures
 * ===============
 */

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

extern volatile uint8_t avr_thread_initialised;

/*
 * Here are the major structs. Yes, these four lines are everything a
 * programmer needs to know.
 *
 * You will not be able to put them on the stack (i.e., you cannot
 * have:
 *
 *  ...
 *  struct avr_thread foo;
 *  ...
 *
 * in your code because the compiler does not know the size.
 * Please use:
 *
 *  ...
 *  struct avr_thread *foo;
 *  foo = avr_thread_create(...);
 *  ...
 *
 * instead.
 *
 * They are deliberately written as opaque data types to prevent
 * programmers from touching members of these structs in their programs.
 *
 * Note:
 * I know even I work hard to hide members from programmers, programmers
 * still can access the members by using pointer arithmetic...
 */

struct avr_thread;

struct avr_thread_mutex;

struct avr_thread_semaphore;

struct avr_thread_rwlock;

/**
 * Basic Operations
 * ================
 */

/*
 * Initialises the library.
 * Must be called before using any other function.
 */
struct avr_thread *avr_thread_init(uint16_t main_stack_size, enum avr_thread_priority
                                   main_priority);

/*
 * Create a new thread. The new threads will start at the `entry'
 * funtion after creation.
 */
struct avr_thread *avr_thread_create(void (*entry) (void),
                                     uint8_t * stack,
                                     uint16_t stack_size,
                                     enum avr_thread_priority
                                     priority);
/*
 * Sleeps the current threads for specified ticks.
 */
void            avr_thread_sleep(uint16_t ticks);

/*
 * Explicitly gives away the CPU. 
 *
 * Note: likely its Java counterpart, this operation always results in
 * a switch if there are other runnable threads.
 */
void            avr_thread_yield(void);

/*
 * Pause a certain thread.
 *
 * Note:
 *
 * 1. the active thread cannot be paused by itself because allowing
 * that will result in progress issues (the program is likley to stuck).
 * Use avr_thread_sleep() instead.
 *
 * 2. It is the programmer's responsibility to ensure that no deadlock
 * is possible.
 */
void            avr_thread_pause(struct avr_thread *t);

/*
 * Resume the execution of a thread.
 */
void            avr_thread_resume(struct avr_thread *t);

/*
 * Do NOT call this function in your program!
 *
 * It has specific purposes and should _only_ be called by the ISR.
 *
 * This function is responsible for:
 *  1. ticking the sleeping threads (wake them up if their timer is off)
 *  2. ticking the running thread
 *  3. returning the stack pointer of the thread that should execute
 *  after ISR returns.
 */
uint8_t        *avr_thread_tick(uint8_t * saved_sp);

/*
 * Do NOT call this function in your programe!
 *
 * This function should only be called by the assembly procedure in
 * `avr_thread_switch.S'.
 */
void            avr_thread_save_sp(uint8_t * sp);

/*
 * Cancels a thread.
 *
 * WARNING:
 *
 * This function should be _avoided_ because:
 *
 * 1. The implementation is naive. Cancelling a thread will directly
 * remove the thread from records and remove all information about the
 * thread from memory. Thus, the behaviour of cancelling a thread and
 * then trying to do other operations on the thread is undefined.
 *
 * 2. The thread does not have a chance to voluntarily release its
 * resources (mutex, semaphore, and rw lock). Thus, deadlock may occur
 * if the programmer does not pay enough attention.
 *
 * This function is similar to pthread_cancel(3) but different from 
 * pthread_cancle(3) in terms of:
 *
 * 1. pthread_cancel allows the programmers to specify cleanup handlers.
 * The resource holding by the thread to be cancelled can be released.
 * This feature is not implemented here. Thus, it is programmers' duty
 * not to cancel a thread before the thread releases its resources.
 *
 * 2. pthread supports much advanced cancellation function like
 * pthread_setcancelstate(3) and pthread_setcanceltype(3) while only the
 * simpliest form of cancelling has been implemented here.
 */
int8_t          avr_thread_cancel(struct avr_thread *t);

/*
 * Terminates the calling thread. Similar to pthread_exit(3).
 */
void            avr_thread_exit(void);


/*
 * Waits for the thread identified by `t' to terminate. If that thread
 * has already terminated, this function returns immediately.) 
 */
void            avr_thread_join(struct avr_thread *t);


/**
 * Advanced Opeartions
 * ===================
 */

/**
 * Mutual execlusion
 * -----------------
 * Note:
 * In this implementation, calling unlock() with a mutex that the
 * calling thread does not hold will unlock the mutex while doing the
 * same thing with pthread will result in undefined behaviour.
 */

/*
 * Creates a new thread.
 * c.f., pthread_mutex_init(3).
 */
struct avr_thread_mutex *avr_thread_mutex_init(void);

/*
 * Frees the resources allocated for the mutex.
 * c.f., pthread_mutex_destory(3).
 */
void            avr_thread_mutex_destory(volatile
                                         struct avr_thread_mutex
                                         *mutex);
/*
 * Locks mutex.  If the mutex is already locked, the calling thread will
 * block until the mutex becomes available.
 * c.f., pthread_mutex_lock(3).
 */
void            avr_thread_mutex_lock(volatile
                                      struct avr_thread_mutex *mutex);

/*
 * Unlocks mutex and unblocks the waiting threads.
 * c.f., pthread_mutex_unlock(3).
 */
void            avr_thread_mutex_unlock(volatile
                                        struct avr_thread_mutex
                                        *mutex);

/**
 * Semaphore
 * ---------
 * Note:
 * The type of semaphore's value is uint8_t. Overflowing will result in
 * undefined behaviour.
 */

/*
 * Creates a semaphore with `value' as the initial value.
 */
struct avr_thread_semaphore *avr_thread_semaphore_init(int value);


/*
 * Frees the resources allocated for the semaphore.
 */
void            avr_thread_semaphore_destroy(volatile
                                             struct avr_thread_semaphore
                                             *sem);

/*
 * a.k.a. signal()
 */
void            avr_thread_sem_up(volatile struct avr_thread_semaphore
                                  *sem);

/*
 * a.k.a wait()
 */
void            avr_thread_sem_down(volatile struct avr_thread_semaphore
                                    *sem);

/**
 * Readers-writer lock
 * -------------------
 * Note:
 * In pthread, there is only one unlock operation,
 * pthread_rwlock_unlock(3). Both the reader and the writer can called
 * this function to unlock the lock.
 * Yet, I distinguish between reader's unlock and writer's unlock().
 */


/*
 * c.f., pthread_rwlock_init(3)
 */
struct avr_thread_rwlock *avr_thread_rwlock_init(void);

/*
 * c.f., pthread_rwlock_destroy(3)
 */
void            avr_thread_rwlock_destroy(volatile struct avr_thread_rwlock
                                          *rwlock);

/*
 * c.f., pthread_rwlock_rdlock(3)
 */
void            avr_thread_rwlock_rdlock(volatile struct avr_thread_rwlock
                                         *rwlock);

/*
 * c.f., pthread_rwlock_wrlock(3)
 */
void            avr_thread_rwlock_wrlock(volatile struct avr_thread_rwlock
                                         *rwlock);

/*
 * c.f., pthread_rwlock_unlock(3)
 */
void            avr_thread_rwlock_wrunlock(volatile struct avr_thread_rwlock
                                           *rwlock);

/*
 * c.f., pthread_rwlock_unlock(3)
 */
void            avr_thread_rwlock_rdunlock(volatile struct avr_thread_rwlock
                                           *rwlock);

#endif
