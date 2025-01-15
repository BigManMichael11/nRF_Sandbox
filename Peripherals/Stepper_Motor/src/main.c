/*

 * Push joystick up for slow blinks
 * push left for fast blinks
 * depress button to toggle (poor mechanical switch so this is unreliable)
 * Log will display messages to screen using 115200, COM5 for my PC

 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>


LOG_MODULE_REGISTER(Joystick, LOG_LEVEL_DBG);//registers this program to be able to run up to debug messages


//Global varables
int x_mv;//x and y values in mv from the ADCs
int y_mv;

bool has_debounce;//bool for when debounce has finished

bool led_timer_running;//bool for if timer for led is already going
	
int16_t x_adc_buf;//buffers for filling wiith adc values
int16_t y_adc_buf;

//structs for adc init and return bufs
struct adc_sequence sequence = {
	.buffer = &x_adc_buf,
	.buffer_size = sizeof(x_adc_buf),
	// Optional
	//.calibrate = true,
};

struct adc_sequence sequence2 = {
	.buffer = &y_adc_buf,
	.buffer_size = sizeof(y_adc_buf),
	// Optional
	//.calibrate = true,
};


//


static const struct adc_dt_spec adc_channel1 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),0);//gets ADC channe from devicetree
static const struct adc_dt_spec adc_channel2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),1);

#define SW0_NODE DT_ALIAS(sw4)//define a new button for the switch
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

#define LED0_NODE DT_ALIAS(led0)//define board LED1
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct k_timer debounce_timer;//defines timers
static struct k_timer led_blink_timer;


static struct gpio_callback button_cb_data;

//button press function, checks for debounce and toggles led
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
	k_timer_start(&debounce_timer, K_MSEC(500), K_NO_WAIT);
	
}





//debounce expiry function, sets debounce to true and stops timer
void debounce_timer_expiry_function(struct k_timer *timer_id)
{
    has_debounce = true;
	k_timer_stop(&debounce_timer);
}


//function for when debounce timer stops, no code needed yet
void debounce_stop_timer_function(struct k_timer *timer_id)
{

}



void led_timer_expiry_function(struct k_timer *timer_id)
{
	//LOG_INF("Led toggling");
	gpio_pin_toggle_dt(&led);
	led_timer_running = true;
    k_timer_start(&led_blink_timer, K_MSEC(x_mv <= 1000 ? 200 : 500), K_NO_WAIT);
	if(x_mv >= 1000 && y_mv >= 1000){
		k_timer_stop(&led_blink_timer);
		led_timer_running = false;
	}
	
}

//function for when led timer stops, no code needed yet
void led_timer_stop_function(struct k_timer *timer_id)
{

}


bool adc_init(){
	if (!adc_is_ready_dt(&adc_channel1) || !adc_is_ready_dt(&adc_channel2)) {
		LOG_ERR("ADC controller devivce %s not ready");
		return false;
	}

	if (adc_channel_setup_dt(&adc_channel1) < 0 || adc_channel_setup_dt(&adc_channel2) < 0) {
		LOG_ERR("Could not setup channel");
		return false;
	}

	if (adc_sequence_init_dt(&adc_channel1, &sequence) < 0 || adc_sequence_init_dt(&adc_channel2, &sequence2) < 0) {
		LOG_ERR("Could not initalize sequnce");
		return false;
	}

	return true;
}


int main(void)
{
	
	int ret;//return value from functions, used to check error codes

	has_debounce = true;//reset debounce so we can start with a button press

	led_timer_running = false;



	adc_init();



	//function for when deboucne timer stops, no code needed yet
	k_timer_init(&debounce_timer, debounce_timer_expiry_function, debounce_stop_timer_function);
	k_timer_init(&led_blink_timer, led_timer_expiry_function, led_timer_stop_function);


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
		ret = adc_read(adc_channel1.dev, &sequence);
		if(ret != 0) LOG_ERR("ADC 1 couln't read ret:%d", ret);
		x_mv = (int)x_adc_buf;
		ret = adc_raw_to_millivolts_dt(&adc_channel1, &x_mv);
		if(led_timer_running == false && x_mv <= 500 && y_mv >= 1000){
			LOG_INF("Timer started by adc 1");
			k_timer_start(&led_blink_timer, K_MSEC(200), K_NO_WAIT);
			led_timer_running = true;
		}
		
		ret = adc_read(adc_channel2.dev, &sequence2);
		if(ret != 0) LOG_ERR("ADC 2 couln't read ret:%d", ret);
		y_mv = (int)y_adc_buf;
		ret = adc_raw_to_millivolts_dt(&adc_channel2, &y_mv);
		if(led_timer_running == false && y_mv <= 500 && x_mv >= 1000){
			LOG_INF("Timer started BY ADC 2");
			k_timer_start(&led_blink_timer, K_MSEC(500), K_NO_WAIT);
			led_timer_running = true;
		}
		
		if(i++ % 10 == 0) LOG_INF("val1:%d\t  val2:%d%stimer running%d",x_mv,y_mv, "\t", led_timer_running);
		k_sleep(K_MSEC(50));
	}
}
