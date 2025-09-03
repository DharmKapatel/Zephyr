/**
 * @file main.c
 * @brief Mutual exclusion example using Zephyr threads and mutex.
 *
 * This program demonstrates:
 * - Creating multiple threads in Zephyr
 * - Using a mutex to protect shared resources
 * - Ensuring correct increments to a shared counter
 *
 * Two threads increment a shared counter 1000 times each.
 * Mutex ensures mutual exclusion and prevents race conditions.
 *
 * Author: Dharm Kapatel
 * Date: 2025
 */

#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

/** Stack size for each thread */
#define STACK_SIZE 1024
/** Number of increments per thread */
#define NUM_INCREMENTS 1000

/** Shared counter */
static int shared_counter = 0;
/** Mutex to protect the shared counter */
static struct k_mutex counter_mutex;

/**
 * @brief Thread function to increment the shared counter.
 *
 * Locks the mutex before incrementing the counter and unlocks
 * after each increment to ensure mutual exclusion.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
void counter_thread(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER);
        shared_counter++;
        k_mutex_unlock(&counter_mutex);
    }
    printk("%s finished\n", k_thread_name_get(k_current_get()));
}

/** Thread stacks */
K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

/** Thread data structures */
struct k_thread thread1_data;
struct k_thread thread2_data;

/**
 * @brief Main function.
 *
 * Initializes mutex, creates two threads, waits for them to finish,
 * and prints the final value of the shared counter.
 *
 * @retval 0 Always returns 0.
 */
int main(void)
{
    printk("Starting mutual exclusion example...\n");

    k_mutex_init(&counter_mutex);

    k_thread_create(&thread1_data, thread1_stack, STACK_SIZE,
                    counter_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);
    k_thread_name_set(&thread1_data, "Thread 1");

    k_thread_create(&thread2_data, thread2_stack, STACK_SIZE,
                    counter_thread, NULL, NULL, NULL,
                    1, 0, K_NO_WAIT);
    k_thread_name_set(&thread2_data, "Thread 2");

    /* Wait for both threads to finish */
    k_thread_join(&thread1_data, K_FOREVER);
    k_thread_join(&thread2_data, K_FOREVER);

    printk("Final counter value: %d (expected %d)\n",
           shared_counter, NUM_INCREMENTS * 2);

    return 0;
}
