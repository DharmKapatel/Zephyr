/**
 * @file main.c
 * @brief LED toggling example using Zephyr GPIO API and logging.
 *
 * This program demonstrates how to control multiple LEDs in Zephyr.
 * - Configures three LEDs as outputs
 * - Toggles their state in a loop
 * - Prints LED status using Zephyr logging API
 *
 * @author Dharm Kapatel
 * @date 2025
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/** Register logging module with debug level */
LOG_MODULE_REGISTER(my_module, LOG_LEVEL_DBG);

/** GPIO spec for LED 0 */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
/** GPIO spec for LED 1 */
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
/** GPIO spec for LED 2 */
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/**
 * @brief Configure all LEDs as output and set them inactive initially.
 *
 * Checks if the devices are ready, and configures each LED pin
 * as GPIO_OUTPUT_INACTIVE.
 */
static void leds_config(void)
{
    if (!device_is_ready(led0.port) || !device_is_ready(led1.port) || !device_is_ready(led2.port)) {
        LOG_ERR("One or more LED devices not ready");
        return;
    }

    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief Main function to toggle LEDs in a loop.
 *
 * Initializes LEDs and continuously toggles them every 500 ms.
 * Logs the current LED state on each toggle.
 *
 * @retval 0 Always returns 0.
 */
int main(void)
{
    bool state = false;
    leds_config();

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        gpio_pin_toggle_dt(&led2);

        state = !state;
        LOG_DBG("LED is %s", state ? "ON" : "OFF");

        k_msleep(500);
    }

    return 0; 
}
