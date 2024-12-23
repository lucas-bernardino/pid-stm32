/****************************************************************************
* MInimal Real-time Operating System (MiROS), GNU-ARM port.
* version 1.26 (matching lesson 26, see https://youtu.be/kLxxXNCrY60)
*
* This software is a teaching aid to illustrate the concepts underlying
* a Real-Time Operating System (RTOS). The main goal of the software is
* simplicity and clear presentation of the concepts, but without dealing
* with various corner cases, portability, or error handling. For these
* reasons, the software is generally NOT intended or recommended for use
* in commercial applications.
*
* Copyright (C) 2018 Miro Samek. All Rights Reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*
* Git repo:
* https://github.com/QuantumLeaps/MiROS
****************************************************************************/
#include <stdint.h>
#include "miros.h"
#include "qassert.h"
#include "stm32f1xx.h"

Q_DEFINE_THIS_FILE

#define NUM_MAX_PERIODIC_TASKS 10
#define NUM_MAX_APERIODIC_TASKS 10
#define NUM_MAX_NESTED_CRITICAL_REGIONS 10

OSThread * volatile OS_curr; /* pointer to the current thread */
OSThread * volatile OS_next; /* pointer to the next thread to run */

OSThread *OS_tasks[NUM_MAX_PERIODIC_TASKS + 2]; /* array of tasks*/
OSThread *OS_aperiodic_tasks[NUM_MAX_APERIODIC_TASKS]; /* array of aperiodics tasks */
uint32_t OS_readySet = 0; /* bitmask of threads that are ready to run */
uint32_t OS_delayedSet = 0; /* bitmask of threads that are delayed */
uint32_t OS_waiting_next_periodSet = 0; /* bitmask of threads that are waiting next period */
uint8_t number_periodic_tasks = 0;
uint8_t number_aperiodic_tasks = 0;

// Priority and index in OS_tasks array of a task in critical region
#define PRIORITY_CRITICAL_REGION_NPP NUM_MAX_PERIODIC_TASKS+1

#define LOG2(x) (32U - __builtin_clz(x))

OSThread idleThread;
void main_idleThread() {
    while (1) {
        OS_onIdle();
    }
}
void OS_error(){
	__disable_irq();
	while(1);
}
void OS_init(void *stkSto, uint32_t stkSize) {
    /* set the PendSV interrupt priority to the lowest level 0xFF */
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);

    /* start idleThread thread */
    OSPeriodic_task_start(&idleThread,
                   &main_idleThread,
                   stkSto, stkSize);
}

// Calculate the next task index (the position in OS_Thread array of next task) 
void OS_wait_next_period(){
    __disable_irq();
    
    uint8_t bit = (1U << (OS_curr->prio - 1U));
    OS_readySet   &= ~bit;  /* insert to set */
    OS_waiting_next_periodSet |= bit; /* remove from set */

    OS_sched();
    __enable_irq();
}

void OS_finished_aperiodic_task(void){
    __disable_irq();

    if (number_aperiodic_tasks == 1){
    	OS_aperiodic_tasks[0] = (OSThread *) 0;

    } else {
		// Update the queue array of aperiodic tasks
		for (uint8_t i = 1; i <= number_aperiodic_tasks; i++){
			OS_aperiodic_tasks[i-1] = OS_aperiodic_tasks[i];
			OS_aperiodic_tasks[i-1]->prio = i-1;
			OS_aperiodic_tasks[i-1]->critical_regions_historic[0] = i-1;
			OS_aperiodic_tasks[i] = (OSThread *) 0;
		}
    }

    // Decreasing number of aperiodic tasks
    number_aperiodic_tasks--;

    OS_sched();
    __enable_irq();
}


void OS_sched(void) {
    OSThread *next;
    uint8_t OS_Periodic_task_running_index = LOG2(OS_readySet);

    // If there is not any periodic task ready to sched
    if (OS_Periodic_task_running_index == 0U) {

        // If there is an aperiodic task to be executable 
        if (number_aperiodic_tasks){
            next = OS_aperiodic_tasks[0];

        } else {
            next = OS_tasks[0]; /* the idle thread */
        }

    } else {
        next = OS_tasks[OS_Periodic_task_running_index];
    }

    Q_ASSERT(next != (OSThread *)0);

    /* trigger PendSV, if needed */
    if (next != OS_curr) {
        OS_next = next;
        //*(uint32_t volatile *)0xE000ED04 = (1U << 28);
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        __asm volatile("dsb");
//        __asm volatile("isb");
    }
    /*
     * DSB - whenever a memory access needs to have completed before program execution progresses.
     * ISB - whenever instruction fetches need to explicitly take place after a certain point in the program,
     * for example after memory map updates or after writing code to be executed.
     * (In practice, this means "throw away any prefetched instructions at this point".)
     * */
}

