#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

K_SEM_DEFINE(ping_sem, 1, 1); // Start with Ping allowed
K_SEM_DEFINE(pong_sem, 0, 1); // Pong waits

void ping_thread(void)
{
    while (1)
    {
        k_sem_take(&ping_sem, K_FOREVER);
        printk("Ping\n");
        k_sem_give(&pong_sem);
        k_msleep(500); // Delay for readability
    }
}

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

K_THREAD_DEFINE(ping_tid, 1024, ping_thread, NULL, NULL, NULL, 1, 0, 0);
K_THREAD_DEFINE(pong_tid, 1024, pong_thread, NULL, NULL, NULL, 1, 0, 0);

