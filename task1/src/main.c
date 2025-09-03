/**
 * @file main.c
 * @brief Multi-threading example in Zephyr using semaphores.
 *
 * This program demonstrates three threads printing numbers
 * in a sequential order using semaphores for synchronization:
 * - task1 prints 1, 4, 7
 * - task2 prints 2, 5, 8
 * - task3 prints 3, 6, 9
 *
 * Semaphores are used to ensure correct order of execution.
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread.h>

/** Stack size for each thread */
#define STACK_SIZE 512
/** Thread priority */
#define PRIORITY 5

/** Thread stacks */
K_THREAD_STACK_DEFINE(t1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(t2_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(t3_stack, STACK_SIZE);

/** Thread data structures */
struct k_thread t1_data, t2_data, t3_data;

/** Semaphores for thread synchronization */
K_SEM_DEFINE(sem1, 1, 1); /**< Initial count 1, for task1 */
K_SEM_DEFINE(sem2, 0, 1); /**< Initial count 0, for task2 */
K_SEM_DEFINE(sem3, 0, 1); /**< Initial count 0, for task3 */

/**
 * @brief Thread function for task1.
 *
 * Prints numbers 1, 4, 7 in sequence and signals task2.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
void task1(void *p1, void *p2, void *p3)
{
    for (int i = 1; i <= 9; i += 3)
    {
        k_sem_take(&sem1, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem2);
    }
}

/**
 * @brief Thread function for task2.
 *
 * Prints numbers 2, 5, 8 in sequence and signals task3.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
void task2(void *p1, void *p2, void *p3)
{
    for (int i = 2; i <= 9; i += 3)
    {
        k_sem_take(&sem2, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem3);
    }
}

/**
 * @brief Thread function for task3.
 *
 * Prints numbers 3, 6, 9 in sequence and signals task1.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
void task3(void *p1, void *p2, void *p3)
{
    for (int i = 3; i <= 9; i += 3)
    {
        k_sem_take(&sem3, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem1);
    }
}

/**
 * @brief Main function.
 *
 * Creates three threads to run task1, task2, and task3.
 */
void main(void)
{
    k_thread_create(&t1_data, t1_stack, STACK_SIZE, task1, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&t2_data, t2_stack, STACK_SIZE, task2, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&t3_data, t3_stack, STACK_SIZE, task3, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}
