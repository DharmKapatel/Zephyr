/**
 * @file main.c
 * @brief UART echo example using Zephyr UART API.
 *
 * This program demonstrates basic UART polling in Zephyr.
 * - Reads a character from UART
 * - Echoes the received character back along with a newline
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

/** Device tree node label for USART1 */
#define UART_DEVICE_NODE DT_NODELABEL(usart1)

/** UART device handle */
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/**
 * @brief Main function.
 *
 * Initializes the UART device and continuously echoes
 * received characters back to the sender.
 */
void main(void)
{
    unsigned char c;

    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return;
    }

    while (1) {
        if (uart_poll_in(uart_dev, &c) == 0) {
            printk("echo:");
            uart_poll_out(uart_dev, c);   
            uart_poll_out(uart_dev, '\r'); 
            uart_poll_out(uart_dev, '\n'); 
        }
    }
}
