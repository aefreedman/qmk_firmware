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

enum layer_names {
    _COLEMAK,
    _BASE,
    _SYM,
    _NUM,
    _FUNC,
    _ADJUST
};

// #define LOWER  MO(_LOWER)
// #define RAISE  MO(_RAISE)
#define ADJUST MO(_ADJUST)
#define COLEMAK DF(_COLEMAK)
#define QWERTY DF(_BASE)

enum td_keycodes {
    OSM_LS_L1, // one-shot shift tap, layer 1 doubletap
    ENT_CTRL, // enter tap, control doubletap, control + enter triple tap
    CTRL_ALT
};

typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    // TD_DOUBLE_SINGLE_TAP, // for cases where you want double-tap on a singletap key to return single (like 'balloon')
    TD_DOUBLE_HOLD,
    TD_TRIPLE_TAP,
    TD_TRIPLE_HOLD
} td_state_t;

static td_state_t td_state;

// Function to determine the current tapdance state
td_state_t cur_dance(qk_tap_dance_state_t *state);

// `finished` and `reset` functions for each tapdance keycode
void osm_ls_l1_finished(qk_tap_dance_state_t *state, void *user_data);
void osm_ls_l1_reset(qk_tap_dance_state_t *state, void *user_data);

void ent_ctrl_finished(qk_tap_dance_state_t *state, void *user_data);
void ent_ctrl_reset(qk_tap_dance_state_t *state, void *user_data);


#define OSM_LS OSM(MOD_LSFT)
#define WINTAB LGUI(KC_TAB)
#define MOD1 LT(1, KC_MINS)
#define MOD2 LT(_NUM, KC_BSPC)
#define MOD3 LT(_FUNC, KC_DEL)
#define MOD4 LT(2, KC_QUOT)
#define TD_OSM_LSL1 TD(OSM_LS_L1)
#define TD_ENT_CTRL TD(ENT_CTRL)
#define TD_CTRL_ALT TD(CTRL_ALT)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_BASE] = LAYOUT_reviung41(
    KC_ESC,   KC_Q,     KC_W,     KC_E,     KC_R,      KC_T,               KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LALT,
    KC_TAB,   KC_A,     KC_S,     KC_D,     KC_F,      KC_G,               KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,
    TD_CTRL_ALT,  KC_Z,     KC_X,     KC_C,     KC_V,      KC_B,               KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  TD_ENT_CTRL,
                                    KC_LGUI,   TD_OSM_LSL1,    KC_SPC,   MOD2,    MOD3
),

[_COLEMAK] = LAYOUT_reviung41(
KC_ESC,   KC_Q,     KC_W,     KC_F,     KC_P,      KC_B,               KC_J,     KC_L,     KC_U,     KC_Y,     KC_SCLN,     KC_LALT,
KC_TAB,   KC_A,     KC_R,     KC_S,     KC_T,      KC_G,               KC_M,     KC_N,     KC_E,     KC_I,     KC_O,  KC_QUOT,
TD_CTRL_ALT,  KC_Z,     KC_X,     KC_C,     KC_D,      KC_V,           KC_K,     KC_H,     KC_COMM,  KC_DOT,   KC_SLSH,  TD_ENT_CTRL,
                                KC_LGUI,   TD_OSM_LSL1,    KC_SPC,   MOD2,    MOD3
),

[_NUM] = LAYOUT_reviung41(
    _______,  _______,  KC_7,     KC_8,     KC_9,   _______,           KC_HOME,  KC_PGUP,  KC_UP,    KC_PGDN,  _______,   ADJUST,
    _______,  _______,  KC_4,     KC_5,     KC_6,   _______,           KC_END,   KC_LEFT,  KC_DOWN,  KC_RGHT,  _______,  KC_LSHIFT,
    _______,  _______,  KC_1,     KC_2,     KC_3,   _______,           KC_INSERT,  _______, _______, _______, _______,  TD_ENT_CTRL,
                                            KC_0,      KC_0,  KC_PDOT,   _______,  _______
  ),

[_FUNC] = LAYOUT_reviung41(
    KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,            XXXXXXX,  KC_VOLD,  KC_VOLU,  KC_MUTE,  XXXXXXX,  XXXXXXX,
    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,           XXXXXXX,  KC_MPRV,  KC_MPLY,  KC_MNXT,  XXXXXXX,  XXXXXXX,
    KC_F13,   KC_F14,   KC_F15,   KC_F16,   KC_F17,   KC_F18,           XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
                                            _______,   _______,  XXXXXXX,  _______,  _______
  ),

[_SYM] = LAYOUT_reviung41(
    KC_TILDE,   XXXXXXX, KC_EXLM,  KC_LPRN,  KC_PSLS,   KC_PIPE,            KC_UNDS,  KC_PAST, KC_RPRN, KC_DLR,  XXXXXXX,  _______,
    _______,   XXXXXXX, KC_AT,    KC_LCBR,  KC_MINS,   KC_BSLS,            KC_SLSH,  KC_PPLS, KC_RCBR, KC_PERC, KC_COLN,  XXXXXXX,
    _______,   XXXXXXX, KC_HASH,  KC_LBRC,  KC_GRV,    KC_QUOT,            KC_DQUO,  KC_EQL,  KC_RBRC, KC_AMPR, KC_QUES,  _______,
                                            _______,   _______,  XXXXXXX,  _______,  _______
),

