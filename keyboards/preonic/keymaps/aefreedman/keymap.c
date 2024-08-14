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

// layer name order needs to match the layer songs order or weird things happen
enum preonic_layers {
  _QWERTY,
  _COLEMAK,
  _SYMBOLS,
  _NUM,
  _FUNC,
  _NAV,
  _ADJ
};

enum preonic_keycodes {
  QWERTY = SAFE_RANGE,
  COLEMAK,
//   DVORAK,
  LOWER,
  RAISE,
//   BACKLIT
};

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


#define OSM_LS OSM  (MOD_LSFT)
#define WINTAB LGUI(KC_TAB)
#define L_SYM LT(_SYMBOLS, KC_MINS)
#define L_NAV LT(_NAV, KC_BSPC)
#define L_FUNC LT(_FUNC, KC_DEL)
#define L_NUM LT(_NUM, KC_QUOT)
#define TD_OSM_LSL1 TD(OSM_LS_L1)


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Default QWERTY
	[_QWERTY] = LAYOUT_ortho_5x12(
		QK_GESC,	    KC_1,	    KC_2,	    KC_3,	        KC_4,	        KC_5,	    KC_6,	    KC_7,	         KC_8,	         KC_9,	         KC_0,		 XXXXXXX,
		KC_TAB,	 	    KC_Q,	    KC_W,	    KC_E,	        KC_R,	        KC_T,	    KC_Y,	    KC_U,	         KC_I,	         KC_O,	         KC_P,		 COLEMAK,
		KC_TAB,	    	KC_A, 	    KC_S,	    KC_D,	        KC_F,	        KC_G,	    KC_H,	    KC_J,	         KC_K,	         KC_L,	         KC_SCLN,	 L_NUM,
		C_S_T(KC_TAB),  KC_Z,	    KC_X,	    KC_C,	        KC_V,	        KC_B,	    KC_N,	    KC_M,	         KC_COMM,        KC_DOT,         KC_SLSH,    KC_ENT,
		KC_LCTL,        KC_LALT,    KC_LGUI,    TD_OSM_LSL1,    TD_OSM_LSL1,    KC_SPC,     KC_SPC,     L_NAV,           L_FUNC,           MEH(KC_NO),     KC_RALT,    KC_RCTL
		),

    
    // Symbols layer
	[_SYMBOLS] = LAYOUT_ortho_5x12(
	_______,     _______,    _______,      _______,     _______,     _______,     _______,      _______,        _______,      _______,    _______,      XXXXXXX,
    _______,   _______,    KC_EXLM,      KC_LPRN,     KC_PSLS,     KC_PIPE,     KC_UNDS,      KC_PAST,        KC_RPRN,      KC_DLR,     _______,      XXXXXXX,
    _______,   _______,    KC_AT,        KC_LCBR,     KC_MINS,     KC_BSLS,     KC_SLSH,      KC_PPLS,        KC_RCBR,      KC_PERC,    KC_COLN,      _______,
    _______,   _______,    KC_HASH,      KC_LBRC,     KC_GRV,      KC_QUOT,     KC_DQUO,      KC_EQL,         KC_RBRC,      KC_AMPR,    KC_QUES,      _______,
    _______,   _______,    _______,      _______,     _______,     _______,     _______,      WINTAB,   _______,      _______,    _______,      _______
		),

    // Numpad layer
	[_NUM] = LAYOUT_ortho_5x12(
	_______,    _______,     _______,     _______,     _______,     _______,     _______,      _______,     KC_PSLS,         KC_PAST,    KC_PMNS,    XXXXXXX,
    _______,    KC_1,        KC_2,        KC_3,        KC_4,        KC_5,        KC_6,         KC_7,         KC_8,           KC_9,       KC_PPLS,    XXXXXXX,
    _______,    _______,     _______,     _______,     _______,     _______,     _______,      KC_4,         KC_5,           KC_6,       KC_PCMM,    _______,
    _______,    _______,     _______,     _______,     _______,     _______,     _______,      KC_1,         KC_2,           KC_3,       KC_PEQL,    KC_ENT,
    _______,    _______,     _______,     _______,     _______,     _______,     _______,      KC_0,         KC_0,           KC_PDOT,    KC_PENT,    _______
		),

    // Function layer
	[_FUNC] = LAYOUT_ortho_5x12(
	_______,       _______,    _______,     _______,   _______,       _______,     _______,    _______,    _______,     _______,     _______,      XXXXXXX,
    _______,      KC_F1,       KC_F2,       KC_F3,     KC_F4,         KC_F5,       KC_F6,      KC_VOLD,    KC_VOLU,     KC_MUTE,     _______,      XXXXXXX,
    _______,      KC_F7,       KC_F8,       KC_F9,     KC_F10,        KC_F11,      KC_F12,     KC_MPRV,    KC_MPLY,     KC_MNXT,     _______,      _______,
    _______,      KC_F13,      KC_F14,      KC_F15,    KC_F16,        KC_F17,      KC_F18,    _______,     _______,      _______,       _______,      _______,
    _______,      _______,     _______,     MO(_ADJ),     _______,       _______,     _______,       _______,    _______,     _______,      _______,    _______
    ),

    // Navigation layer
	[_NAV] = LAYOUT_ortho_5x12(
	_______,     _______,     _______,      _______,   _______,   _______,   _______,     _______,     _______,      _______,     _______,      _______,
    _______,   _______,     _______,      _______,   _______,   _______,    KC_HOME,    KC_PGUP,    KC_UP,       KC_PGDN,   _______,      _______,
    _______,   _______,     _______,      _______,   _______,   _______,    KC_END,     KC_LEFT,    KC_DOWN,     KC_RGHT,    KC_HOME,    _______,
    _______,   _______,     _______,      _______,   _______,   _______,   _______,     _______,     _______,      _______,      KC_END,     KC_ENT,
    _______,    _______,    _______,    _______,   _______,   _______,   _______,     _______,     _______,       _______,    _______,    _______
		),

    /* Keyboard settings layer
    * ,-----------------------------------------------------------------------------------.
    * |      |      |      |      |      |      |      |      |      |      |      |      |
    * |------+------+------+------+------+------+------+------+------+------+------+------|
    * |      | Reset| Debug|      |      |      |      |      |      |      |      |      |
    * |------+------+------+------+------+-------------+------+------+------+------+------|
    * |      |      |Aud cy|Aud on|AudOff|AGnorm|AGswap|      |      |      |      |      |
    * |------+------+------+------+------+------|------+------+------+------+------+------|
    * |      |Voice-|Voice+|Mus on|MusOff|MidiOn|MidOff|      |      |      |      |      |
    * |------+------+------+------+------+------+------+------+------+------+------+------|
    * |      |      |      |      |      |             |      |      |      |      |      |
    * `-----------------------------------------------------------------------------------'
    */
