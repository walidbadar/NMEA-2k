/*
 *
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nmea-2k.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nmea_converter, LOG_LEVEL_DBG);

K_THREAD_STACK_DEFINE(can_rx_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(nmea_rx_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(converter_stack, STACK_SIZE * 2);

K_MSGQ_DEFINE(converter_msgq, sizeof(struct converter_msg), MSG_QUEUE_SIZE, 4);

static struct k_thread can_rx_tid;
static struct k_thread uart_rx_tid;
static struct k_thread converter_tid;

static const struct device *can_dev;
static const struct device *uart_dev;

static char uart_rx_buf[NMEA_MAX_LEN];
static int uart_rx_pos = 0;

void can_rx_callback(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct converter_msg msg = {.type = MSG_CAN_TO_UART, .data.can_frame = *frame};

	if (k_msgq_put(&converter_msgq, &msg, K_NO_WAIT) != 0) {
		LOG_WRN("CAN RX queue full, dropping message");
	}
}

void can_rx_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct can_filter filter = {.flags = CAN_FILTER_IDE, .id = 0, .mask = 0};

	int filter_id = can_add_rx_filter(can_dev, can_rx_callback, NULL, &filter);
	if (filter_id < 0) {
		LOG_ERR("Failed to add CAN filter");
		return;
	}

	LOG_INF("CAN RX thread started");

	while (1) {
		k_sleep(K_FOREVER);
	}
}

void uart_rx_callback(const struct device *dev, void *user_data)
{
	uint8_t c;

	while (uart_poll_in(dev, &c) == 0) {
		if (c == '\n') {
			uart_rx_buf[uart_rx_pos] = '\0';

			if (uart_rx_pos > 0 && uart_rx_buf[0] == '$') {
				struct converter_msg msg = {.type = MSG_UART_TO_CAN};
				strncpy(msg.data.nmea_sentence, uart_rx_buf, NMEA_MAX_LEN);

				if (k_msgq_put(&converter_msgq, &msg, K_NO_WAIT) != 0) {
					LOG_WRN("NMEA RX queue full");
				}
			}

			uart_rx_pos = 0;
		} else if (c != '\r' && uart_rx_pos < NMEA_MAX_LEN - 1) {
			uart_rx_buf[uart_rx_pos++] = c;
		}
	}
}

void uart_rx_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	uart_irq_callback_set(uart_dev, uart_rx_callback);
	uart_irq_rx_enable(uart_dev);

	LOG_INF("NMEA RX thread started");

	while (1) {
		k_sleep(K_FOREVER);
	}
}

void converter_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct converter_msg msg;
	struct can_frame frame;

	LOG_INF("Converter thread started");

	while (1) {
		if (k_msgq_get(&converter_msgq, &msg, K_FOREVER) == 0) {
			if (msg.type == MSG_CAN_TO_UART) {
				char nmea_out[NMEA_MAX_LEN];
				if (convert_nmea2000_to_nmea0183(&msg.data.can_frame, nmea_out,
								 sizeof(nmea_out)) == 0) {

					for (int i = 0; nmea_out[i]; i++) {
						uart_poll_out(uart_dev, nmea_out[i]);
					}
					LOG_DBG("CAN->NMEA: %s", nmea_out);
				}
			} else if (msg.type == MSG_UART_TO_CAN) {
				if (convert_nmea0183_to_nmea2000(msg.data.nmea_sentence, &frame) ==
				    0) {
					if (can_send(can_dev, &frame, K_MSEC(100), NULL, NULL) !=
					    0) {
						LOG_ERR("Failed to send CAN frame");
					}
					LOG_DBG("NMEA->CAN: %s", msg.data.nmea_sentence);
				}
			}
		}
	}
}

int main(void)
{
	LOG_INF("NMEA 2000 <-> NMEA 0183 Converter Starting");

	can_dev = DEVICE_DT_GET(NMEA2000_CAN_NODE);
	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN device not ready");
		return -ENODEV;
	}

	uart_dev = DEVICE_DT_GET(NMEA0183_UART_NODE);
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	struct uart_config uart_cfg = {.baudrate = NMEA0183_BAUD_RATE,
				       .parity = UART_CFG_PARITY_NONE,
				       .stop_bits = UART_CFG_STOP_BITS_1,
				       .data_bits = UART_CFG_DATA_BITS_8,
				       .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

	uart_configure(uart_dev, &uart_cfg);
	LOG_INF("UART initialized at %d bps", NMEA0183_BAUD_RATE);

	k_thread_create(&can_rx_tid, can_rx_stack, STACK_SIZE, can_rx_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	k_thread_create(&uart_rx_tid, nmea_rx_stack, STACK_SIZE, uart_rx_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	k_thread_create(&converter_tid, converter_stack, STACK_SIZE * 2, converter_thread, NULL,
			NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	LOG_INF("All threads started");

	return 0;
}
