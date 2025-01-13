/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 * Note:
 * Tested on nRF Connect SDK Version : 2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>

int val_mv;

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

#define SLEEP_TIME_MS 10 * 60 * 1000

#define SW0_NODE DT_ALIAS(sw4)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct k_timer my_timer;
static struct k_timer led_timer;
bool has_debounce;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (has_debounce){
		gpio_pin_toggle_dt(&led);
		return;
	}
	has_debounce = false;
	k_timer_start(&my_timer, K_MSEC(250), K_NO_WAIT);
}

static struct gpio_callback button_cb_data;




void timer_expiry_function(struct k_timer *timer_id)
{
    has_debounce = true;
	k_timer_stop(&my_timer);
}

void timer_stop_function(struct k_timer *timer_id)
{

}

bool led_timer_running;

void led_timer_expiry_function(struct k_timer *timer_id)
{	
	gpio_pin_toggle_dt(&led);
	led_timer_running = true;
    k_timer_start(&led_timer, K_MSEC(250), K_NO_WAIT);
	if(val_mv >= 1000){
		k_timer_stop(&led_timer);
		led_timer_running = false;
	}
}

void led_timer_stop_function(struct k_timer *timer_id)
{

}

LOG_MODULE_REGISTER(Joystick, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("Start of main funciton");
	int ret;
	has_debounce = true;

	led_timer_running = false;

	int err;
	uint32_t count = 0;

	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
		// Optional
		//.calibrate = true,
	};

	/* STEP 3.3 - validate that the ADC peripheral (SAADC) is ready */
	if (!adc_is_ready_dt(&adc_channel)) {
		return 0;
	}
	/* STEP 3.4 - Setup the ADC channel */
	err = adc_channel_setup_dt(&adc_channel);
	if (err < 0) {
		gpio_pin_toggle_dt(&led);
		return 0;
	}
	/* STEP 4.2 - Initialize the ADC sequence */
	err = adc_sequence_init_dt(&adc_channel, &sequence);
	if (err < 0) {
		gpio_pin_toggle_dt(&led);
		return 0;
	}


	k_timer_init(&my_timer, timer_expiry_function, timer_stop_function);
	k_timer_init(&led_timer, led_timer_expiry_function, led_timer_stop_function);

	if (!device_is_ready(led.port)) {
		return -1;
	}

	if (!device_is_ready(button.port)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);


	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));


	gpio_add_callback(button.port, &button_cb_data);

	while (1) {
		err = adc_read(adc_channel.dev, &sequence);
		val_mv = (int)buf;
		err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
		if(val_mv <= 1000 && led_timer_running == false){
			k_timer_start(&led_timer, K_MSEC(250), K_NO_WAIT);
			led_timer_running = true;
		}
		k_sleep(K_MSEC(100));
	}
}