[_ADJ] = LAYOUT_ortho_5x12(
	_______,    RGB_M_P,    RGB_M_B,    RGB_M_R,    RGB_M_SW,    RGB_M_SN,    RGB_M_K,    RGB_M_G,    RGB_M_T,     RGB_SPD,    RGB_SPI,    RGB_TOG,
    _______,    QK_BOOT,      DB_TOGG,     _______,      _______,     GUI_OFF,     GUI_ON,     _______,    _______,    RGB_RMOD,   RGB_MOD,   _______,
    _______,   _______,      MU_NEXT,     AU_ON,      AU_OFF,      AG_NORM,     AG_SWAP,   _______,     _______,       RGB_HUD,    RGB_HUI,   _______,
    _______,    AU_PREV,     AU_NEXT,     MU_ON,      MU_OFF,      MI_ON,       MI_OFF,    _______,     _______,       RGB_SAD,    RGB_SAI,   _______,
    _______,   _______,     _______,     _______,     _______,      _______,      _______,     _______,     _______,       RGB_VAD,    RGB_VAI,   _______
		),


    /* Colemak DH
    * ,-----------------------------------------------------------------------------------.
    * |   `  |   1  |   2  |   3  |   4  |   5  |   6  |   7  |   8  |   9  |   0  | Bksp |
    * |------+------+------+------+------+------+------+------+------+------+------+------|
    * | Tab  |   Q  |   W  |   F  |   P  |   B  |   J  |   L  |   U  |   Y  |   ;  | Del  |
    * |------+------+------+------+------+-------------+------+------+------+------+------|
    * | Esc  |   A  |   R  |   S  |   T  |   G  |   M  |   N  |   E  |   I  |   O  |  "   |
    * |------+------+------+------+------+------|------+------+------+------+------+------|
    * | Shift|   Z  |   X  |   C  |   D  |   V  |   K  |   H  |   ,  |   .  |   /  |Enter |
    * |------+------+------+------+------+------+------+------+------+------+------+------|
    * | Brite| Ctrl | Alt  | GUI  |Lower |    Space    |Raise | Left | Down |  Up  |Right |
    * `-----------------------------------------------------------------------------------'
    */
    [_COLEMAK] = LAYOUT_preonic_grid(
        QK_GESC,          KC_1,    KC_2,        KC_3,           KC_4,           KC_5,    KC_6,    KC_7,    KC_8,        KC_9,       KC_0,       QWERTY,
        KC_TAB,           KC_Q,    KC_W,        KC_F,           KC_P,           KC_B,    KC_J,    KC_L,    KC_U,        KC_Y,       KC_SCLN,    XXXXXXX,
        C_S_T(KC_TAB),    KC_A,    KC_R,        KC_S,           KC_T,           KC_G,    KC_M,    KC_N,    KC_E,        KC_I,       KC_O,       L_NUM,
        C_S_T(KC_TAB),    KC_Z,    KC_X,        KC_C,           KC_D,           KC_V,    KC_K,    KC_H,    KC_COMM,     KC_DOT,     KC_SLSH,    KC_ENT,
        KC_LCTL,          KC_LALT, KC_LGUI,     TD_OSM_LSL1,    TD_OSM_LSL1,    KC_SPC,  KC_SPC,  L_NAV,   L_FUNC,      MEH(KC_NO), KC_RALT,    KC_RCTL
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
            set_oneshot_mods(MOD_BIT(KC_LSFT));
            break;
        case TD_SINGLE_HOLD:
            register_mods(MOD_BIT(KC_LSFT)); // For a layer-tap key, use `layer_on(_MY_LAYER)` here
            break;
        case TD_DOUBLE_SINGLE_TAP: // Allow nesting of 2 parens `((` within tapping term
            set_oneshot_layer(_SYMBOLS, ONESHOT_START);
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
            unregister_code16(KC_LSFT);
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

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
        case QWERTY:
          if (record->event.pressed) {
            set_single_persistent_default_layer(_QWERTY);
          }
          return false;
          break;
        case COLEMAK:
          if (record->event.pressed) {
            set_single_persistent_default_layer(_COLEMAK);
          }
          return false;
          break;
        // case DVORAK:
        //   if (record->event.pressed) {
        //     set_single_persistent_default_layer(_DVORAK);
        //   }
        //   return false;
        //   break;
        // case LOWER:
        //   if (record->event.pressed) {
        //     layer_on(_LOWER);
        //     update_tri_layer(_LOWER, _RAISE, _ADJUST);
        //   } else {
        //     layer_off(_LOWER);
        //     update_tri_layer(_LOWER, _RAISE, _ADJUST);
        //   }
        //   return false;
        //   break;
        // case RAISE:
        //   if (record->event.pressed) {
        //     layer_on(_RAISE);
        //     update_tri_layer(_LOWER, _RAISE, _ADJUST);
        //   } else {
        //     layer_off(_RAISE);
        //     update_tri_layer(_LOWER, _RAISE, _ADJUST);
        //   }
        //   return false;
        //   break;
        // case BACKLIT:
        //   if (record->event.pressed) {
        //     register_code(KC_RSFT);
        //     #ifdef BACKLIGHT_ENABLE
        //       backlight_step();
        //     #endif
        //     #ifdef RGBLIGHT_ENABLE
        //       rgblight_step();
        //     #endif
        //     #ifdef __AVR__
        //     writePinLow(E6);
        //     #endif
        //   } else {
        //     unregister_code(KC_RSFT);
        //     #ifdef __AVR__
        //     writePinHigh(E6);
        //     #endif
        //   }
        //   return false;
        //   break;
      }
    return true;
};