void OS_run(void) {
    /* callback to configure and start interrupts */
    OS_onStartup();

    __disable_irq();
    OS_sched();
    __enable_irq();

    /* the following code should never execute */
    Q_ERROR();
}

void OS_tick(void) {

    uint32_t workingSet = OS_delayedSet;
    while (workingSet != 0U) {
        OSThread *t = OS_tasks[LOG2(workingSet)];
        uint32_t bit;
        Q_ASSERT((t != (OSThread *)0) && (t->timeout != 0U));

        bit = (1U << (t->prio - 1U));
        --t->timeout;
        if (t->timeout == 0U) {
            OS_readySet   |= bit;  /* insert to set */
            OS_delayedSet &= ~bit; /* remove from set */
        }
        workingSet &= ~bit; /* remove from working set */
    }

    /* Update the dinamics parameters os periodics tasks */
    for (int i = 1; i <= number_periodic_tasks; i++){
        OSThread *t = OS_tasks[i];

        t->task_parameters->deadline_dinamic--;
        t->task_parameters->period_dinamic--;

        if (t->task_parameters->period_dinamic == 0){
            uint32_t bit = (1U << (t->prio - 1U));

            OS_readySet   |= bit;  /* insert to set */
            OS_waiting_next_periodSet &= ~bit; /* remove from set */

            t->task_parameters->deadline_dinamic = t->task_parameters->deadline_absolute;
            t->task_parameters->period_dinamic = t->task_parameters->period_absolute;
        }
    }
}

void OS_delay(uint32_t ticks) {
    uint32_t bit;
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idleThread */
    Q_REQUIRE(OS_curr != OS_tasks[0]);

    OS_curr->timeout = ticks;
    bit = (1U << (OS_curr->prio - 1U));
    OS_readySet &= ~bit;
    OS_delayedSet |= bit;
    OS_sched();
    __asm volatile ("cpsie i");
}

/* initialization of the semaphore variable */
void semaphore_init(semaphore_t *p_semaphore, uint32_t start_value, uint32_t max_value){
	if (!p_semaphore){
		__disable_irq();
	}
	p_semaphore->sem_value = start_value;
	p_semaphore->max_value = max_value;
}

/*  */
void sem_up(semaphore_t *p_semaphore){
	__disable_irq();

    if (p_semaphore->sem_value < p_semaphore->max_value)
	    p_semaphore->sem_value++;


    // Update the queue of critical_regions_historic array and update the OS_readySet bitmask for schedulling
    for (uint8_t i = 0; i < NUM_MAX_NESTED_CRITICAL_REGIONS+1; i++){
       
        // Update the queue of critical_regions_historic array 
        OS_curr->critical_regions_historic[i] = OS_curr->critical_regions_historic[i+1];
        OS_curr->critical_regions_historic[i+1] = 0;

        // If 'i' is the end position of array
        if (OS_curr->critical_regions_historic[i] == OS_curr->prio){

            // If there was just one critical region,
            if (i == 0){

                // Update the OS_readySet bitmask
                uint32_t bit = (1U << (PRIORITY_CRITICAL_REGION_NPP - 1U));
                OS_readySet &= ~bit;

                // Set a null pointer in the unused position 
                OS_tasks[PRIORITY_CRITICAL_REGION_NPP] = (OSThread *) 0;
            }
            break;
        }
    }

	__enable_irq();
}

