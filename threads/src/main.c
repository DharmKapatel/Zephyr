/**
 * @file main.c
 * @brief Multithreaded LED blinking with FIFO-based logging in Zephyr.
 *
 * This program demonstrates:
 * - Creating multiple threads to blink LEDs at different intervals
 * - Using a FIFO (k_fifo) to send LED toggle information to a logging thread
 * - Dynamic memory allocation for FIFO messages
 * - UART output of LED toggle events
 *
 * Author: Dharm Kapatel
 * Date: 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>
#include <zephyr/sys/mem_manage.h>

/** Stack size for each thread */
#define STACKSIZE 1024
/** Thread priority */
#define PRIORITY 7

/** Device tree aliases for LEDs */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(LED1_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

/**
 * @brief Data structure sent through FIFO to logging thread.
 */
struct printk_data_t {
    void *fifo_reserved; /**< 1st word reserved for use by FIFO */
    uint32_t led;        /**< LED identifier */
    uint32_t cnt;        /**< Toggle count */
};

/** FIFO to communicate LED toggle data to UART/logging thread */
K_FIFO_DEFINE(printk_fifo);

/**
 * @brief LED structure holding GPIO specification and ID.
 */
struct led {
    struct gpio_dt_spec spec;
    uint8_t num;
};

/** LED0 configuration */
static const struct led led0 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
    .num = 0,
};

/** LED1 configuration */
static const struct led led1 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
    .num = 1,
};

/**
 * @brief Blink an LED periodically and send toggle info to FIFO.
 *
 * @param led Pointer to LED structure
 * @param sleep_ms Blink interval in milliseconds
 * @param id LED identifier for logging
 */
void blink(const struct led *led, uint32_t sleep_ms, uint32_t id)
{
    const struct gpio_dt_spec *spec = &led->spec;
    int cnt = 0;
    int ret;

    if (!device_is_ready(spec->port)) {
        printk("Error: %s device is not ready\n", spec->port->name);
        return;
    }

    ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure pin %d (LED '%d')\n",
               ret, spec->pin, led->num);
        return;
    }

    while (1) {
        gpio_pin_set(spec->port, spec->pin, cnt % 2);

        struct printk_data_t tx_data = { .led = id, .cnt = cnt };

        size_t size = sizeof(struct printk_data_t);
        char *mem_ptr = k_malloc(size);
        __ASSERT_NO_MSG(mem_ptr != 0);

        memcpy(mem_ptr, &tx_data, size);

        k_fifo_put(&printk_fifo, mem_ptr);

        k_msleep(sleep_ms);
        cnt++;
    }
}

/** Blink thread for LED0 (fast blink) */
void blink0(void) { blink(&led0, 100, 0); }

/** Blink thread for LED1 (slow blink) */
void blink1(void) { blink(&led1, 1000, 1); }

/**
 * @brief UART/logging thread that prints FIFO messages.
 *
 * Receives LED toggle data from FIFO and prints it via printk.
 */
void uart_out(void)
{
    while (1) {
        struct printk_data_t *rx_data = k_fifo_get(&printk_fifo, K_FOREVER);
        printk("Toggled led%d; counter=%d\n", rx_data->led, rx_data->cnt);
        k_free(rx_data);
    }
}

/** Thread definitions */
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL, PRIORITY, 0, 0);