bool muse_mode = false;
uint8_t last_muse_note = 0;
uint16_t muse_counter = 0;
uint8_t muse_offset = 70;
uint16_t muse_tempo = 50;

// bool encoder_update_user(uint8_t index, bool clockwise) {
//   if (muse_mode) {
//     if (IS_LAYER_ON(_RAISE)) {
//       if (clockwise) {
//         muse_offset++;
//       } else {
//         muse_offset--;
//       }
//     } else {
//       if (clockwise) {
//         muse_tempo+=1;
//       } else {
//         muse_tempo-=1;
//       }
//     }
//   } else {
//     if (clockwise) {
//       register_code(KC_PGDN);
//       unregister_code(KC_PGDN);
//     } else {
//       register_code(KC_PGUP);
//       unregister_code(KC_PGUP);
//     }
//   }
//     return true;
// }

// bool dip_switch_update_user(uint8_t index, bool active) {
//     switch (index) {
//         case 0:
//             if (active) {
//                 layer_on(_ADJUST);
//             } else {
//                 layer_off(_ADJUST);
//             }
//             break;
//         case 1:
//             if (active) {
//                 muse_mode = true;
//             } else {
//                 muse_mode = false;
//             }
//     }
//     return true;
// }


void matrix_scan_user(void) {
#ifdef AUDIO_ENABLE
    if (muse_mode) {
        if (muse_counter == 0) {
            uint8_t muse_note = muse_offset + SCALE[muse_clock_pulse()];
            if (muse_note != last_muse_note) {
                stop_note(compute_freq_for_midi_note(last_muse_note));
                play_note(compute_freq_for_midi_note(muse_note), 0xF);
                last_muse_note = muse_note;
            }
        }
        muse_counter = (muse_counter + 1) % muse_tempo;
    } else {
        if (muse_counter) {
            stop_all_notes();
            muse_counter = 0;
        }
    }
#endif
}

bool music_mask_user(uint16_t keycode) {
  switch (keycode) {
    case RAISE:
    case LOWER:
      return false;
    default:
      return true;
  }
}