// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "raw_hid.h"
#include "rgb_notify.h"

#if defined(RGBLIGHT_ENABLE) && defined(RGBLIGHT_WS2812)
#    include "ws2812.h"
// The AVR WS2812 driver owns this buffer; diagnostics only read it.
extern ws2812_led_t ws2812_leds[WS2812_LED_COUNT];
#endif

#define QMK_NOTIFY_CMD_PING   0x01
#define QMK_NOTIFY_CMD_FLASH  0x02
#define QMK_NOTIFY_CMD_RGB    0x03
#define QMK_NOTIFY_CMD_RGB_AT 0x04
#define QMK_NOTIFY_CMD_LED_STATE 0x05

#define QMK_NOTIFY_STATUS_OK          0x00
#define QMK_NOTIFY_STATUS_UNSUPPORTED 0x01
#define QMK_NOTIFY_STATUS_INVALID_LED 0x02

#ifndef RAW_EPSIZE
#    define RAW_EPSIZE 32
#endif

#ifdef RGBLIGHT_ENABLE
#    define QMK_NOTIFY_FLASH_H_DEFAULT    0
#    define QMK_NOTIFY_FLASH_S_DEFAULT  255
#    define QMK_NOTIFY_FLASH_V_DEFAULT  160
#    define QMK_NOTIFY_FLASH_MS_DEFAULT 800
#    define QMK_NOTIFY_FLASH_MS_MIN     100
#    define QMK_NOTIFY_FLASH_MS_MAX   60000

typedef enum {
    NOTIFY_NONE,
    NOTIFY_HSV,
    NOTIFY_RGB,
    NOTIFY_SINGLE_LED,
} notify_mode_t;

typedef struct {
    notify_mode_t mode;
    union {
        hsv_t hsv;
        rgb_t rgb;
    } color;
    uint8_t  led_index;
    uint16_t duration_ms;
} notify_request_t;

typedef struct {
    bool             active;
    rgblight_config_t config;
    rgblight_ranges_t ranges;
    uint32_t         start_time;
    uint16_t         duration_ms;
} notify_state_t;

static notify_request_t pending;
static notify_state_t   flash;

// The working LEDs selected during board diagnostics. Single-LED requests may
// still address any physical LED, including those omitted from this list.
static const uint8_t active_leds[] = {0, 2, 3, 4, 5, 6, 8, 9};

static uint16_t read_u16le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint16_t clamp_duration(uint16_t duration_ms) {
    if (duration_ms < QMK_NOTIFY_FLASH_MS_MIN) {
        return QMK_NOTIFY_FLASH_MS_MIN;
    }
    if (duration_ms > QMK_NOTIFY_FLASH_MS_MAX) {
        return QMK_NOTIFY_FLASH_MS_MAX;
    }
    return duration_ms;
}

static uint8_t queue_request(uint8_t command, const uint8_t *data, uint8_t length) {
    const bool single_led = command == QMK_NOTIFY_CMD_RGB_AT;
    const bool use_hsv    = command == QMK_NOTIFY_CMD_FLASH;
    const uint8_t color_offset    = single_led ? 2 : 1;
    const uint8_t duration_offset = color_offset + 3;
    const uint8_t led_index       = single_led && length >= 2 ? data[1] : 0;

    // Reject before touching the pending request or the saved lighting state.
    if (single_led && led_index >= RGBLIGHT_LED_COUNT) {
        return QMK_NOTIFY_STATUS_INVALID_LED;
    }

    notify_request_t request = {
        .mode        = use_hsv ? NOTIFY_HSV : single_led ? NOTIFY_SINGLE_LED : NOTIFY_RGB,
        .led_index   = led_index,
        .duration_ms = QMK_NOTIFY_FLASH_MS_DEFAULT,
    };
    if (use_hsv) {
        request.color.hsv = (hsv_t){
            length > color_offset     ? data[color_offset]     : QMK_NOTIFY_FLASH_H_DEFAULT,
            length > color_offset + 1 ? data[color_offset + 1] : QMK_NOTIFY_FLASH_S_DEFAULT,
            length > color_offset + 2 ? data[color_offset + 2] : QMK_NOTIFY_FLASH_V_DEFAULT,
        };
    } else {
        request.color.rgb = (rgb_t){
            length > color_offset     ? data[color_offset]     : 255,
            length > color_offset + 1 ? data[color_offset + 1] : 0,
            length > color_offset + 2 ? data[color_offset + 2] : 0,
        };
    }
    if (length >= duration_offset + 2) {
        request.duration_ms = read_u16le(data + duration_offset);
    }
    request.duration_ms = clamp_duration(request.duration_ms);
    pending             = request;
    return QMK_NOTIFY_STATUS_OK;
}

