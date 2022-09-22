/* Copyright 2015-2021 Jack Humbert
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
#include "muse.h"

// enum preonic_layers {
//   _QWERTY,
//   _COLEMAK,
//   _DVORAK,
//   _LOWER,
//   _RAISE,
//   _ADJUST
// };

// enum preonic_keycodes {
//   QWERTY = SAFE_RANGE,
//   COLEMAK,
//   DVORAK,
//   LOWER,
//   RAISE,
//   BACKLIT
// };

enum td_keycodes {
    OSM_LS_L1 // one-shot shift tap, layer 1 doubletap
};

typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_SINGLE_TAP
} td_state_t;

static td_state_t td_state;

// Function to determine the current tapdance state
td_state_t cur_dance(qk_tap_dance_state_t *state);

// `finished` and `reset` functions for each tapdance keycode
void osm_ls_l1_finished(qk_tap_dance_state_t *state, void *user_data);
void osm_ls_l1_reset(qk_tap_dance_state_t *state, void *user_data);


#define OSM_LS OSM(MOD_LSFT)
#define WINTAB LGUI(KC_TAB)
#define MOD1 LT(1,KC_MINS)
#define MOD2 LT(4,KC_BSPC)
#define MOD3 LT(3,KC_DEL)
#define MOD4 LT(2,KC_QUOT)
#define TD_OSM_LSL1 TD(OSM_LS_L1)


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = LAYOUT_ortho_5x12(
		KC_GESC,	KC_1,	    KC_2,	    KC_3,	        KC_4,	        KC_5,	    KC_6,	    KC_7,	         KC_8,	         KC_9,	         KC_0,		 _______,
		KC_TAB,	 	KC_Q,	    KC_W,	    KC_E,	        KC_R,	        KC_T,	    KC_Y,	    KC_U,	         KC_I,	         KC_O,	         KC_P,		 _______,
		KC_NO,		KC_A, 	    KC_S,	    KC_D,	        KC_F,	        KC_G,	    KC_H,	    KC_J,	         KC_K,	         KC_L,	         KC_SCLN,	 MOD4,
		KC_NO,		KC_Z,	    KC_X,	    KC_C,	        KC_V,	        KC_B,	    KC_N,	    KC_M,	         KC_COMM,        KC_DOT,         KC_SLSH,    KC_ENT,
		KC_LCTL,    KC_LALT,    KC_LGUI,    TD_OSM_LSL1, TD_OSM_LSL1,         KC_SPC,     KC_SPC,     MOD2,            MOD3,           MEH(KC_NO),     KC_RALT,    KC_RCTL
		),
	[1] = LAYOUT_ortho_5x12(
		TO(0),     _______,    _______,      _______,     _______,     _______,     _______,      _______,        _______,      _______,    _______,      KC_TRNS,
    KC_TRNS,   _______,    KC_EXLM,      KC_LPRN,     KC_PSLS,     KC_PIPE,     KC_UNDS,      KC_PAST,        KC_RPRN,      KC_DLR,     _______,      KC_TRNS,
    _______,   _______,    KC_AT,        KC_LCBR,     KC_MINS,     KC_BSLS,     KC_SLSH,      KC_PPLS,        KC_RCBR,      KC_PERC,    KC_COLN,      _______,
    KC_TRNS,   _______,    KC_HASH,      KC_LBRC,     KC_GRV,      KC_QUOT,     KC_DQUO,      KC_EQL,         KC_RBRC,      KC_AMPR,    KC_QUES,      KC_TRNS,
    KC_TRNS,   KC_TRNS,    KC_TRNS,      _______,     _______,     _______,     _______,      WINTAB,   _______,      KC_TRNS,    KC_TRNS,      KC_TRNS
		),
	[2] = LAYOUT_ortho_5x12(
		TO(0),      _______,     _______,     _______,     _______,     _______,     _______,      _______,      KC_PSLS,        KC_PAST,    KC_PMNS,    KC_TRNS,
    KC_TRNS,    KC_1,        KC_2,        KC_3,        KC_4,        KC_5,        KC_6,         KC_7,         KC_8,           KC_9,       KC_PPLS,    KC_TRNS,
    KC_TRNS,    _______,     _______,     _______,     _______,     _______,     _______,      KC_4,         KC_5,           KC_6,       KC_PCMM,    KC_TRNS,
    KC_TRNS,    _______,     _______,     _______,     _______,     _______,     _______,      KC_1,         KC_2,           KC_3,       KC_PEQL,    KC_ENT,
    KC_TRNS,    KC_TRNS,     KC_TRNS,     _______,     _______,     _______,     _______,      KC_0,         KC_0,           KC_PDOT,    KC_PENT,    KC_TRNS
		),
	[3] = LAYOUT_ortho_5x12(
		TO(0),        _______,     _______,     _______,   _______,       _______,      _______,     _______,     _______,      _______,       _______,      KC_TRNS,
    _______,      KC_F1,       KC_F2,       KC_F3,     KC_F4,         KC_F5,       KC_F6,      KC_VOLD,    KC_VOLU,     KC_MUTE,     _______,      KC_TRNS,
    KC_TRNS,      KC_F7,       KC_F8,       KC_F9,     KC_F10,        KC_F11,      KC_F12,     KC_MPRV,    KC_MPLY,     KC_MNXT,     _______,      KC_TRNS,
    KC_TRNS,      KC_F13,      KC_F14,      KC_F15,    KC_F16,        KC_F17,      KC_F18,    _______,     _______,      _______,       _______,      KC_TRNS,
    KC_TRNS,      KC_TRNS,     KC_TRNS,     MO(5),     KC_TRNS,       _______,     _______,       KC_TRNS,    KC_TRNS,     KC_TRNS,      KC_TRNS,    KC_TRNS
    ),
	[4] = LAYOUT_ortho_5x12(
		TO(0),     _______,     _______,      _______,   _______,   _______,   _______,     _______,     _______,      _______,     _______,      KC_TRNS,
    KC_TRNS,   _______,     _______,      _______,   _______,   _______,    KC_HOME,    KC_PGUP,    KC_UP,       KC_PGDN,   _______,      KC_TRNS,
    KC_TRNS,   _______,     _______,      _______,   _______,   _______,    KC_END,     KC_LEFT,    KC_DOWN,     KC_RGHT,    KC_HOME,    KC_TRNS,
    KC_TRNS,   _______,     _______,      _______,   _______,   _______,   _______,     _______,     _______,      _______,      KC_END,     KC_ENT,
    KC_TRNS,    KC_TRNS,    KC_TRNS,    _______,   _______,   _______,   _______,     _______,     _______,       KC_TRNS,    KC_TRNS,    KC_TRNS
		),
	[5] = LAYOUT_ortho_5x12(
		TO(0),    RGB_M_P,    RGB_M_B,    RGB_M_R,    RGB_M_SW,    RGB_M_SN,    RGB_M_K,    RGB_M_G,    RGB_M_T,     RGB_SPD,    RGB_SPI,    RGB_TOG,
    _______,    RESET,      DEBUG,     _______,      KC_TRNS,     GUI_OFF,     GUI_ON,     TERM_ON,    TERM_OFF,    RGB_RMOD,   RGB_MOD,   _______,
    _______,   _______,      MU_MOD,     AU_ON,      AU_OFF,      AG_NORM,     AG_SWAP,   _______,     _______,       RGB_HUD,    RGB_HUI,   _______,
    _______,    MUV_DE,     MUV_IN,     MU_ON,      MU_OFF,      MI_ON,       MI_OFF,    _______,     _______,       RGB_SAD,    RGB_SAI,   _______,
    DF(0),   _______,     _______,     _______,     _______,      _______,      _______,     _______,     _______,       RGB_VAD,    RGB_VAI,   _______
		)
};

// Determine the tapdance state to return
td_state_t cur_dance(qk_tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return TD_SINGLE_TAP;
        else return TD_SINGLE_HOLD;
    }

    if (state->count == 2) return TD_DOUBLE_SINGLE_TAP;
    else return TD_UNKNOWN; // Any number higher than the maximum state value you return above
}

// Handle the possible states for each tapdance keycode you define:
void osm_ls_l1_finished(qk_tap_dance_state_t *state, void *user_data) {
    td_state = cur_dance(state);
    switch (td_state) {
        case TD_SINGLE_TAP:
            set_oneshot_mods(MOD_BIT(KC_LSHIFT));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LSHIFT)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_SINGLE_TAP: // Allow nesting of 2 parens `((` within tapping term
            set_oneshot_layer(1, ONESHOT_START);
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
            unregister_code16(KC_LSHIFT);
            clear_oneshot_mods(); // For a layer-tap key, use `layer_off(_MY_LAYER)` here
            break;
        case TD_DOUBLE_SINGLE_TAP:
            clear_oneshot_layer_state(ONESHOT_PRESSED);
            break;
        default:
            break;
    }
}

// Define `ACTION_TAP_DANCE_FN_ADVANCED()` for each tapdance keycode, passing in `finished` and `reset` functions
qk_tap_dance_action_t tap_dance_actions[] = {
    [OSM_LS_L1] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, osm_ls_l1_finished, osm_ls_l1_reset)
};
