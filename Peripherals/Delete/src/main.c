/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>/* printf */
#include <string.h> /* strcmp */
#include <zephyr/drivers/uart.h> /* UART */


static uint8_t rx_buf[64] = {0}; //A buffer to store incoming UART data
static uint8_t tx_buf[] =  {"nRF Connect SDK Fundamentals Course \n\r"};

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));//defines pointer to UART device
//no api-specific structure for UART so it is defined using DEVICE_DT_GET

const struct uart_config uart_cfg = {//defining uart configurations that can be changed at runtime
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};


//uart callback with all options
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	
	case UART_TX_DONE:
		// do something
		break;

	case UART_TX_ABORTED:
		// do something
		break;
		
	case UART_RX_RDY:
		strncpy(rx_buf, evt->data.rx.buf, MIN(strlen(rx_buf), strlen(evt->data.rx.buf)));
		// do something
		break;

	case UART_RX_BUF_REQUEST:
		// do something
		break;

	case UART_RX_BUF_RELEASED:
		// do something
		break;
		
	case UART_RX_DISABLED:
		uart_rx_enable(dev, rx_buf, sizeof(rx_buf), 100);
		//continous reciption is not default, put here to have it continously record uart data
		break;

	case UART_RX_STOPPED:
		// do something
		break;
		
	default:
		break;
	}
}



int main(void)
{

	if (!device_is_ready(uart)) {//check if UART is ready
    	printf("\t\tUART ERROR\r\n");
		return 0;
	}

	int err = uart_configure(uart, &uart_cfg);
	if (err == -ENOSYS){
		return -ENOSYS;
	}

	err = uart_callback_set(uart, uart_cb, NULL);//initialize uart callback
	if (err) {
		return err;
	}

	err = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
	if (err){
		return err;
	}

	uart_rx_enable(uart, rx_buf, sizeof(rx_buf), 100);
	while (1){
		if(strncmp(rx_buf, tx_buf, MIN(strlen(rx_buf), strlen(tx_buf)-1)) != 0){
			strncpy(tx_buf, rx_buf,MIN(strlen(rx_buf), strlen(tx_buf))-1);
			tx_buf[MIN(strlen(rx_buf), strlen(tx_buf))] = 0;
			uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
		}
	}


	return 0;
}
