// Exercise production QMK tap-dance events, not just isolated user callbacks.
// The surrounding matrix/layer-tap engine and USB reports are modeled here.
#include "qmk_stub.h"
#include "keycodes.h"
#include "../config.h"

#define MOD_BIT(k) (1u << ((k) & 7))
#define _SYM 2
#define ONESHOT_START 1
#define ONESHOT_PRESSED 1
#define TAP_CODE_DELAY 0
#define GET_TAPPING_TERM(code, record) TAPPING_TERM
#define QK_TAP_DANCE_GET_INDEX(code) ((code) & 0xff)

typedef struct {
    uint16_t interrupting_keycode;
    uint8_t count, weak_mods, oneshot_mods;
    bool pressed, finished, interrupted, in_use;
    uint8_t index;
} tap_dance_state_t;
typedef struct { struct { bool pressed; } event; } keyrecord_t;
typedef void (*tap_dance_user_fn_t)(tap_dance_state_t *, void *);
typedef struct {
    struct {
        tap_dance_user_fn_t on_each_tap, on_dance_finished, on_reset, on_each_release;
    } fn;
    void *user_data;
} tap_dance_action_t;

static uint8_t mods, weak_mods, oneshot_mods, report_mods;
static uint16_t now, oneshot_time;
static bool oneshot_enabled = true, key_down;
static uint8_t get_mods(void) { return mods; }
static uint8_t get_weak_mods(void) { return weak_mods; }
static uint8_t get_oneshot_mods(void) { return oneshot_mods; }
static void set_oneshot_mods(uint8_t bits) {
    if (oneshot_enabled && oneshot_mods != bits) {
        oneshot_mods = bits;
        oneshot_time = now;
    }
}
static void del_oneshot_mods(uint8_t bits) { oneshot_mods &= ~bits; }
static void add_mods(uint8_t bits) { mods |= bits; }
static void del_mods(uint8_t bits) { mods &= ~bits; }
static void register_mods(uint8_t bits) { add_mods(bits); }
static void unregister_mods(uint8_t bits) { del_mods(bits); }
static void add_weak_mods(uint8_t bits) { weak_mods |= bits; }
static void del_weak_mods(uint8_t bits) { weak_mods &= ~bits; }
static void clear_weak_mods(void) { weak_mods = 0; }
static void send_keyboard_report(void) {
    if ((uint16_t)(now - oneshot_time) >= ONESHOT_TIMEOUT) oneshot_mods = 0;
    report_mods = mods | weak_mods | oneshot_mods;
    if (key_down) oneshot_mods = 0;
}
static uint16_t timer_read(void) { return now; }
static uint16_t timer_elapsed(uint16_t start) { return now - start; }
static void set_oneshot_layer(uint8_t layer, uint8_t state) { (void)layer; (void)state; }
static void clear_oneshot_layer_state(uint8_t state) { (void)state; }
static void layer_on(uint8_t layer) { (void)layer; }
static void layer_off(uint8_t layer) { (void)layer; }
static void register_code16(uint16_t code) { (void)code; }
static void unregister_code16(uint16_t code) { (void)code; }

#include "tap_dance_under_test.inc"

static tap_dance_action_t actions[] = {
    {.fn = {shift_sym_on_each_tap, shift_sym_finished, shift_sym_reset, NULL}},
    {.fn = {NULL, enter_ctrl_finished, enter_ctrl_reset, NULL}},
    {.fn = {NULL, ctrl_alt_finished, ctrl_alt_reset, NULL}},
};
static uint8_t tap_dance_count(void) { return ARRAY_SIZE(actions); }
static tap_dance_action_t *tap_dance_get(uint8_t index) { return &actions[index]; }

#include "tap_dance_core_under_test.inc"

#define SHIFT_DANCE (QK_TAP_DANCE + 0)
#define CTRL_DANCE (QK_TAP_DANCE + 2)
#define NUM_THUMB (QK_LAYER_TAP | (3 << 8) | KC_BSPC)
#define FUNC_THUMB (QK_LAYER_TAP | (4 << 8) | KC_DEL)

