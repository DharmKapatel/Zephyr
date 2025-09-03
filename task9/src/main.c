/**
 * @file main.c
 * @brief UART command parser to control an LED on Zephyr.
 *
 * This program demonstrates:
 * - Receiving UART commands via interrupt-driven reception
 * - Echoing received commands back via UART polling
 * - Controlling an LED based on commands: "LED ON", "LED OFF", "TOGGLE"
 * - Using a message queue (k_msgq) to store incoming messages
 *
 * Commands are case-sensitive and terminated by newline or carriage return.
 *
 * Author: Dharm Kapatel
 * Date: 08th August 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

/** Device tree alias for LED0 */
#define LED0_NODE DT_ALIAS(led0)
/** Device tree node for UART */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

/** Maximum message size */
#define MSG_SIZE 32

/** Message queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/** GPIO spec for LED */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
/** UART device handle */
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/** Receive buffer for UART ISR */
static char rx_buf[MSG_SIZE];
/** Current position in receive buffer */
static int rx_buf_pos;

/**
 * @brief Send a string over UART using polling.
 *
 * @param buf Null-terminated string to send
 */
void print_uart(char *buf)
{
    int msg_len = strlen(buf);

    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}

/**
 * @brief UART interrupt callback to read incoming characters.
 *
 * Reads characters from the UART FIFO until empty.
 * When newline or carriage return is detected, the message
 * is pushed to the message queue for processing in main().
 *
 * @param dev UART device
 * @param user_data User data pointer (unused)
 */
void read_uart(const struct device *dev, void *user_data)
{
    uint8_t c;
    ARG_UNUSED(user_data);

    if (!uart_irq_update(uart_dev) || !uart_irq_rx_ready(uart_dev)) {
        return;
    }

    while (uart_fifo_read(uart_dev, &c, 1) == 1) {
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
            rx_buf[rx_buf_pos] = '\0';
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }
        /* Characters beyond buffer size are dropped */
    }
}

/**
 * @brief Main application entry point.
 *
 * Configures LED and UART, sets up interrupt-driven reception,
 * and processes incoming commands to control the LED.
 *
 * Commands supported:
 * - "LED ON"   : Turn LED on
 * - "LED OFF"  : Turn LED off
 * - "TOGGLE"   : Toggle LED state
 */
int main(void)
{
    char tx_buf[MSG_SIZE];

    if (!gpio_is_ready_dt(&led)) {
        return 0;
    }

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
        return 0;
    }

    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return 0;
    }

    /* Configure UART interrupt callback */
    int ret = uart_irq_callback_user_data_set(uart_dev, read_uart, NULL);
    if (ret < 0) {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART API support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }
        return 0;
    }

    const char *welcome_msg = "Hello, Welcome to the UART Serial Terminal!\r\n";
    uart_irq_rx_enable(uart_dev);
    print_uart((char *)welcome_msg);

    /* Wait for messages from UART and process commands */
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        print_uart("Echo: ");
        print_uart(tx_buf);
        print_uart("\r\n");

        if (strcmp(tx_buf, "LED ON") == 0) {
            gpio_pin_set_dt(&led, 1); /* Turn ON LED */
        } else if (strcmp(tx_buf, "LED OFF") == 0) {
            gpio_pin_set_dt(&led, 0); /*Turn OFF LED*/
        } else if (strcmp(tx_buf, "TOGGLE") == 0) {
            gpio_pin_toggle_dt(&led); /*TOGGLE LED*/
        } else {
            print_uart("Unknown command! Use: LED ON, LED OFF, TOGGLE\r\n");
        }

        k_msleep(1000); /*Small delay to avoid flooding UART*/
    }

    return 0;
}
