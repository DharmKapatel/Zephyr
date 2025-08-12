/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>


#define SLEEP_TIME_MS   1000


#define LED0_NODE DT_ALIAS(led0)
#define LED2_NODE DT_ALIAS(led2)
#define BUTTON_NODE DT_ALIAS(sw0)

/* Get GPIO device specs from the devicetree */
static const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

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
