/*
 *
 * Copyright (c) 2025 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nmea-2k.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nmea2000, LOG_LEVEL_DBG);

int convert_nmea2000_to_nmea0183(const struct can_frame *frame, char *nmea_out, size_t max_len)
{
	uint32_t pgn = (frame->id >> 8) & 0x1FFFF;

	switch (pgn) {
	case 127250: { // Vessel Heading
		// Extract heading from CAN data (simplified)
		uint16_t heading_raw = (frame->data[1] << 8) | frame->data[0];
		float heading = heading_raw * 0.0001; // radians to degrees conversion
		heading = heading * 57.2958;

		// Generate HDT sentence
		snprintf(nmea_out, max_len, "$HEHDT,%.1f,T", heading);
		uint8_t cs = calculate_nmea_checksum(nmea_out);
		snprintf(nmea_out + strlen(nmea_out), max_len - strlen(nmea_out), "*%02X\r\n", cs);
		return 0;
	}

	case 128267: { // Water Depth
		uint32_t depth_cm = (frame->data[3] << 24) | (frame->data[2] << 16) |
				    (frame->data[1] << 8) | frame->data[0];
		float depth = depth_cm * 0.01;

		// Generate DPT sentence
		snprintf(nmea_out, max_len, "$HEDPT,%.2f,0.0,", depth);
		uint8_t cs = calculate_nmea_checksum(nmea_out);
		snprintf(nmea_out + strlen(nmea_out), max_len - strlen(nmea_out), "*%02X\r\n", cs);
		return 0;
	}

	case 129026: { // COG & SOG
		uint16_t cog_raw = (frame->data[3] << 8) | frame->data[2];
		uint16_t sog_raw = (frame->data[5] << 8) | frame->data[4];
		float cog = cog_raw * 0.0001 * 57.2958;
		float sog = sog_raw * 0.01 * 1.94384; // m/s to knots

		// Generate VTG sentence
		snprintf(nmea_out, max_len, "$HEVTG,%.1f,T,%.1f,M,%.2f,N,%.2f,K,A", cog, cog, sog,
			 sog * 1.852);
		uint8_t cs = calculate_nmea_checksum(nmea_out);
		snprintf(nmea_out + strlen(nmea_out), max_len - strlen(nmea_out), "*%02X\r\n", cs);
		return 0;
	}

	default:
		LOG_DBG("Unhandled PGN: %u", pgn);
		return -EINVAL;
	}
}
