/* Copyright 2020 gtips
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "raw_hid.h"

#define QMK_NOTIFY_CMD_PING  0x01
#define QMK_NOTIFY_CMD_FLASH 0x02
#define QMK_NOTIFY_CMD_RGB   0x03
#define QMK_NOTIFY_CMD_RGB_AT 0x04

#define QMK_NOTIFY_STATUS_OK          0x00
#define QMK_NOTIFY_STATUS_UNSUPPORTED 0x01

#ifndef RAW_EPSIZE
#    define RAW_EPSIZE 32
#endif

#define QMK_NOTIFY_FLASH_H_DEFAULT     0
#define QMK_NOTIFY_FLASH_S_DEFAULT   255
#define QMK_NOTIFY_FLASH_V_DEFAULT   160
#define QMK_NOTIFY_FLASH_MS_DEFAULT  800
#define QMK_NOTIFY_FLASH_MS_MIN      100
#define QMK_NOTIFY_FLASH_MS_MAX    60000

typedef struct {
    bool     active;
    bool     was_enabled;
    uint8_t  mode;
    uint8_t  hue;
    uint8_t  sat;
    uint8_t  val;
    uint8_t  speed;
    uint32_t start_time;
    uint16_t duration_ms;
} rgb_flash_state_t;

static rgb_flash_state_t rgb_flash_state = {0};

typedef struct {
    bool     valid;
    bool     use_rgb;
    bool     use_single_led;
    uint8_t  hue;
    uint8_t  sat;
    uint8_t  val;
    uint8_t  red;
    uint8_t  green;
    uint8_t  blue;
    uint8_t  led_index;
    uint16_t duration_ms;
} rgb_pending_state_t;

static rgb_pending_state_t rgb_pending_state = {0};

static const uint8_t qmk_notify_active_leds[] = {0, 2, 3, 4, 5, 6, 8, 9};

enum layer_names {
    _COLEMAK,
    _BASE,
    _SYM,
    _NUM,
    _FUNC,
    _ADJUST,
    _FPS,
    _DJMAX,
};

#define ADJUST MO(_ADJUST)
#define COLEMAK DF(_COLEMAK)
#define QWERTY DF(_BASE)
#define FPS DF(_FPS)
#define DJMAX TG(_DJMAX)

enum td_keycodes {
    TD_SHIFT_SYM,
    TD_ENTER_CTRL,
    TD_CTRL_ALT,
};

typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_DOUBLE_HOLD,
    TD_TRIPLE_TAP,
    TD_TRIPLE_HOLD,
} td_state_t;

static td_state_t shift_sym_state  = TD_NONE;
static td_state_t enter_ctrl_state = TD_NONE;
static td_state_t ctrl_alt_state  = TD_NONE;

#define SHIFT_HOLD_GRACE_MS 130

static uint16_t shift_sym_timer = 0;

static td_state_t cur_dance(tap_dance_state_t *state);
static void       shift_sym_on_each_tap(tap_dance_state_t *state, void *user_data);
static void       shift_sym_finished(tap_dance_state_t *state, void *user_data);
static void       shift_sym_reset(tap_dance_state_t *state, void *user_data);
static void       enter_ctrl_finished(tap_dance_state_t *state, void *user_data);
static void       enter_ctrl_reset(tap_dance_state_t *state, void *user_data);
static void       ctrl_alt_finished(tap_dance_state_t *state, void *user_data);
static void       ctrl_alt_reset(tap_dance_state_t *state, void *user_data);
static void       unregister_mods_and_clear_oneshot(uint8_t mods);
static uint16_t   qmk_notify_u16le(const uint8_t *bytes);
static uint16_t   qmk_notify_clamp_u16(uint16_t value, uint16_t min, uint16_t max);
static void       qmk_notify_begin_flash(const uint8_t *data, uint8_t length);
static void       qmk_notify_begin_rgb(const uint8_t *data, uint8_t length);
static void       qmk_notify_begin_rgb_at(const uint8_t *data, uint8_t length);
static void       qmk_notify_apply_pending(void);
static void       qmk_notify_setrgb_active(uint8_t red, uint8_t green, uint8_t blue);
static void       qmk_notify_restore_rgb(void);

#define TH_NUM_BSPC LT(_NUM, KC_BSPC)
#define TH_FUNC_DEL LT(_FUNC, KC_DEL)
#define DK_SHIFT_SYM TD(TD_SHIFT_SYM)
#define DK_ENTER_CTRL TD(TD_ENTER_CTRL)
#define DK_CTRL_ALT TD(TD_CTRL_ALT)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_reviung41(
        KC_ESC,      KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                       KC_Y,      KC_U,    KC_I,    KC_O,    KC_P,       KC_LALT,
        KC_TAB,      KC_A,    KC_S,    KC_D,    KC_F,    KC_G,                       KC_H,      KC_J,    KC_K,    KC_L,    KC_SCLN,    KC_QUOT,
        DK_CTRL_ALT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,                       KC_N,      KC_M,    KC_COMM, KC_DOT,  KC_SLSH,    DK_ENTER_CTRL,
                                              KC_LGUI,    DK_SHIFT_SYM, KC_SPC,       TH_NUM_BSPC, TH_FUNC_DEL
    ),

    [_COLEMAK] = LAYOUT_reviung41(
        KC_ESC,      KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,                       KC_J,      KC_L,    KC_U,    KC_Y,    KC_SCLN,    KC_LALT,
        KC_TAB,      KC_A,    KC_R,    KC_S,    KC_T,    KC_G,                       KC_M,      KC_N,    KC_E,    KC_I,    KC_O,       KC_QUOT,
        DK_CTRL_ALT, KC_Z,    KC_X,    KC_C,    KC_D,    KC_V,                       KC_K,      KC_H,    KC_COMM, KC_DOT,  KC_SLSH,    DK_ENTER_CTRL,
                                              KC_LGUI,    DK_SHIFT_SYM, KC_SPC,       TH_NUM_BSPC, TH_FUNC_DEL
    ),

    [_NUM] = LAYOUT_reviung41(
        _______,     _______, KC_7,    KC_8,    KC_9,    _______,                    KC_HOME,   KC_PGUP, KC_UP,   KC_PGDN, _______,    ADJUST,
        _______,     _______, KC_4,    KC_5,    KC_6,    _______,                    KC_END,    KC_LEFT, KC_DOWN, KC_RGHT, _______,    KC_LSFT,
        _______,     _______, KC_1,    KC_2,    KC_3,    _______,                    KC_INSERT, _______, _______, _______, _______,    DK_ENTER_CTRL,
                                              KC_0,       KC_0,       KC_PDOT,       TH_NUM_BSPC, _______
    ),

    [_FUNC] = LAYOUT_reviung41(
        KC_F1,       KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                      XXXXXXX,   KC_VOLD, KC_VOLU, KC_MUTE, XXXXXXX,    XXXXXXX,
        KC_F7,       KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,                     XXXXXXX,   KC_MPRV, KC_MPLY, KC_MNXT, XXXXXXX,    XXXXXXX,
        KC_F13,      KC_F14,  KC_F15,  KC_F16,  KC_F17,  KC_F18,                     XXXXXXX,   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX,
                                              _______,    _______,    XXXXXXX,        _______,    _______
    ),

    [_SYM] = LAYOUT_reviung41(
        KC_TILDE,    XXXXXXX, KC_EXLM, KC_LPRN, KC_PSLS, KC_PIPE,                    KC_UNDS,   KC_PAST, KC_RPRN, KC_DLR,  XXXXXXX,    _______,
        _______,     XXXXXXX, KC_AT,   KC_LCBR, KC_MINS, KC_BSLS,                    KC_SLSH,   KC_PPLS, KC_RCBR, KC_PERC, KC_COLN,    XXXXXXX,
        _______,     XXXXXXX, KC_HASH, KC_LBRC, KC_GRV,  KC_QUOT,                    KC_DQUO,   KC_EQL,  KC_RBRC, KC_AMPR, KC_QUES,    _______,
                                              _______,    _______,    XXXXXXX,        _______,    _______
    ),

    [_ADJUST] = LAYOUT_reviung41(
        UG_VALU,     UG_SATU, UG_HUEU, UG_NEXT, XXXXXXX, UG_TOGG,                    QWERTY,    COLEMAK, FPS,     DJMAX,   XXXXXXX,    XXXXXXX,
        UG_VALD,     UG_SATD, UG_HUED, UG_PREV, XXXXXXX, XXXXXXX,                   QK_RBT,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX,
        XXXXXXX,     XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                    QK_BOOT,   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX,
                                              _______,    _______,    XXXXXXX,        _______,    _______
    ),

    [_FPS] = LAYOUT_reviung41(
        KC_ESC,      KC_F1,   KC_F2,   KC_F3,   KC_T,    KC_I,                       KC_3,      KC_Q,    KC_W,    KC_E,    KC_R,       KC_4,
        KC_TAB,      KC_F4,   KC_F5,   KC_F6,   KC_G,    KC_J,                       KC_2,      KC_A,    KC_S,    KC_D,    KC_F,       KC_B,
        COLEMAK,     KC_F7,   KC_F8,   KC_F9,   KC_P,    KC_M,                       KC_1,      KC_Z,    KC_X,    KC_C,    KC_V,       KC_LCTL,
                                              KC_LALT,    KC_TAB,    KC_SPC,         KC_LSFT,    KC_Y
    ),

    [_DJMAX] = LAYOUT_reviung41(
        KC_ESC,      KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                       KC_UP,     KC_U,    KC_I,    KC_LSFT, KC_RSFT,    DJMAX,
        KC_TAB,      KC_A,    KC_S,    KC_D,    KC_F,    KC_G,                       KC_DOWN,   KC_J,    KC_K,    KC_L,    KC_SCLN,    TH_NUM_BSPC,
        DK_CTRL_ALT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,                       KC_LEFT,   KC_RGHT, KC_PGUP, KC_PGDN, KC_F1,      KC_ENT,
                                              KC_V,       KC_B,      KC_SPC,         KC_N,       KC_F2
    ),

    // [_BLANK] = LAYOUT_reviung41(
    //     XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
    //     XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
    //     XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
    //                                      _______, _______, XXXXXXX,              _______, _______
    // )
};

static td_state_t cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->pressed) {
            return TD_SINGLE_HOLD;
        }
        return TD_SINGLE_TAP;
    }

    if (state->count == 2) {
        if (state->pressed) {
            return TD_DOUBLE_HOLD;
        }
        return TD_DOUBLE_TAP;
    }

    if (state->count == 3) {
        if (state->pressed) {
            return TD_TRIPLE_HOLD;
        }
        return TD_TRIPLE_TAP;
    }

    return TD_UNKNOWN;
}

static void shift_sym_on_each_tap(tap_dance_state_t *state, void *user_data) {
    (void)user_data;

    if (state->count == 1) {
        shift_sym_timer = timer_read();
    }
}

static void shift_sym_finished(tap_dance_state_t *state, void *user_data) {
    (void)user_data;

    shift_sym_state = cur_dance(state);

    if (shift_sym_state == TD_SINGLE_HOLD &&
        state->interrupted &&
        timer_elapsed(shift_sym_timer) < SHIFT_HOLD_GRACE_MS) {
        shift_sym_state = TD_SINGLE_TAP;
    }

    switch (shift_sym_state) {
        case TD_SINGLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LSFT));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LSFT));
            break;
        case TD_DOUBLE_TAP:
            set_oneshot_layer(_SYM, ONESHOT_START);
            break;
        case TD_DOUBLE_HOLD:
            layer_on(_SYM);
            break;
        default:
            break;
    }
}

static void shift_sym_reset(tap_dance_state_t *state, void *user_data) {
    (void)state;
    (void)user_data;

    switch (shift_sym_state) {
        case TD_SINGLE_TAP:
            break;
        case TD_SINGLE_HOLD:
            unregister_mods_and_clear_oneshot(MOD_BIT(KC_LSFT));
            break;
        case TD_DOUBLE_TAP:
            clear_oneshot_layer_state(ONESHOT_PRESSED);
            break;
        case TD_DOUBLE_HOLD:
            layer_off(_SYM);
            break;
        default:
            break;
    }

    shift_sym_state = TD_NONE;
}

static void enter_ctrl_finished(tap_dance_state_t *state, void *user_data) {
    (void)user_data;

    enter_ctrl_state = cur_dance(state);

    switch (enter_ctrl_state) {
        case TD_SINGLE_TAP:
            register_code16(KC_ENT);
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LCTL));
            break;
        default:
            break;
    }
}

static void enter_ctrl_reset(tap_dance_state_t *state, void *user_data) {
    (void)state;
    (void)user_data;

    switch (enter_ctrl_state) {
        case TD_SINGLE_TAP:
            unregister_code16(KC_ENT);
            break;
        case TD_SINGLE_HOLD:
            unregister_mods(MOD_BIT(KC_LCTL));
            break;
        default:
            break;
    }

    enter_ctrl_state = TD_NONE;
}

static void ctrl_alt_finished(tap_dance_state_t *state, void *user_data) {
    (void)user_data;

    ctrl_alt_state = cur_dance(state);

    switch (ctrl_alt_state) {
        case TD_SINGLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LCTL));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LCTL));
            break;
        case TD_DOUBLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LALT));
            break;
        case TD_DOUBLE_HOLD:
            register_mods(MOD_BIT(KC_LALT));
            break;
        case TD_TRIPLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT));
            break;
        case TD_TRIPLE_HOLD:
            register_mods(MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT));
            break;
        default:
            break;
    }
}

static void ctrl_alt_reset(tap_dance_state_t *state, void *user_data) {
    (void)state;
    (void)user_data;

    switch (ctrl_alt_state) {
        case TD_SINGLE_HOLD:
            unregister_mods_and_clear_oneshot(MOD_BIT(KC_LCTL));
            break;
        case TD_DOUBLE_HOLD:
            unregister_mods_and_clear_oneshot(MOD_BIT(KC_LALT));
            break;
        case TD_TRIPLE_HOLD:
            unregister_mods_and_clear_oneshot(MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT));
            break;
        default:
            break;
    }

    ctrl_alt_state = TD_NONE;
}

static void unregister_mods_and_clear_oneshot(uint8_t mods) {
    unregister_mods(mods);
    clear_oneshot_mods();
}

static uint16_t qmk_notify_u16le(const uint8_t *bytes) {
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint16_t qmk_notify_clamp_u16(uint16_t value, uint16_t min, uint16_t max) {
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

static void qmk_notify_begin_flash(const uint8_t *data, uint8_t length) {
#ifdef RGBLIGHT_ENABLE
    uint8_t  hue         = QMK_NOTIFY_FLASH_H_DEFAULT;
    uint8_t  sat         = QMK_NOTIFY_FLASH_S_DEFAULT;
    uint8_t  val         = QMK_NOTIFY_FLASH_V_DEFAULT;
    uint16_t duration_ms = QMK_NOTIFY_FLASH_MS_DEFAULT;

    if (length >= 2) {
        hue = data[1];
    }
    if (length >= 3) {
        sat = data[2];
    }
    if (length >= 4) {
        val = data[3];
    }
    if (length >= 6) {
        duration_ms = qmk_notify_u16le(data + 4);
    }

    duration_ms = qmk_notify_clamp_u16(duration_ms, QMK_NOTIFY_FLASH_MS_MIN, QMK_NOTIFY_FLASH_MS_MAX);

    if (!rgb_flash_state.active) {
        rgb_flash_state.was_enabled = rgblight_is_enabled();
        rgb_flash_state.mode        = rgblight_get_mode();
        rgb_flash_state.hue         = rgblight_get_hue();
        rgb_flash_state.sat         = rgblight_get_sat();
        rgb_flash_state.val         = rgblight_get_val();
        rgb_flash_state.speed       = rgblight_get_speed();
    }

    rgb_pending_state.valid       = true;
    rgb_pending_state.use_rgb     = false;
    rgb_pending_state.use_single_led = false;
    rgb_pending_state.hue         = hue;
    rgb_pending_state.sat         = sat;
    rgb_pending_state.val         = val;
    rgb_pending_state.duration_ms = duration_ms;
#else
    (void)data;
    (void)length;
#endif
}

static void qmk_notify_begin_rgb(const uint8_t *data, uint8_t length) {
#ifdef RGBLIGHT_ENABLE
    uint8_t  red         = 255;
    uint8_t  green       = 0;
    uint8_t  blue        = 0;
    uint16_t duration_ms = QMK_NOTIFY_FLASH_MS_DEFAULT;

    if (length >= 2) {
        red = data[1];
    }
    if (length >= 3) {
        green = data[2];
    }
    if (length >= 4) {
        blue = data[3];
    }
    if (length >= 6) {
        duration_ms = qmk_notify_u16le(data + 4);
    }

    duration_ms = qmk_notify_clamp_u16(duration_ms, QMK_NOTIFY_FLASH_MS_MIN, QMK_NOTIFY_FLASH_MS_MAX);

    if (!rgb_flash_state.active) {
        rgb_flash_state.was_enabled = rgblight_is_enabled();
        rgb_flash_state.mode        = rgblight_get_mode();
        rgb_flash_state.hue         = rgblight_get_hue();
        rgb_flash_state.sat         = rgblight_get_sat();
        rgb_flash_state.val         = rgblight_get_val();
        rgb_flash_state.speed       = rgblight_get_speed();
    }

    rgb_pending_state.valid       = true;
    rgb_pending_state.use_rgb     = true;
    rgb_pending_state.use_single_led = false;
    rgb_pending_state.red         = red;
    rgb_pending_state.green       = green;
    rgb_pending_state.blue        = blue;
    rgb_pending_state.duration_ms = duration_ms;
#else
    (void)data;
    (void)length;
#endif
}

static void qmk_notify_begin_rgb_at(const uint8_t *data, uint8_t length) {
#ifdef RGBLIGHT_ENABLE
    uint8_t  led_index   = 0;
    uint8_t  red         = 255;
    uint8_t  green       = 0;
    uint8_t  blue        = 0;
    uint16_t duration_ms = QMK_NOTIFY_FLASH_MS_DEFAULT;

    if (length >= 2) {
        led_index = data[1];
    }
    if (length >= 3) {
        red = data[2];
    }
    if (length >= 4) {
        green = data[3];
    }
    if (length >= 5) {
        blue = data[4];
    }
    if (length >= 7) {
        duration_ms = qmk_notify_u16le(data + 5);
    }

    if (led_index >= RGBLIGHT_LED_COUNT) {
        return;
    }

    duration_ms = qmk_notify_clamp_u16(duration_ms, QMK_NOTIFY_FLASH_MS_MIN, QMK_NOTIFY_FLASH_MS_MAX);

    if (!rgb_flash_state.active) {
        rgb_flash_state.was_enabled = rgblight_is_enabled();
        rgb_flash_state.mode        = rgblight_get_mode();
        rgb_flash_state.hue         = rgblight_get_hue();
        rgb_flash_state.sat         = rgblight_get_sat();
        rgb_flash_state.val         = rgblight_get_val();
        rgb_flash_state.speed       = rgblight_get_speed();
    }

    rgb_pending_state.valid          = true;
    rgb_pending_state.use_rgb        = true;
    rgb_pending_state.use_single_led = true;
    rgb_pending_state.led_index      = led_index;
    rgb_pending_state.red            = red;
    rgb_pending_state.green          = green;
    rgb_pending_state.blue           = blue;
    rgb_pending_state.duration_ms    = duration_ms;
#else
    (void)data;
    (void)length;
#endif
}

static void qmk_notify_apply_pending(void) {
#ifdef RGBLIGHT_ENABLE
    if (!rgb_pending_state.valid) {
        return;
    }

    rgblight_enable_noeeprom();
    rgblight_set_clipping_range(0, RGBLIGHT_LED_COUNT);
    rgblight_set_effect_range(0, RGBLIGHT_LED_COUNT);
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

    if (rgb_pending_state.use_rgb) {
        if (rgb_pending_state.use_single_led) {
            rgblight_setrgb_range(0, 0, 0, 0, RGBLIGHT_LED_COUNT);
            rgblight_setrgb_at(
                rgb_pending_state.red,
                rgb_pending_state.green,
                rgb_pending_state.blue,
                rgb_pending_state.led_index
            );
        } else {
            qmk_notify_setrgb_active(rgb_pending_state.red, rgb_pending_state.green, rgb_pending_state.blue);
        }
    } else {
        RGB rgb = hsv_to_rgb((HSV){rgb_pending_state.hue, rgb_pending_state.sat, rgb_pending_state.val});
        qmk_notify_setrgb_active(rgb.r, rgb.g, rgb.b);
    }

    rgb_flash_state.active      = true;
    rgb_flash_state.start_time  = timer_read32();
    rgb_flash_state.duration_ms = rgb_pending_state.duration_ms;
    rgb_pending_state.valid     = false;
#endif
}

static void qmk_notify_setrgb_active(uint8_t red, uint8_t green, uint8_t blue) {
#ifdef RGBLIGHT_ENABLE
    rgblight_setrgb_range(0, 0, 0, 0, RGBLIGHT_LED_COUNT);

    for (uint8_t i = 0; i < ARRAY_SIZE(qmk_notify_active_leds); i++) {
        uint8_t led_index = qmk_notify_active_leds[i];
        if (led_index < RGBLIGHT_LED_COUNT) {
            rgblight_setrgb_at(red, green, blue, led_index);
        }
    }
#else
    (void)red;
    (void)green;
    (void)blue;
#endif
}

static void qmk_notify_restore_rgb(void) {
#ifdef RGBLIGHT_ENABLE
    if (!rgb_flash_state.active) {
        return;
    }

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(rgb_flash_state.mode);
    rgblight_sethsv_noeeprom(rgb_flash_state.hue, rgb_flash_state.sat, rgb_flash_state.val);
    rgblight_set_speed_noeeprom(rgb_flash_state.speed);

    if (!rgb_flash_state.was_enabled) {
        rgblight_disable_noeeprom();
    }

    rgb_flash_state.active = false;
#endif
}

void housekeeping_task_user(void) {
#ifdef RGBLIGHT_ENABLE
    qmk_notify_apply_pending();

    if (rgb_flash_state.active && timer_elapsed32(rgb_flash_state.start_time) >= rgb_flash_state.duration_ms) {
        qmk_notify_restore_rgb();
    }
#endif
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    uint8_t command;
    uint8_t status = QMK_NOTIFY_STATUS_UNSUPPORTED;
    uint8_t response[RAW_EPSIZE] = {0};

    if (length == 0) {
        return;
    }

    command = data[0];

    switch (command) {
        case QMK_NOTIFY_CMD_PING:
            status = QMK_NOTIFY_STATUS_OK;
            break;
        case QMK_NOTIFY_CMD_FLASH:
            qmk_notify_begin_flash(data, length);
            status = QMK_NOTIFY_STATUS_OK;
            break;
        case QMK_NOTIFY_CMD_RGB:
            qmk_notify_begin_rgb(data, length);
            status = QMK_NOTIFY_STATUS_OK;
            break;
        case QMK_NOTIFY_CMD_RGB_AT:
            qmk_notify_begin_rgb_at(data, length);
            status = QMK_NOTIFY_STATUS_OK;
            break;
        default:
            break;
    }

    response[0] = command;
    response[1] = status;
#ifdef RGBLIGHT_ENABLE
    response[2] = rgb_flash_state.active ? 1 : 0;
    response[3] = rgblight_is_enabled() ? 1 : 0;
    response[4] = rgblight_get_mode();
    response[5] = rgblight_get_hue();
    response[6] = rgblight_get_sat();
    response[7] = rgblight_get_val();
#endif

    raw_hid_send(response, RAW_EPSIZE);
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_SHIFT_SYM]  = ACTION_TAP_DANCE_FN_ADVANCED(shift_sym_on_each_tap, shift_sym_finished, shift_sym_reset),
    [TD_ENTER_CTRL] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, enter_ctrl_finished, enter_ctrl_reset),
    [TD_CTRL_ALT]   = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ctrl_alt_finished, ctrl_alt_reset),
};
