/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 * Tested on nRF Connect SDK Version : 2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>


int val_mv;
int val_mv2;

static const struct adc_dt_spec adc_channel1 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),0);
static const struct adc_dt_spec adc_channel2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),1);

#define SLEEP_TIME_MS 10 * 60 * 1000

#define SW0_NODE DT_ALIAS(sw4)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct k_timer my_timer;
static struct k_timer led_timer;
bool has_debounce;

LOG_MODULE_REGISTER(Joystick, LOG_LEVEL_DBG);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	
	if (has_debounce){
		LOG_INF("Button pressed");
		gpio_pin_toggle_dt(&led);
		return;
	} else {
		LOG_INF("Button waiting on debounce");
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
	//LOG_INF("Led toggling");
	gpio_pin_toggle_dt(&led);
	led_timer_running = true;
    k_timer_start(&led_timer, K_MSEC(val_mv <= 1000 ? 200 : 500), K_NO_WAIT);
	if(val_mv >= 1000 && val_mv2 >= 1000){
		k_timer_stop(&led_timer);
		led_timer_running = false;
	}
	
}

void led_timer_stop_function(struct k_timer *timer_id)
{

}


int main(void)
{
	
	int ret;
	LOG_INF("Start of main funciton\n");

	has_debounce = true;

	led_timer_running = false;
	int err;

	uint32_t count = 0;

	int16_t buf;
	int16_t buf2 = 1500;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
		// Optional
		//.calibrate = true,
	};
	
	struct adc_sequence sequence2 = {
		.buffer = &buf2,
		.buffer_size = sizeof(buf2),
		// Optional
		//.calibrate = true,
	};

	if (!adc_is_ready_dt(&adc_channel1) || !adc_is_ready_dt(&adc_channel2)) {
		LOG_ERR("ADC controller devivce %s not ready");
		return 0;
	}

	if (adc_channel_setup_dt(&adc_channel1) < 0 || adc_channel_setup_dt(&adc_channel2) < 0) {
		LOG_ERR("Could not setup channel");
		return 0;
	}

	if (adc_sequence_init_dt(&adc_channel1, &sequence) < 0 || adc_sequence_init_dt(&adc_channel2, &sequence2) < 0) {
		LOG_ERR("Could not initalize sequnce");
		return 0;
	}



	k_timer_init(&my_timer, timer_expiry_function, timer_stop_function);
	k_timer_init(&led_timer, led_timer_expiry_function, led_timer_stop_function);


	if (!device_is_ready(button.port)) {
		//return -1;
	}

	if (!device_is_ready(led.port)) {
		return -1;
	}
	
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		//return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		//return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);


	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));

	uint16_t i;
	gpio_add_callback(button.port, &button_cb_data);
	while (1) {
		err = adc_read(adc_channel1.dev, &sequence);
		if(err != 0) LOG_ERR("ADC 1 couln't read err:%d", err);
		val_mv = (int)buf;
		err = adc_raw_to_millivolts_dt(&adc_channel1, &val_mv);
		if(led_timer_running == false && val_mv <= 500 && val_mv2 >= 1000){
			LOG_INF("Timer started by adc 1");
			k_timer_start(&led_timer, K_MSEC(200), K_NO_WAIT);
			led_timer_running = true;
		}
		
		err = adc_read(adc_channel2.dev, &sequence2);
		if(err != 0) LOG_ERR("ADC 2 couln't read err:%d", err);
		val_mv2 = (int)buf2;
		err = adc_raw_to_millivolts_dt(&adc_channel2, &val_mv2);
		if(led_timer_running == false && val_mv2 <= 500 && val_mv >= 1000){
			LOG_INF("Timer started BY ADC 2");
			k_timer_start(&led_timer, K_MSEC(500), K_NO_WAIT);
			led_timer_running = true;
		}
		
		if(i++ % 10 == 0) LOG_INF("val1:%d\tval2:%d\ttimer running%d",val_mv,val_mv2, led_timer_running);
		k_sleep(K_MSEC(50));
	}
}
