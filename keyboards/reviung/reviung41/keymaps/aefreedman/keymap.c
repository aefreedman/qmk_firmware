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
#include "rgb_notify.h"

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

// Shift/Symbol: tap = one-shot Shift; hold = Shift;
// double tap = one-shot symbols; double hold = symbols.
// Enter/Ctrl: tap = Enter; hold = Ctrl.
// Ctrl/Alt: tap = one-shot Ctrl; double/triple tap = one-shot Ctrl+Alt;
// single/double/triple hold = Ctrl / Alt / Ctrl+Alt.
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

static td_state_t classify_tap_dance(const tap_dance_state_t *state);
static void       shift_sym_on_each_tap(tap_dance_state_t *state, void *user_data);
static void       shift_sym_finished(tap_dance_state_t *state, void *user_data);
static void       shift_sym_reset(tap_dance_state_t *state, void *user_data);
static void       enter_ctrl_finished(tap_dance_state_t *state, void *user_data);
static void       enter_ctrl_reset(tap_dance_state_t *state, void *user_data);
static void       ctrl_alt_finished(tap_dance_state_t *state, void *user_data);
static void       ctrl_alt_reset(tap_dance_state_t *state, void *user_data);
static void       release_held_mods(uint8_t mods);

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
        _______,     _______, KC_7,    KC_8,    KC_9,    _______,                    KC_HOME,   KC_PGUP, KC_UP,   KC_PGDN, _______,    KC_LALT,
        _______,     _______, KC_4,    KC_5,    KC_6,    _______,                    KC_END,    KC_LEFT, KC_DOWN, KC_RGHT, _______,    KC_LSFT,
        _______,     _______, KC_1,    KC_2,    KC_3,    _______,                    KC_INSERT, _______, _______, _______, _______,    DK_ENTER_CTRL,
                                              KC_0,       KC_LSFT,    KC_PDOT,       TH_NUM_BSPC, _______
    ),

    [_FUNC] = LAYOUT_reviung41(
        KC_F1,       KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                      XXXXXXX,   KC_VOLD, KC_VOLU, KC_MUTE, XXXXXXX,    KC_LALT,
        KC_F7,       KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,                     XXXXXXX,   KC_MPRV, KC_MPLY, KC_MNXT, XXXXXXX,    ADJUST,
        KC_F13,      KC_F14,  KC_F15,  KC_F16,  KC_F17,  KC_F18,                     XXXXXXX,   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    KC_LCTL,
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
        UG_VALD,     UG_SATD, UG_HUED, UG_PREV, XXXXXXX, XXXXXXX,                    QK_RBT,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX,
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

static void queue_oneshot_mods(uint8_t mods) {
    set_oneshot_mods(get_oneshot_mods() | mods);
}

static td_state_t classify_tap_dance(const tap_dance_state_t *state) {
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

    shift_sym_state = classify_tap_dance(state);

    // Treat a rapid rollover as one-shot Shift rather than a held modifier.

    if (shift_sym_state == TD_SINGLE_HOLD &&
        state->interrupted &&
        timer_elapsed(shift_sym_timer) < SHIFT_HOLD_GRACE_MS) {
        shift_sym_state = TD_SINGLE_TAP;
    }

    switch (shift_sym_state) {
        case TD_SINGLE_TAP:
            queue_oneshot_mods(MOD_BIT(KC_LSFT));
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
            release_held_mods(MOD_BIT(KC_LSFT));
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

    enter_ctrl_state = classify_tap_dance(state);

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

    ctrl_alt_state = classify_tap_dance(state);

    switch (ctrl_alt_state) {
        case TD_SINGLE_TAP:
            queue_oneshot_mods(MOD_BIT(KC_LCTL));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LCTL));
            break;
        case TD_DOUBLE_TAP:
        case TD_TRIPLE_TAP:
            queue_oneshot_mods(MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT));
            break;
        case TD_DOUBLE_HOLD:
            register_mods(MOD_BIT(KC_LALT));
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
            release_held_mods(MOD_BIT(KC_LCTL));
            break;
        case TD_DOUBLE_HOLD:
            release_held_mods(MOD_BIT(KC_LALT));
            break;
        case TD_TRIPLE_HOLD:
            release_held_mods(MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT));
            break;
        default:
            break;
    }

    ctrl_alt_state = TD_NONE;
}

static void release_held_mods(uint8_t mods) {
    unregister_mods(mods);
    // Releasing a held modifier must not cancel unrelated pending one-shots.
    del_oneshot_mods(mods);
}

void housekeeping_task_user(void) {
    rgb_notify_task();
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    rgb_notify_receive(data, length);
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_SHIFT_SYM]  = ACTION_TAP_DANCE_FN_ADVANCED(shift_sym_on_each_tap, shift_sym_finished, shift_sym_reset),
    [TD_ENTER_CTRL] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, enter_ctrl_finished, enter_ctrl_reset),
    [TD_CTRL_ALT]   = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ctrl_alt_finished, ctrl_alt_reset),
};
