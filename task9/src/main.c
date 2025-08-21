/*
* 9. Control LED via UART Commands Create a simple UART command parser:
* 		a. Accept commands like "LED ON\n", "LED OFF\n", "TOGGLE\n".
* 		b. Based on the command, turn ON, OFF, or TOGGLE an LED.
* 		c. Use UART reception either via polling or interrupts.
* 
* Author: Dhruv Mamtora
* Date: 08th August 2025
*/
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
 
#include <string.h>
 
#define LED0_NODE DT_ALIAS(led0)
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
 
#define MSG_SIZE 32
 
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
 
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);
 
/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
 
 
void print_uart(char *buf)
{
	int msg_len = strlen(buf);
 
	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}
 
/* Read characters from UART until line end is detected. Afterwards push the
* data to the message queue.
*/
void read_uart(const struct device *dev, void *user_data)
{
	uint8_t c;
 
	if (!uart_irq_update(uart_dev)) {
		return;
	}
 
	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}
 
	/* read until FIFO empty */
	while (uart_fifo_read(uart_dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';
 
			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
 
			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}
 
 
int main(void)
{
    char tx_buf[MSG_SIZE];
 
	if(!gpio_is_ready_dt(&led)) {
        return 0;
    }
 
    if(gpio_pin_configure_dt(&led,GPIO_OUTPUT_ACTIVE) < 0) {
        return 0;
    }
 
    if(!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return 0;
    }
 
    /* configure interrupt and callback to receive data */
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
 
    const char *welcome_msg = "Hello, Welcome to the UART Serial Terminal !\n\r";
 
    uart_irq_rx_enable(uart_dev);
 
    print_uart((char *)welcome_msg);
 
    /* wait for messages from the UART */
	while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
		print_uart("Echo: ");
		print_uart(tx_buf);
		print_uart("\r\n");
 
		if(strcmp(tx_buf, "LED ON") == 0) {
			gpio_pin_set_dt(&led, 1); // Turn ON LED
		} else if(strcmp(tx_buf, "LED OFF") == 0) {
			gpio_pin_set_dt(&led, 0); // Turn OFF LED
		} else if(strcmp(tx_buf, "TOGGLE") == 0) {
			gpio_pin_toggle_dt(&led); // TOGGLE LED state
		} else {
			print_uart("Unknown command! write LED ON LED OFF TOGGLE\r\n");
		}
		k_msleep(1000); // Small delay to avoid flooding the UART
	}
	return 0;
}
