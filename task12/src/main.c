/**
 * @file main.c
 * @brief Ping-Pong thread synchronization using semaphores in Zephyr.
 *
 * This program demonstrates:
 * - Synchronization between two threads using semaphores
 * - Alternating "Ping" and "Pong" messages in the console
 * - Using k_sem_take and k_sem_give to control execution order
 *
 * Author: Dharm Kapatel
 * Date: 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/** Semaphore allowing Ping thread to start first */
K_SEM_DEFINE(ping_sem, 1, 1);
/** Semaphore for Pong thread, initially waiting */
K_SEM_DEFINE(pong_sem, 0, 1);

/**
 * @brief Thread function for Ping.
 *
 * Waits on ping_sem, prints "Ping", then signals pong_sem.
 * Adds a small delay for readability.
 */
void ping_thread(void)
{
    while (1)
    {
        k_sem_take(&ping_sem, K_FOREVER);
        printk("Ping\n");
        k_sem_give(&pong_sem);
        k_msleep(500);
    }
}

/**
 * @brief Thread function for Pong.
 *
 * Waits on pong_sem, prints "Pong", then signals ping_sem.
 * Adds a small delay for readability.
 */
void pong_thread(void)
{
    while (1)
    {
        k_sem_take(&pong_sem, K_FOREVER);
        printk("Pong\n");
        k_sem_give(&ping_sem);
        k_msleep(500);
    }
}

/** Define Ping thread */
K_THREAD_DEFINE(ping_tid, 1024, ping_thread, NULL, NULL, NULL, 1, 0, 0);
/** Define Pong thread */
K_THREAD_DEFINE(pong_tid, 1024, pong_thread, NULL, NULL, NULL, 1, 0, 0);
