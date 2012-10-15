/*
 * Copyright (c) 2002-2004  Brian S. Dean <bsd@bdmicro.com>
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN S. DEAN ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL BRIAN S. DEAN BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 */

/* 
 * $Id: thread_swtch.S,v 1.1 2004/01/17 01:37:53 bsd Exp $
 *
 * Context switch to new thread.
 *
 *
 * Author : Brian S. Dean <bsd@bsdhome.com>
 * Date   : 2002-08-29
 *
 *
 * Compile using:
 *
 *   avr-gcc -c -O2 -mmcu=${CPU} -Wa,--gstabs -o thread_swtch.o thread_swtch.S
 *
 *
 * Code Space Requirements:
 *
 *   160 bytes of text
 *
 */

        .section .text
        .global thread_swtch

#define SREG         0x3f
#define SPH          0x3e
#define SPL          0x3d


/*
 * thread_switch - switch context to new thread
 *
 * void thread_swtch(uint8_t * sp); 
 */
thread_swtch:
	cli
        push r0
        in   r0,SREG
	push r0
        push r1
        push r2
        push r3
        push r4
        push r5
        push r6
        push r7
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        push r16
        push r17
        push r18
        push r19
        push r20
        push r21
        push r22
        push r23
        push r24
        push r25
        push r26
        push r27
        push r28
        push r29
        push r30
        push r31

	/* save passed in destination stack argument */
	push r24
	push r25

        /* 
         * save the current stack pointer in the previous thread
         * structure 
         */
        in   r24,SPL
        in   r25,SPH
        call thread_save_sp

	pop  r25
	pop  r24

	/* switch stacks */
	out  SPL,r24
	out  SPH,r25

	/* restore new thread context and return */
        pop  r31
        pop  r30
        pop  r29
        pop  r28
        pop  r27
        pop  r26
        pop  r25
        pop  r24
        pop  r23
        pop  r22
        pop  r21
        pop  r20
        pop  r19
        pop  r18
        pop  r17
        pop  r16
        pop  r15
        pop  r14
        pop  r13
        pop  r12
        pop  r11
        pop  r10
        pop  r9
        pop  r8
        pop  r7
        pop  r6
        pop  r5
        pop  r4
        pop  r3
        pop  r2
	pop  r1
	pop  r0
        out  SREG,r0
        pop  r0
        reti
