#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel/thread.h>

#define STACK_SIZE 512
#define PRIORITY 5

K_THREAD_STACK_DEFINE(t1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(t2_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(t3_stack, STACK_SIZE);

struct k_thread t1_data, t2_data, t3_data;

K_SEM_DEFINE(sem1, 1, 1);
K_SEM_DEFINE(sem2, 0, 1);
K_SEM_DEFINE(sem3, 0, 1);

void task1(void *p1, void *p2, void *p3)
{
    for (int i = 1; i <= 9; i += 3)
    {
        k_sem_take(&sem1, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem2);
    }
}

void task2(void *p1, void *p2, void *p3)
{
    for (int i = 2; i <= 9; i += 3)
    {
        k_sem_take(&sem2, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem3);
    }
}

void task3(void *p1, void *p2, void *p3)
{
    for (int i = 3; i <= 9; i += 3)
    {
        k_sem_take(&sem3, K_FOREVER);
        printk("%d\n", i);
        k_sem_give(&sem1);
    }
}

void main()
{
    k_thread_create(&t1_data, t1_stack, STACK_SIZE,task1, NULL, NULL, NULL,PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&t2_data, t2_stack, STACK_SIZE,task2, NULL, NULL, NULL,PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&t3_data, t3_stack, STACK_SIZE,task3, NULL, NULL, NULL,PRIORITY, 0, K_NO_WAIT);
}