void sem_down(semaphore_t *p_semaphore){
	__disable_irq();

	while (p_semaphore->sem_value == 0){
		OS_delay(1U);
		__disable_irq();
	}

    // Update the queue of critical_regions_historic array and update the OS_readySet bitmask for schedulling
    for (uint8_t i = 0; i < NUM_MAX_NESTED_CRITICAL_REGIONS+1; i++){
        
        // If 'i' is the end position of array
        if (OS_curr->critical_regions_historic[i] == OS_curr->prio) {

            // Check if the critical_regions_historic queue is not full
            Q_REQUIRE(i+1 < NUM_MAX_NESTED_CRITICAL_REGIONS+1);

            // Update the positions of index
            for (uint8_t j = i+1; j > 0; j--){
                OS_curr->critical_regions_historic[j] = OS_curr->critical_regions_historic[j-1];
            }

            // Set the new prioriry of task and put the pointer in OS_tasks array
            OS_curr->critical_regions_historic[0] = PRIORITY_CRITICAL_REGION_NPP;
            OS_tasks[PRIORITY_CRITICAL_REGION_NPP] = OS_curr;

            // Set the bit of the task in OS_readySet bitmask
            uint32_t bit = (1U << (PRIORITY_CRITICAL_REGION_NPP - 1U));
            OS_readySet |= bit;

            break;
        }
    }

	p_semaphore->sem_value--;

	__enable_irq();
}

// Start a aperiodic task
void OSAperiodic_task_start(OSThread *me,
    OSThreadHandler threadHandler,
    void *stkSto, uint32_t stkSize){

	__disable_irq();

    uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    /* number of aperiodic tasks must be lower or equal to array with its parameters */
    Q_REQUIRE((number_aperiodic_tasks+1 < NUM_MAX_APERIODIC_TASKS));

    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the thread's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }

    OS_aperiodic_tasks[number_aperiodic_tasks] = me;
    OS_aperiodic_tasks[number_aperiodic_tasks]->prio = number_aperiodic_tasks;
    OS_aperiodic_tasks[number_aperiodic_tasks]->critical_regions_historic[0] = number_aperiodic_tasks;

    number_aperiodic_tasks++;

    __enable_irq();
}

void OSPeriodic_task_start(
    OSThread *me,
    OSThreadHandler threadHandler,
    void *stkSto, uint32_t stkSize) {

    /* round down the stack top to the 8-byte boundary
    * NOTE: ARM Cortex-M stack grows down from hi -> low memory
    */
    uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    /* priority must be in range of periodic tasks in array
    * and the priority level must be unused
    */
    Q_REQUIRE((number_periodic_tasks+1 < Q_DIM(OS_tasks)-1)
              && (OS_tasks[number_periodic_tasks+1] == (OSThread *)0));

    if (threadHandler != &main_idleThread)
        number_periodic_tasks++;

    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the thread's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xDEADBEEF */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEFU;
    }

    // If is the Idle Thread
    if (number_periodic_tasks == 0){
        OS_tasks[0] = me;
        OS_tasks[0]->prio = 0;
        OS_tasks[0]->critical_regions_historic[0] = 0;

    } else {
        for (uint8_t i=1; i <= number_periodic_tasks; i++){
            // Se deadline da task_i é menor, a task_me tem prioridade maior  ->  me vai ser salvo

            // If is the last loop, the task_me has the higher priority
            if (i == number_periodic_tasks){
                OS_tasks[i] = me;
                OS_tasks[i]->prio = i;
                OS_tasks[i]->critical_regions_historic[0] = i;

            } else if (OS_tasks[i]->task_parameters->deadline_absolute < me->task_parameters->deadline_absolute ||
            /* If the task_i has a lower deadline, it has a higher priority or
                its has a lower period with the same deadline than we need to 
                rearrange the tasks with the correct priority
            */
                    (OS_tasks[i]->task_parameters->deadline_absolute == me->task_parameters->deadline_absolute &&
                        OS_tasks[i]->task_parameters->period_absolute < me->task_parameters->period_absolute)){

                for (uint8_t j = number_periodic_tasks; j > i; j--){
                    OS_tasks[j] = OS_tasks[j-1];
                    OS_tasks[j]->prio = j;
                    OS_tasks[j]->critical_regions_historic[0] = j;
                }
                OS_tasks[i] = me;
                OS_tasks[i]->prio = i;
                OS_tasks[i]->critical_regions_historic[0] = i;
                break;
            }
        }
    }

    /* register the thread with the OS */
    /* make the thread ready to run */
    if (me->prio > 0U) {
        OS_readySet |= (1U << (me->prio - 1U));
    }
}

__attribute__ ((naked, optimize("-fno-stack-protector")))
void PendSV_Handler(void) {
__asm volatile (

    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSThread *)0) { */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,PendSV_restore \n"

    /*     push registers r4-r11 on the stack */
    "  PUSH          {r4-r11}          \n"

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    /* } */

    "PendSV_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=OS_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
    "  POP           {r4-r11}          \n"

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next thread */
    "  BX            lr                \n"
    );
}