[_ADJUST] = LAYOUT_reviung41(
RGB_VAI,   RGB_SAI, RGB_HUI,  RGB_MOD,  XXXXXXX,   RGB_TOG,            QWERTY,  COLEMAK,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
RGB_VAD,   RGB_SAD, RGB_HUD,  RGB_RMOD, XXXXXXX,   XXXXXXX,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
XXXXXXX,   XXXXXXX, XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,            RESET,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
                                        _______,   _______,  XXXXXXX,  _______,  _______
)

// [_BLANK] = LAYOUT_reviung41(
//     XXXXXXX,   XXXXXXX, XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
//     XXXXXXX,   XXXXXXX, XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
//     XXXXXXX,   XXXXXXX, XXXXXXX,  XXXXXXX,  XXXXXXX,   XXXXXXX,            XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
//                                             _______,   _______,  XXXXXXX,  _______,  _______
//   )
};

// layer_state_t layer_state_set_user(layer_state_t state) {
//   return update_tri_layer_state(state, _LOWER, _RAISE, _ADJUST);
// }

// Determine the tapdance state to return
td_state_t cur_dance(qk_tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return TD_SINGLE_TAP;
        // Key has not been interrupted, but the key is still held. Means you want to send a 'HOLD'.
        else return TD_SINGLE_HOLD;
    } else if (state->count == 2) {
        if (state->interrupted || !state->pressed) return TD_DOUBLE_TAP;
        else return TD_DOUBLE_HOLD;
    }

    // Assumes no one is trying to type the same letter three times (at least not quickly).
    // If your tap dance key is 'KC_W', and you want to type "www." quickly - then you will need to add
    // an exception here to return a 'TD_TRIPLE_SINGLE_TAP', and define that enum just like 'TD_DOUBLE_SINGLE_TAP'
    if (state->count == 3) {
        if (state->interrupted || !state->pressed) return TD_TRIPLE_TAP;
        else return TD_TRIPLE_HOLD;
    } else return TD_UNKNOWN;
}

// Handle the possible states for each tapdance keycode you define:
// Method for handling One-Shot Left Shift to Layer 1 on doubletap-hold
void osm_ls_l1_finished(qk_tap_dance_state_t *state, void *user_data) {
    td_state = cur_dance(state);
    switch (td_state) {
        case TD_SINGLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LSHIFT));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LSHIFT)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_TAP: // Allow nesting of 2 parens `((` within tapping term
            set_oneshot_layer(_SYM, ONESHOT_START);
            break;
        case TD_DOUBLE_HOLD:
            layer_on(_SYM);
            break;
        default:
            break;
    }
}

void osm_ls_l1_reset(qk_tap_dance_state_t *state, void *user_data) {
    switch (td_state) {
        case TD_SINGLE_TAP:
            // clear_oneshot_mods(); // dont clear on single tap or it resets osm dummy
            break;
        case TD_SINGLE_HOLD:
            unregister_mods(MOD_BIT(KC_LSHIFT));
            clear_oneshot_mods(); // For a layer-tap key, use `layer_off(_MY_LAYER)` here
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
}

// Handle the possible states for each tapdance keycode you define:
void ent_ctrl_finished(qk_tap_dance_state_t *state, void *user_data) {
    td_state = cur_dance(state);
    switch (td_state) {
        case TD_SINGLE_TAP:
            register_code16(KC_ENT);
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LCTRL)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_TAP: // Allow nesting of 2 parens `((` within tapping term
            // set_oneshot_layer(_SYM, ONESHOT_START);
            break;
        case TD_DOUBLE_HOLD:

            break;
        default:
            break;
    }
}

void ent_ctrl_reset(qk_tap_dance_state_t *state, void *user_data) {
    switch (td_state) {
        case TD_SINGLE_TAP:
            // clear_oneshot_mods(); // dont clear on single tap or it resets osm dummy
            unregister_code16(KC_ENT);
            break;
        case TD_SINGLE_HOLD:
            unregister_mods(MOD_BIT(KC_LCTRL)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_TAP:
            break;
        default:
            break;
    }
}// Handle the possible states for each tapdance keycode you define:
void ctrl_alt_finished(qk_tap_dance_state_t *state, void *user_data) {
    td_state = cur_dance(state);
    switch (td_state) {
        case TD_SINGLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LCTRL));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LCTRL)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LALT));
            break;
        case TD_DOUBLE_HOLD:
            register_mods(MOD_BIT(KC_LALT)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_TRIPLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LCTRL) | MOD_BIT(KC_LALT));
            break;
        case TD_TRIPLE_HOLD:
            set_mods(MOD_BIT(KC_LCTRL) | MOD_BIT(KC_LALT)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        default:
            break;
    }
}

void ctrl_alt_reset(qk_tap_dance_state_t *state, void *user_data) {
    switch (td_state) {
        case TD_SINGLE_TAP:
            // clear_oneshot_mods(); // dont clear on single tap or it resets osm dummy
            break;
        case TD_SINGLE_HOLD:
            unregister_mods(MOD_BIT(KC_LCTRL));
            clear_oneshot_mods();
            break;
        case TD_DOUBLE_HOLD:
            unregister_mods(MOD_BIT(KC_LALT));
            clear_oneshot_mods();
            break;
        case TD_TRIPLE_HOLD:
            clear_mods();
            clear_oneshot_mods();
            break;
        default:
            break;
    }
}

// Define `ACTION_TAP_DANCE_FN_ADVANCED()` for each tapdance keycode, passing in `finished` and `reset` functions
qk_tap_dance_action_t tap_dance_actions[] = {
    [OSM_LS_L1] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, osm_ls_l1_finished, osm_ls_l1_reset),
    [ENT_CTRL] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ent_ctrl_finished, ent_ctrl_reset),
    [CTRL_ALT] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ctrl_alt_finished, ctrl_alt_reset)
};
