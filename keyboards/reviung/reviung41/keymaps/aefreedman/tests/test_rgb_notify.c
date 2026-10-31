// Host-side tests of the production module, with QMK hardware calls mocked.
#define QMK_KEYBOARD_H "qmk_stub.h"
#include "../rgb_notify.c"

rgblight_ranges_t rgblight_ranges;
rgblight_config_t rgblight_config;
#ifdef RGBLIGHT_WS2812
ws2812_led_t ws2812_leds[WS2812_LED_COUNT];
#endif
static hsv_t saved_color;
static uint8_t speed;
static rgb_t pixels[RGBLIGHT_LED_COUNT];
static uint8_t reply[RAW_EPSIZE];
static unsigned replies;
static unsigned nonzero_writes;
static uint32_t now;

void raw_hid_send(uint8_t *data, uint8_t length) {
    assert(length == RAW_EPSIZE);
    memcpy(reply, data, length);
    replies++;
}
bool rgblight_is_enabled(void) { return rgblight_config.enable; }
uint8_t rgblight_get_mode(void) { return rgblight_config.enable ? rgblight_config.mode : 0; }
uint8_t rgblight_get_hue(void) { return saved_color.h; }
uint8_t rgblight_get_sat(void) { return saved_color.s; }
uint8_t rgblight_get_val(void) { return saved_color.v; }
hsv_t rgblight_get_hsv(void) { return saved_color; }
uint8_t rgblight_get_speed(void) { return speed; }
uint64_t rgblight_read_qword(void) {
    rgblight_config_t config = rgblight_config;
    config.hue = saved_color.h;
    config.sat = saved_color.s;
    config.val = saved_color.v;
    config.speed = speed;
    return config.raw;
}
uint32_t timer_read32(void) { return now; }
uint32_t timer_elapsed32(uint32_t start) { return now - start; }
// Deliberately simple conversion: these tests validate plumbing, not QMK color math.
rgb_t hsv_to_rgb(hsv_t hsv) { return (rgb_t){hsv.h, hsv.s, hsv.v}; }
void rgblight_set_clipping_range(uint8_t start, uint8_t count) {
    rgblight_ranges.clipping_start_pos = start;
    rgblight_ranges.clipping_num_leds = count;
}
void rgblight_set_effect_range(uint8_t start, uint8_t count) {
    assert(start < RGBLIGHT_LED_COUNT && start + count <= RGBLIGHT_LED_COUNT);
    rgblight_ranges.effect_start_pos = start;
    rgblight_ranges.effect_end_pos = start + count;
    rgblight_ranges.effect_num_leds = count;
}
void rgblight_setrgb_at(uint8_t r, uint8_t g, uint8_t b, uint8_t index) {
    assert(index < RGBLIGHT_LED_COUNT);
    pixels[index] = (rgb_t){r, g, b};
#ifdef RGBLIGHT_WS2812
    ws2812_leds[index] = (ws2812_led_t){.r = r, .g = g, .b = b};
#endif
    if (r || g || b) nonzero_writes++;
}
void rgblight_setrgb_range(uint8_t r, uint8_t g, uint8_t b, uint8_t start, uint8_t end) {
    assert(start <= end && end <= RGBLIGHT_LED_COUNT);
    for (uint8_t i = start; i < end; i++) rgblight_setrgb_at(r, g, b, i);
}
static void render(void) {
    rgb_t rgb = rgblight_config.enable ? hsv_to_rgb(saved_color) : (rgb_t){0};
    rgblight_setrgb_range(rgb.r, rgb.g, rgb.b, rgblight_ranges.effect_start_pos, rgblight_ranges.effect_end_pos);
}
void rgblight_mode_noeeprom(uint8_t mode) {
    if (rgblight_config.enable) {
        rgblight_config.mode = mode ? mode : RGBLIGHT_MODE_STATIC_LIGHT;
        render();
    }
}
void rgblight_enable_noeeprom(void) { rgblight_config.enable = true; render(); }
void rgblight_disable_noeeprom(void) { rgblight_config.enable = false; render(); }
void rgblight_sethsv_noeeprom(uint8_t h, uint8_t s, uint8_t v) {
    saved_color = (hsv_t){h, s, v};
    render();
}
void rgblight_set_speed_noeeprom(uint8_t value) { speed = value; }
void rgblight_update_qword(uint64_t config) {
    rgblight_config.raw = config;
    saved_color = (hsv_t){rgblight_config.hue, rgblight_config.sat, rgblight_config.val};
    speed = rgblight_config.speed;
    // Match QMK's update_qword: render only the new state, not an intermediate
    // enabled state when the new configuration is disabled.
    if (rgblight_config.enable) rgblight_mode_noeeprom(rgblight_config.mode);
    else render();
}

