#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define NUM_INCREMENTS 1000

static int shared_counter = 0;
static struct k_mutex counter_mutex;

// Thread function to increment the counter
void counter_thread(void *p1, void *p2, void *p3)
{
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        k_mutex_lock(&counter_mutex, K_FOREVER); // Lock before access
        shared_counter++;
        k_mutex_unlock(&counter_mutex); // Unlock after access
    }
    printk("%s finished\n", k_thread_name_get(k_current_get()));
}

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

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

    // Wait for both threads to finish
    k_thread_join(&thread1_data, K_FOREVER);
    k_thread_join(&thread2_data, K_FOREVER);

    printk("Final counter value: %d (expected %d)\n",
           shared_counter, NUM_INCREMENTS * 2);

    return 0;
}

