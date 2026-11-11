// Tests the actual callback bodies extracted from keymap.c by run.py.
// This does not simulate QMK's event pipeline or resolve modifier ownership.
#include "qmk_stub.h"

#define KC_LCTL 0
#define KC_LSFT 1
#define KC_LALT 2
#define KC_ENT 3
#define MOD_BIT(k) (1u << (k))
#define _SYM 2
#define ONESHOT_START 1
#define ONESHOT_PRESSED 1

typedef struct { uint8_t count; bool pressed, interrupted; } tap_dance_state_t;
static uint8_t pending_mods, held_mods;
static uint16_t now;
static bool oneshot_enabled = true;
static bool symbols_held, symbols_oneshot, enter_pressed;

static uint8_t get_oneshot_mods(void) { return pending_mods; }
static void set_oneshot_mods(uint8_t mods) { if (oneshot_enabled) pending_mods = mods; }
static void del_oneshot_mods(uint8_t mods) { pending_mods &= ~mods; }
static void register_mods(uint8_t mods) { held_mods |= mods; }
static void unregister_mods(uint8_t mods) { held_mods &= ~mods; }
static uint16_t timer_read(void) { return now; }
static uint16_t timer_elapsed(uint16_t start) { return now - start; }
static void set_oneshot_layer(uint8_t layer, uint8_t state) {
    assert(layer == _SYM && state == ONESHOT_START);
    symbols_oneshot = true;
}
static void clear_oneshot_layer_state(uint8_t state) {
    assert(state == ONESHOT_PRESSED);
}
static void layer_on(uint8_t layer) { assert(layer == _SYM); symbols_held = true; }
static void layer_off(uint8_t layer) { assert(layer == _SYM); symbols_held = false; }
static void register_code16(uint16_t code) { assert(code == KC_ENT); enter_pressed = true; }
static void unregister_code16(uint16_t code) { assert(code == KC_ENT); enter_pressed = false; }

#include "tap_dance_under_test.inc"

static void dance(unsigned count, bool shift, bool hold) {
    tap_dance_state_t state = {.count = count, .pressed = hold};
    if (shift) shift_sym_finished(&state, NULL);
    else ctrl_alt_finished(&state, NULL);
    state.pressed = false;
    if (shift) shift_sym_reset(&state, NULL);
    else ctrl_alt_reset(&state, NULL);
}

int main(void) {
    const uint8_t shift = MOD_BIT(KC_LSFT), ctrl = MOD_BIT(KC_LCTL), alt = MOD_BIT(KC_LALT);
    for (unsigned count = 1; count <= 3; count++) {
        const uint8_t tapped = count == 1 ? ctrl : ctrl | alt;
        const uint8_t held = count == 1 ? ctrl : count == 2 ? alt : ctrl | alt;
        // Ctrl/Alt gestures stack with Shift in either order.
        for (unsigned reverse = 0; reverse < 2; reverse++) {
            pending_mods = held_mods = 0;
            dance(reverse ? 1 : count, reverse, false);
            dance(reverse ? count : 1, !reverse, false);
            assert(pending_mods == (tapped | shift) && held_mods == 0);
        }
        // Hold release keeps unrelated one-shots, removing only its own bits.
        pending_mods = ctrl | shift | alt;
        tap_dance_state_t state = {.count = count, .pressed = true};
        ctrl_alt_finished(&state, NULL);
        assert(held_mods == held);
        ctrl_alt_reset(&state, NULL);
        assert(pending_mods == ((ctrl | shift | alt) & ~held) && held_mods == 0);
    }
    pending_mods = ctrl | shift | alt;
    dance(1, true, true);
    assert(pending_mods == (ctrl | alt) && held_mods == 0);

    // Rapid interrupted Shift preserves the grace-period behavior.
    pending_mods = ctrl;
    now = 20;
    shift_sym_timer = 0;
    tap_dance_state_t state = {.count = 1, .pressed = true, .interrupted = true};
    shift_sym_finished(&state, NULL);
    shift_sym_reset(&state, NULL);
    assert(pending_mods == (ctrl | shift) && held_mods == 0);
    now = SHIFT_HOLD_GRACE_MS;
    shift_sym_finished(&state, NULL);
    assert(held_mods == shift);
    shift_sym_reset(&state, NULL);
    assert(held_mods == 0);
    shift_sym_on_each_tap(&state, NULL);
    assert(shift_sym_timer == now);

    pending_mods = held_mods = 0;
    oneshot_enabled = false;
    dance(1, true, false);
    dance(3, false, false);
    assert(pending_mods == 0);
    oneshot_enabled = true;
    pending_mods = shift;
    dance(1, false, false);
    dance(1, false, false);
    assert(pending_mods == (ctrl | shift));

    state = (tap_dance_state_t){.count = 1, .pressed = true};
    enter_ctrl_finished(&state, NULL);
    assert(held_mods == ctrl);
    enter_ctrl_reset(&state, NULL);
    assert(held_mods == 0 && pending_mods == (ctrl | shift));
    state.pressed = false;
    enter_ctrl_finished(&state, NULL);
    assert(enter_pressed);
    enter_ctrl_reset(&state, NULL);
    assert(!enter_pressed);

    dance(2, true, false);
    assert(symbols_oneshot && !symbols_held);
    state = (tap_dance_state_t){.count = 2, .pressed = true};
    shift_sym_finished(&state, NULL);
    assert(symbols_held);
    shift_sym_reset(&state, NULL);
    assert(!symbols_held);
    for (unsigned count = 4; count <= 5; count++) {
        pending_mods = held_mods = 0;
        dance(count, false, false);
        assert(pending_mods == 0 && held_mods == 0);
    }
    puts("Tap-dance callback regression tests passed");
}