#ifdef RGBLIGHT_ENABLE
static void reset(bool enabled) {
    memset(&pending, 0, sizeof(pending));
    memset(&flash, 0, sizeof(flash));
    memset(pixels, 0, sizeof(pixels));
    rgblight_ranges = (rgblight_ranges_t){1, 9, 2, 6, 4};
    rgblight_config = (rgblight_config_t){.enable = enabled, .mode = 3};
    saved_color = (hsv_t){11, 22, 33};
    speed = 2;
    now = 0;
    replies = 0;
}

static void test_protocol(void) {
    reset(true);
    uint8_t packet[RAW_EPSIZE] = {QMK_NOTIFY_CMD_PING};
    rgb_notify_receive(packet, 0);
    assert(replies == 0);
    rgb_notify_receive(packet, sizeof(packet));
    assert(replies == 1 && reply[0] == packet[0] && reply[1] == QMK_NOTIFY_STATUS_OK);
    assert(reply[2] == 0 && reply[3] == 1 && reply[4] == 3);
    assert(reply[5] == 11 && reply[6] == 22 && reply[7] == 33);
    for (unsigned i = 8; i < sizeof(reply); i++) assert(reply[i] == 0);
    packet[0] = 0xff;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_UNSUPPORTED && pending.mode == NOTIFY_NONE);
}

static void test_readonly_diagnostics(void) {
    reset(true);
    uint8_t packet[] = {QMK_NOTIFY_CMD_LED_STATE, 0};
#ifdef RGBLIGHT_WS2812
    for (uint8_t i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_leds[i] = (ws2812_led_t){.r = i + 1, .g = i + 2, .b = i + 3};
        packet[1] = i;
        rgb_notify_receive(packet, sizeof(packet));
        assert(reply[1] == QMK_NOTIFY_STATUS_OK);
        assert(reply[8] == WS2812_LED_COUNT);
        assert(reply[9] == 1 && reply[10] == 9 && reply[11] == 2 && reply[12] == 4);
        assert(reply[13] == i && reply[14] == i + 1 && reply[15] == i + 2 && reply[16] == i + 3);
        assert(!flash.active && pending.mode == NOTIFY_NONE);
    }
    packet[1] = WS2812_LED_COUNT;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_INVALID_LED);
    for (unsigned i = 8; i < sizeof(reply); i++) assert(reply[i] == 0);
    rgb_notify_receive(packet, 1);
    assert(reply[1] == QMK_NOTIFY_STATUS_INVALID_LED);
#else
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_UNSUPPORTED);
#endif
}

static void test_invalid_led(void) {
    reset(true);
    uint8_t packet[] = {QMK_NOTIFY_CMD_RGB_AT, RGBLIGHT_LED_COUNT, 80, 90, 100, 100, 0};
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_INVALID_LED);
    assert(!flash.active && pending.mode == NOTIFY_NONE);
    assert(rgblight_ranges.effect_start_pos == 2);
    packet[1] = RGBLIGHT_LED_COUNT - 1;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_OK && pending.mode == NOTIFY_SINGLE_LED);
    notify_request_t previous = pending;
    packet[1] = 255;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_INVALID_LED);
    assert(memcmp(&pending, &previous, sizeof(pending)) == 0);
    rgb_notify_task();
    assert(flash.active && pixels[RGBLIGHT_LED_COUNT - 1].r == 80);
    now = 20;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_INVALID_LED && flash.start_time == 0);
    now = 100;
    rgb_notify_task();
    assert(!flash.active);
}

static void test_defaults_and_partial_packets(void) {
    for (uint8_t command = QMK_NOTIFY_CMD_FLASH; command <= QMK_NOTIFY_CMD_RGB_AT; command++) {
        uint8_t packet[RAW_EPSIZE] = {command, 3, 44, 55, 66, 0x34, 0x12};
        const uint8_t offset = command == QMK_NOTIFY_CMD_RGB_AT ? 2 : 1;
        for (uint8_t length = 1; length <= sizeof(packet); length++) {
            reset(true);
            rgb_notify_receive(packet, length);
            assert(reply[1] == QMK_NOTIFY_STATUS_OK);
            uint8_t a = length > offset ? packet[offset] : command == QMK_NOTIFY_CMD_FLASH ? 0 : 255;
            uint8_t b = length > offset + 1 ? packet[offset + 1] : command == QMK_NOTIFY_CMD_FLASH ? 255 : 0;
            uint8_t c = length > offset + 2 ? packet[offset + 2] : command == QMK_NOTIFY_CMD_FLASH ? 160 : 0;
            if (command == QMK_NOTIFY_CMD_FLASH) {
                assert(pending.mode == NOTIFY_HSV);
                assert(pending.color.hsv.h == a && pending.color.hsv.s == b && pending.color.hsv.v == c);
            } else {
                assert(pending.color.rgb.r == a && pending.color.rgb.g == b && pending.color.rgb.b == c);
            }
            uint16_t duration = length >= offset + 5 ? (uint16_t)packet[offset + 3] | ((uint16_t)packet[offset + 4] << 8) : 800;
            duration = duration < 100 ? 100 : duration > 60000 ? 60000 : duration;
            assert(pending.duration_ms == duration);
        }
    }
    reset(true);
    uint8_t packet[] = {QMK_NOTIFY_CMD_RGB, 1, 2, 3, 0, 0};
    rgb_notify_receive(packet, sizeof(packet));
    assert(pending.duration_ms == 100);
    packet[4] = packet[5] = 255;
    rgb_notify_receive(packet, sizeof(packet));
    assert(pending.duration_ms == 60000);
}

