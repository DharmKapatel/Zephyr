/**
 * @file main.c
 * @brief Button-controlled LED toggling example using Zephyr GPIO API.
 *
 * This program demonstrates how to use GPIO with Zephyr.
 * - It configures two LEDs as outputs.
 * - It configures a button as input.
 * - When the button is pressed, the LEDs toggle state and print the new state.
 * - When the button is released, the LEDs are turned off.
 *
 * @author Dharm Kapatel
 * @date 2025
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/** Sleep duration in milliseconds */
#define SLEEP_TIME_MS   1000

/** Device tree alias for LED 1 */
#define LED0_NODE DT_ALIAS(led0)
/** Device tree alias for LED 2 */
#define LED2_NODE DT_ALIAS(led2)
/** Device tree alias for Button */
#define BUTTON_NODE DT_ALIAS(sw0)

/** GPIO spec for LED 1 */
static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
/** GPIO spec for LED 2 */
static const struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
/** GPIO spec for Button */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

/**
 * @brief Main application entry point.
 *
 * Initializes two LEDs and one button. If the button is pressed,
 * the LEDs toggle state and the status is printed. If released,
 * LEDs remain off.
 *
 * @retval 0 Always returns 0.
 */
int main(void)
{
	int ret;
	bool led_state = false;

	if (!gpio_is_ready_dt(&led_1) || !gpio_is_ready_dt(&button) || !gpio_is_ready_dt(&led_2)) {
		printf("Error: LED or button device not ready.\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led_1, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led_2, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		/* Read button state */
		int val = gpio_pin_get_dt(&button);
		if (val > 0) {
			ret = gpio_pin_toggle_dt(&led_1);
			if (ret < 0) {
				return 0;
			}
			ret = gpio_pin_toggle_dt(&led_2);
			if (ret < 0) {
				return 0;
			}
			led_state = !led_state;
			printf("LED state: %s\n", led_state ? "ON" : "OFF");
			k_msleep(SLEEP_TIME_MS);
		} else {
			gpio_pin_set_dt(&led_1, 0);
			gpio_pin_set_dt(&led_2, 0);
		}
	}
	return 0;
}
