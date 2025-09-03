/**
 * @file main.c
 * @brief Button interrupt example with LED toggle using Zephyr GPIO API.
 *
 * This program demonstrates how to configure a button to generate
 * interrupts in Zephyr. When the button is pressed, an interrupt
 * service routine (ISR) toggles an LED and prints a message.
 *
 * - Configures one LED as output
 * - Configures one button as input with interrupt on press
 * - Uses gpio_callback to handle the button press
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/** Device tree alias for the LED */
#define LED_NODE DT_ALIAS(led0)
/** Device tree alias for the Button */
#define BUTTON_NODE DT_ALIAS(sw0)

/** GPIO spec for LED */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
/** GPIO spec for Button */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

/** GPIO callback structure for button interrupt */
static struct gpio_callback button_cb;

/**
 * @brief Button interrupt service routine (ISR).
 *
 * This function is called whenever the button is pressed.
 * It toggles the LED state and prints a debug message.
 *
 * @param dev Pointer to the device structure for the button port
 * @param cb Pointer to the callback structure
 * @param pins Bitmask of pins that triggered the interrupt
 */
void isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
    printk("Button pressed! LED toggled.\n");
}

/**
 * @brief Main application entry point.
 *
 * Configures the LED and button, sets up an interrupt for the
 * button, and waits indefinitely while handling events via ISR.
 *
 * @retval 0 Always returns 0.
 */
int main(void)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    gpio_pin_configure_dt(&button, GPIO_INPUT);

    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_cb, isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb);

    printk("Interrupt based button handling started\n");

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
