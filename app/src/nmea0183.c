/*
 *
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nmea-2k.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nmea0183, LOG_LEVEL_DBG);

static int parse_nmea0183_field(const char *sentence, int field_num, char *field, size_t field_len)
{
	const char *start = sentence;
	int current_field = 0;

	while (*start && current_field < field_num) {
		if (*start == ',') {
			current_field++;
		}
		start++;
	}

	if (current_field != field_num) {
		return -1;
	}

	const char *end = start;
	while (*end && *end != ',' && *end != '*') {
		end++;
	}

	size_t len = end - start;
	if (len >= field_len) {
		len = field_len - 1;
	}

	memcpy(field, start, len);
	field[len] = '\0';

	return 0;
}

int convert_nmea0183_to_nmea2000(const char *nmea_in, struct can_frame *frame)
{
	char talker[3] = {0};
	char sentence_type[4] = {0};

	// Extract sentence type (e.g., $HEHDT or $GPRMC)
	if (nmea_in[0] != '$') {
		return -1;
	}

	memcpy(talker, &nmea_in[1], 2);
	const char *type_start = &nmea_in[3];
	int i = 0;
	while (type_start[i] && type_start[i] != ',' && i < 3) {
		sentence_type[i] = type_start[i];
		i++;
	}

	if (strcmp(sentence_type, "HDT") == 0) {
		// PGN 127250 - Vessel Heading
		char heading_str[16];
		if (parse_nmea0183_field(nmea_in, 1, heading_str, sizeof(heading_str)) == 0) {
			float heading = atof(heading_str);
			uint16_t heading_raw = (uint16_t)(heading / 57.2958 / 0.0001);

			frame->id = CAN_EFF_FLAG | (6 << 26) | (127250 << 8) | 0xFF;
			frame->dlc = 8;
			frame->data[0] = 0xFF; // SID
			frame->data[1] = heading_raw & 0xFF;
			frame->data[2] = (heading_raw >> 8) & 0xFF;
			frame->data[3] = 0xFF; // Deviation
			frame->data[4] = 0xFF;
			frame->data[5] = 0xFF; // Variation
			frame->data[6] = 0xFF;
			frame->data[7] = 0x00; // Reference (True)

			return 0;
		}
	} else if (strcmp(sentence_type, "DPT") == 0) {
		// PGN 128267 - Water Depth
		char depth_str[16];
		if (parse_nmea0183_field(nmea_in, 1, depth_str, sizeof(depth_str)) == 0) {
			float depth = atof(depth_str);
			uint32_t depth_cm = (uint32_t)(depth * 100);

			frame->id = CAN_EFF_FLAG | (6 << 26) | (128267 << 8) | 0xFF;
			frame->dlc = 8;
			frame->data[0] = 0xFF; // SID
			frame->data[1] = depth_cm & 0xFF;
			frame->data[2] = (depth_cm >> 8) & 0xFF;
			frame->data[3] = (depth_cm >> 16) & 0xFF;
			frame->data[4] = (depth_cm >> 24) & 0xFF;
			frame->data[5] = 0xFF; // Offset
			frame->data[6] = 0xFF;
			frame->data[7] = 0xFF; // Max range

			return 0;
		}
	}

	LOG_DBG("Unhandled sentence type: %s", sentence_type);
	return -EINVAL;
}
