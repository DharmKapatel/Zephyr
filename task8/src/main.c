/**
 * @file main.c
 * @brief UART interrupt RX and polling TX example using Zephyr.
 *
 * This program demonstrates:
 * - Receiving UART data using interrupts
 * - Echoing received data using polling
 * - Handling newline and carriage return characters
 *
 * Author: Dharm Kapatel
 * Date: 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

/** Device tree node label for USART1 */
#define UART_DEVICE_NODE DT_NODELABEL(usart1)

/** UART device handle */
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/** UART RX buffer size */
#define RX_BUF_SIZE 64
/** UART RX buffer */
static char rx_buf[RX_BUF_SIZE];
/** Current position in RX buffer */
static int rx_pos = 0;

/**
 * @brief UART interrupt callback for receiving data.
 *
 * Reads incoming bytes from UART FIFO. When a newline or carriage
 * return is received, the collected string is echoed back using
 * polling transmit.
 *
 * @param dev Pointer to the UART device
 * @param user_data User data (unused)
 */
static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t c;
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        uart_fifo_read(dev, &c, 1);

        if (c == '\n' || c == '\r') {
            rx_buf[rx_pos] = '\0';

            for (int i = 0; i < strlen(rx_buf); i++) {
                uart_poll_out(dev, rx_buf[i]);
            }
            uart_poll_out(dev, '\r');
            uart_poll_out(dev, '\n');

            rx_pos = 0; 
        } else {
            if (rx_pos < RX_BUF_SIZE - 1) {
                rx_buf[rx_pos++] = c;
            }
        }
    }
}

/**
 * @brief Main application entry point.
 *
 * Initializes the UART device, sets up the RX interrupt callback,
 * and prints an initial message. The program then sleeps indefinitely
 * while handling UART interrupts.
 */
void main(void)
{
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return;
    }

    uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
    uart_irq_rx_enable(uart_dev);

    const char *msg = "UART Interrupt RX, Polling TX demo\r\n";
    for (int i = 0; msg[i] != '\0'; i++) {
        uart_poll_out(uart_dev, msg[i]);
    }

    while (1) {
        k_sleep(K_MSEC(100)); 
    }
}
