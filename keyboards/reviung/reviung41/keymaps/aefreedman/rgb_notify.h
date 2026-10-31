#pragma once

#include <stdint.h>

// Raw HID requests (durations are little-endian milliseconds):
// 0x01: ping
// 0x02: flash, H, S, V, duration_lo, duration_hi
// 0x03: RGB, R, G, B, duration_lo, duration_hi
// 0x04: RGB_AT, LED index, R, G, B, duration_lo, duration_hi
// 0x05: LED_STATE, required LED index (read-only; WS2812 driver only)
// Missing trailing fields retain their defaults, as in the original protocol.
// Replies: command, status, active, enabled, mode, H, S, V; then zero padding.
// Successful LED_STATE replies append: LED count, clipping start/count,
// effect start/count, queried index, buffered R, G, B at bytes 8..16.
// Status: 0x00 OK, 0x01 unsupported, 0x02 invalid LED index.
// For color requests OK means queued, not yet displayed. Only the latest
// pending color request is kept. LED_STATE never changes the lighting.
void rgb_notify_receive(uint8_t *data, uint8_t length);
void rgb_notify_task(void);
