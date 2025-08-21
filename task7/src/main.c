#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define UART_DEVICE_NODE DT_NODELABEL(usart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

void main(void)
{
    unsigned char c;

    if (!device_is_ready(uart_dev)) {
        return;
    }

    while (1) {
        if (uart_poll_in(uart_dev, &c) == 0)
       	{
	    printk("echo:");
            uart_poll_out(uart_dev, c);   
            uart_poll_out(uart_dev, '\r'); 
            uart_poll_out(uart_dev, '\n'); 
        }
    }
}

