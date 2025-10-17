/*
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NMEA_2K_H_
#define NMEA_2K_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/uart.h>

#define STACK_SIZE 2048

#define NMEA0183_UART_NODE DT_NODELABEL(usart2)
#define NMEA0183_BAUD_RATE 38400

#define NMEA2000_CAN_NODE DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))

#define NMEA2000_CAN_BITRATE (DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
					  DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
						     CONFIG_CAN_DEFAULT_BITRATE)) / 1000)

#define MSG_QUEUE_SIZE 32
#define NMEA_MAX_LEN 82
#define CAN_FRAME_TIMEOUT K_MSEC(100)

enum msg_type {
    MSG_CAN_TO_UART,
    MSG_UART_TO_CAN
};

struct converter_msg {
    enum msg_type type;
    union {
        struct can_frame can_frame;
        char nmea_sentence[NMEA_MAX_LEN];
    } data;
};

int convert_nmea0183_to_nmea2000(const char *nmea_in, struct can_frame *frame);
int convert_nmea2000_to_nmea0183(const struct can_frame *frame, char *nmea_out, size_t max_len);

#endif /* NMEA_2K_H_ */