static void test_restore(bool initially_enabled) {
    reset(initially_enabled);
    const rgblight_ranges_t original_ranges = rgblight_ranges;
    uint8_t packet[] = {QMK_NOTIFY_CMD_RGB, 80, 90, 100, 100, 0};
    rgb_notify_receive(packet, sizeof(packet));
    assert(!flash.active); // Receiving does not touch the LEDs.
    rgb_notify_task();
    assert(flash.active && rgblight_is_enabled());
    assert(rgblight_ranges.clipping_start_pos == 0 && rgblight_ranges.clipping_num_leds == RGBLIGHT_LED_COUNT);
    assert(rgblight_ranges.effect_start_pos == 0 && rgblight_ranges.effect_num_leds == RGBLIGHT_LED_COUNT);
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        bool selected = false;
        for (unsigned j = 0; j < ARRAY_SIZE(active_leds); j++) selected |= active_leds[j] == i;
        assert(pixels[i].r == (selected ? 80 : 0));
    }
    now = 99;
    rgb_notify_task();
    assert(flash.active);
    now = 100;
    nonzero_writes = 0;
    rgb_notify_task();
    // Checking only the final pixels misses a bright frame just before off.
    if (!initially_enabled) assert(nonzero_writes == 0);
    assert(!flash.active && rgblight_is_enabled() == initially_enabled);
    assert(memcmp(&rgblight_ranges, &original_ranges, sizeof(original_ranges)) == 0);
    assert(rgblight_config.mode == 3 && speed == 2);
    assert(saved_color.h == 11 && saved_color.s == 22 && saved_color.v == 33);
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        bool in_range = initially_enabled && i >= 2 && i < 6;
        assert(pixels[i].r == (in_range ? 11 : 0));
    }
}

static void test_replacement_and_wrap(void) {
    reset(true);
    uint8_t packet[] = {QMK_NOTIFY_CMD_RGB, 80, 90, 100, 100, 0};
    rgb_notify_receive(packet, sizeof(packet));
    packet[1] = 50;
    rgb_notify_receive(packet, sizeof(packet));
    // Snapshot only when the first notification actually starts.
    saved_color.h = 17;
    rgb_notify_task();
    assert(pixels[0].r == 50 && flash.config.hue == 17);
    now = 80;
    packet[1] = 20;
    rgb_notify_receive(packet, sizeof(packet));
    rgb_notify_task();
    assert(pixels[0].r == 20 && flash.config.hue == 17);
    now = 100;
    rgb_notify_task();
    assert(flash.active);
    now = 180;
    rgb_notify_task();
    assert(!flash.active && saved_color.h == 17 && rgblight_ranges.effect_start_pos == 2);
    reset(true);
    now = UINT32_MAX - 50;
    rgb_notify_receive(packet, sizeof(packet));
    rgb_notify_task();
    now = 49;
    rgb_notify_task();
    assert(!flash.active);
}
#endif

int main(void) {
#ifdef RGBLIGHT_ENABLE
    test_protocol();
    test_readonly_diagnostics();
    test_invalid_led();
    test_defaults_and_partial_packets();
    test_restore(true);
    test_restore(false);
    test_replacement_and_wrap();
    puts("RGB notification regression tests passed (including 96 packet-length cases)");
#else
    uint8_t packet[] = {QMK_NOTIFY_CMD_PING};
    rgb_notify_receive(packet, 0);
    assert(replies == 0);
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_OK);
    packet[0] = QMK_NOTIFY_CMD_RGB;
    rgb_notify_receive(packet, sizeof(packet));
    assert(reply[1] == QMK_NOTIFY_STATUS_UNSUPPORTED);
    for (unsigned i = 2; i < sizeof(reply); i++) assert(reply[i] == 0);
    rgb_notify_task();
    puts("RGB-disabled protocol tests passed");
#endif
}
