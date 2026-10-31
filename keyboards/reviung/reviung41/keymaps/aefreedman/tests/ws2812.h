#pragma once
#include "qmk_stub.h"
#define WS2812_LED_COUNT RGBLIGHT_LED_COUNT
typedef struct { uint8_t g, r, b; } ws2812_led_t;