static void save_lighting(void) {
    // Save the complete live configuration, including the mode hidden by
    // rgblight_get_mode() when disabled. Do not read or modify EEPROM.
    flash.config.raw = rgblight_read_qword();
    flash.ranges     = rgblight_ranges;
}

static void apply_pending(void) {
    if (pending.mode == NOTIFY_NONE) {
        return;
    }

    // Overlapping notifications extend the flash, but never replace the
    // original snapshot with a snapshot of the notification itself.
    if (!flash.active) {
        save_lighting();
    }

    rgblight_set_clipping_range(0, RGBLIGHT_LED_COUNT);
    rgblight_set_effect_range(0, RGBLIGHT_LED_COUNT);
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_setrgb_range(0, 0, 0, 0, RGBLIGHT_LED_COUNT);

    const rgb_t rgb = pending.mode == NOTIFY_HSV ? hsv_to_rgb(pending.color.hsv) : pending.color.rgb;
    if (pending.mode == NOTIFY_SINGLE_LED) {
        rgblight_setrgb_at(rgb.r, rgb.g, rgb.b, pending.led_index);
    } else {
        for (uint8_t i = 0; i < ARRAY_SIZE(active_leds); i++) {
            if (active_leds[i] < RGBLIGHT_LED_COUNT) {
                rgblight_setrgb_at(rgb.r, rgb.g, rgb.b, active_leds[i]);
            }
        }
    }

    flash.active      = true;
    flash.start_time  = timer_read32();
    flash.duration_ms = pending.duration_ms;
    pending.mode      = NOTIFY_NONE;
}

static void restore_lighting(void) {
    // Clear notification pixels with the full ranges still selected, so LEDs
    // outside the restored effect range cannot retain the notification color.
    rgblight_setrgb_range(0, 0, 0, 0, RGBLIGHT_LED_COUNT);
    rgblight_set_clipping_range(flash.ranges.clipping_start_pos, flash.ranges.clipping_num_leds);
    rgblight_set_effect_range(flash.ranges.effect_start_pos, flash.ranges.effect_num_leds);
    // Restore enable/mode/color/speed together. In particular, restoring an
    // off state must never briefly render the saved animation before disabling.
    rgblight_update_qword(flash.config.raw);
    flash.active = false;
}
#endif

void rgb_notify_task(void) {
#ifdef RGBLIGHT_ENABLE
    apply_pending();
    if (flash.active && timer_elapsed32(flash.start_time) >= flash.duration_ms) {
        restore_lighting();
    }
#endif
}

void rgb_notify_receive(uint8_t *data, uint8_t length) {
    if (length == 0) {
        return;
    }

    const uint8_t command = data[0];
    uint8_t       status  = QMK_NOTIFY_STATUS_UNSUPPORTED;
    switch (command) {
        case QMK_NOTIFY_CMD_PING:
            status = QMK_NOTIFY_STATUS_OK;
            break;
#ifdef RGBLIGHT_ENABLE
        case QMK_NOTIFY_CMD_FLASH:
        case QMK_NOTIFY_CMD_RGB:
        case QMK_NOTIFY_CMD_RGB_AT:
            status = queue_request(command, data, length);
            break;
#    ifdef RGBLIGHT_WS2812
        case QMK_NOTIFY_CMD_LED_STATE:
            status = length >= 2 && data[1] < WS2812_LED_COUNT ? QMK_NOTIFY_STATUS_OK : QMK_NOTIFY_STATUS_INVALID_LED;
            break;
#    endif
#endif
        default:
            break;
    }

    uint8_t response[RAW_EPSIZE] = {command, status};
#ifdef RGBLIGHT_ENABLE
    response[2] = flash.active ? 1 : 0;
    response[3] = rgblight_is_enabled() ? 1 : 0;
    response[4] = rgblight_get_mode();
    response[5] = rgblight_get_hue();
    response[6] = rgblight_get_sat();
    response[7] = rgblight_get_val();
#    ifdef RGBLIGHT_WS2812
    if (command == QMK_NOTIFY_CMD_LED_STATE && status == QMK_NOTIFY_STATUS_OK) {
        response[8]  = WS2812_LED_COUNT;
        response[9]  = rgblight_ranges.clipping_start_pos;
        response[10] = rgblight_ranges.clipping_num_leds;
        response[11] = rgblight_ranges.effect_start_pos;
        response[12] = rgblight_ranges.effect_num_leds;
        response[13] = data[1];
        response[14] = ws2812_leds[data[1]].r;
        response[15] = ws2812_leds[data[1]].g;
        response[16] = ws2812_leds[data[1]].b;
    }
#    endif
#endif
    raw_hid_send(response, RAW_EPSIZE);
}
