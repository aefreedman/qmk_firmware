#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define RAW_EPSIZE 32
#define RGBLIGHT_LED_COUNT 11
#define RGBLIGHT_MODE_STATIC_LIGHT 1

typedef struct { uint8_t r, g, b; } rgb_t;
typedef struct { uint8_t h, s, v; } hsv_t;
typedef struct {
    uint8_t clipping_start_pos, clipping_num_leds;
    uint8_t effect_start_pos, effect_end_pos, effect_num_leds;
} rgblight_ranges_t;
typedef union {
    uint64_t raw;
    struct {
        bool enable : 1;
        bool velocikey : 1;
        uint8_t mode : 6;
        uint8_t hue, sat, val, speed;
    };
} rgblight_config_t;
extern rgblight_ranges_t rgblight_ranges;
extern rgblight_config_t rgblight_config;

void raw_hid_send(uint8_t *data, uint8_t length);
bool rgblight_is_enabled(void);
uint8_t rgblight_get_mode(void);
uint8_t rgblight_get_hue(void);
uint8_t rgblight_get_sat(void);
uint8_t rgblight_get_val(void);
hsv_t rgblight_get_hsv(void);
uint8_t rgblight_get_speed(void);
uint64_t rgblight_read_qword(void);
void rgblight_update_qword(uint64_t config);
void rgblight_set_clipping_range(uint8_t start, uint8_t count);
void rgblight_set_effect_range(uint8_t start, uint8_t count);
void rgblight_enable_noeeprom(void);
void rgblight_disable_noeeprom(void);
void rgblight_mode_noeeprom(uint8_t mode);
void rgblight_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v);
void rgblight_set_speed_noeeprom(uint8_t speed);
void rgblight_setrgb_range(uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t end);
void rgblight_setrgb_at(uint8_t r, uint8_t g, uint8_t b, uint8_t index);
rgb_t hsv_to_rgb(hsv_t hsv);
uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t start);