static void event(uint16_t code, bool pressed) {
    keyrecord_t record = {.event = {.pressed = pressed}};
    preprocess_tap_dance(code, &record);
    process_tap_dance(code, &record);
}
static void reset(void) {
    memset(tap_dance_states, 0, sizeof(tap_dance_states));
    active_td = last_tap_time = now = oneshot_time = 0;
    mods = weak_mods = oneshot_mods = report_mods = 0;
    key_down = false;
    shift_sym_state = enter_ctrl_state = ctrl_alt_state = TD_NONE;
}
static void arrow(void) {
    event(KC_LEFT, true);
    key_down = true;
    send_keyboard_report();
    assert(report_mods & MOD_BIT(KC_LSFT));
    event(KC_LEFT, false);
    key_down = false;
    send_keyboard_report();
}
static void test_shift_then_layer(uint16_t layer_key, uint16_t delay) {
    reset();
    event(SHIFT_DANCE, true);
    now = delay;
    tap_dance_task();
    event(layer_key, true);
    // A layer switch must resolve a physically held Shift as a real hold.
    assert(mods & MOD_BIT(KC_LSFT));
    now = 300; // Model waiting for the layer-tap hold to activate.
    arrow();
    now = 400;
    arrow();
    now = 4000; // The one-shot timeout must not expire a held modifier.
    arrow();
    event(SHIFT_DANCE, false);
    assert(!(mods & MOD_BIT(KC_LSFT)));
    event(layer_key, false);
}
static void test_modifier_chord(void) {
    reset();
    event(SHIFT_DANCE, true);
    now = 20;
    event(CTRL_DANCE, true);
    assert(mods == MOD_BIT(KC_LSFT));
    now = 40;
    event(NUM_THUMB, true);
    assert(mods == (MOD_BIT(KC_LSFT) | MOD_BIT(KC_LCTL)));
    now = 4000;
    arrow();
    assert(report_mods == (MOD_BIT(KC_LSFT) | MOD_BIT(KC_LCTL)));
    event(CTRL_DANCE, false);
    arrow();
    assert(report_mods == MOD_BIT(KC_LSFT));
    event(SHIFT_DANCE, false);
    assert(mods == 0);
}
static void test_ctrl_alt_holds(void) {
    for (unsigned count = 1; count <= 3; count++) {
        reset();
        for (unsigned tap = 1; tap < count; tap++) {
            event(CTRL_DANCE, true);
            now += 20;
            event(CTRL_DANCE, false);
            now += 20;
        }
        event(CTRL_DANCE, true);
        now += 20;
        event(NUM_THUMB, true);
        uint8_t expected = count == 1 ? MOD_BIT(KC_LCTL) : count == 2 ? MOD_BIT(KC_LALT) : MOD_BIT(KC_LCTL) | MOD_BIT(KC_LALT);
        assert(mods == expected && oneshot_mods == 0);
        for (unsigned press = 0; press < 3; press++) {
            now += ONESHOT_TIMEOUT + 100;
            event(KC_LEFT, true);
            key_down = true;
            send_keyboard_report();
            assert(report_mods == expected);
            event(KC_LEFT, false);
            key_down = false;
        }
        event(CTRL_DANCE, false);
        assert(mods == 0);
    }
}
static void test_typing_rollover(void) {
    reset();
    event(SHIFT_DANCE, true);
    now = 20;
    event(KC_A, true);
    assert(mods == 0 && oneshot_mods == MOD_BIT(KC_LSFT));
    key_down = true;
    send_keyboard_report();
    assert(report_mods == MOD_BIT(KC_LSFT));
    event(KC_A, false);
    key_down = false;
    event(SHIFT_DANCE, false);
    send_keyboard_report();
    assert(mods == 0 && oneshot_mods == 0 && report_mods == 0);
}
static void test_tap_and_timeout(void) {
    reset();
    event(SHIFT_DANCE, true);
    now = 30;
    event(SHIFT_DANCE, false);
    now = 250;
    tap_dance_task();
    assert(mods == 0 && oneshot_mods == MOD_BIT(KC_LSFT));
    arrow(); // Only the first key consumes a tapped one-shot.
    assert(oneshot_mods == 0 && mods == 0);
    reset();
    event(SHIFT_DANCE, true);
    now = 30;
    event(SHIFT_DANCE, false);
    now = 250;
    tap_dance_task();
    now = 3300;
    send_keyboard_report();
    assert(report_mods == 0 && oneshot_mods == 0);
}
int main(void) {
    test_shift_then_layer(NUM_THUMB, 20);
    test_shift_then_layer(FUNC_THUMB, 20);
    test_shift_then_layer(NUM_THUMB, 250);
    test_shift_then_layer(QK_MOMENTARY | 5, 20);
    test_modifier_chord();
    test_ctrl_alt_holds();
    test_typing_rollover();
    test_tap_and_timeout();
    puts("Tap-dance event regression tests passed");
}
