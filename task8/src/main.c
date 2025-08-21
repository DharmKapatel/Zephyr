#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

#define UART_DEVICE_NODE DT_NODELABEL(usart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define RX_BUF_SIZE 64
static char rx_buf[RX_BUF_SIZE];
static int rx_pos = 0;

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

void main(void)
{
    if (!device_is_ready(uart_dev)) {
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

