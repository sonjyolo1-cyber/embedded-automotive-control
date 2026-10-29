/*
 * test_buzzer.c - buzzer tick logic with fake ticks. Runs on the PC.
 * The last tests drive the buzzer from the real blink and turn_signal code.
 */
#include "unity.h"
#include "buzzer.h"
#include "blink.h"
#include "turn_signal.h"

#define P  BUZZER_PULSE_MS

static buzzer_t bz;

void setUp(void)    { buzzer_init(&bz, 0U); }
void tearDown(void) {}

static uint8_t at(uint8_t lamp, uint32_t now) { return buzzer_update(&bz, lamp, now); }

/* ================= Basic edges ================= */
static void test_lamp_off_stays_silent(void)
{
    uint32_t t;
    for (t = 0U; t < 2000U; t++) {
        TEST_ASSERT_EQUAL_UINT8(0U, at(0U, t));
    }
}

static void test_lamp_on_edge_gives_pulse_of_pulse_ms(void)
{
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, 100U));            /* "tich" at once  */
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, 100U + P - 1U));   /* still in pulse  */
    TEST_ASSERT_EQUAL_UINT8(0U, at(1U, 100U + P));        /* pulse over      */
    TEST_ASSERT_EQUAL_UINT8(0U, at(1U, 300U));            /* lamp still on: silent, no DC in the coil */
}

static void test_lamp_off_edge_gives_pulse_too(void)
{
    at(1U, 100U);
    at(1U, 200U);
    TEST_ASSERT_EQUAL_UINT8(1U, at(0U, 433U));            /* "tac" */
    TEST_ASSERT_EQUAL_UINT8(0U, at(0U, 433U + P));
}

static void test_any_nonzero_lamp_value_counts_as_on(void)
{
    TEST_ASSERT_EQUAL_UINT8(1U, at(0xFFU, 10U));
    TEST_ASSERT_EQUAL_UINT8(0U, at(1U, 10U + P));         /* 0xFF -> 1 is not an edge */
}

/* ================= Failure cases ================= */
/* blink catching up after a stall: lamp toggles several times in the same ms.
 * The pin must stay 1 (one click), not chatter. */
static void test_edges_in_same_ms_give_one_pulse(void)
{
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, 1000U));
    TEST_ASSERT_EQUAL_UINT8(1U, at(0U, 1000U));
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, 1000U));
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, 1000U + P - 1U));
    TEST_ASSERT_EQUAL_UINT8(0U, at(1U, 1000U + P));
}

static void test_tick_wraparound(void)
{
    const uint32_t start = 0xFFFFFFFFU;
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, start));
    TEST_ASSERT_EQUAL_UINT8(1U, at(1U, start + P - 1U));  /* wrapped */
    TEST_ASSERT_EQUAL_UINT8(0U, at(1U, start + P));
}

static void test_null_gives_coil_off(void)
{
    TEST_ASSERT_EQUAL_UINT8(0U, buzzer_update(NULL, 1U, 0U));
}

/* ================= With the real blink + turn_signal ================= */
/* Run main's pipeline for every ms in [from, to): count pulses (pin 0 -> 1). */
static uint32_t count_clicks(const turn_signal_t *ts, blink_t *b, uint32_t from, uint32_t to)
{
    uint32_t t, clicks = 0U;
    uint8_t prev_pin = 0U;
    for (t = from; t < to; t++) {
        blink_out_t out = blink_update(b, ts->state, t);
        uint8_t pin = buzzer_update(&bz, (uint8_t)(out.left || out.right), t);
        if ((pin != 0U) && (prev_pin == 0U)) {
            clicks++;
        }
        prev_pin = pin;
    }
    return clicks;
}

static void test_left_for_1400ms_clicks_on_every_lamp_edge(void)
{
    turn_signal_t ts; blink_t b;
    turn_signal_init(&ts); blink_init(&b, 0U);
    turn_signal_step(&ts, EVENT_LEFT);
    /* lamp edges at 0 (on), 333, 666, 999, 1332 -> 5 clicks */
    TEST_ASSERT_EQUAL_UINT32(5U, count_clicks(&ts, &b, 0U, 1400U));
}

static void test_off_state_is_silent_with_real_blink(void)
{
    turn_signal_t ts; blink_t b;
    turn_signal_init(&ts); blink_init(&b, 0U);
    TEST_ASSERT_EQUAL_UINT32(0U, count_clicks(&ts, &b, 0U, 3000U));
}

static void test_hazard_clicks_once_per_edge_not_twice(void)
{
    turn_signal_t ts; blink_t b;
    turn_signal_init(&ts); blink_init(&b, 0U);
    turn_signal_step(&ts, EVENT_HAZARD);
    /* both sides switch together: still one click per edge */
    TEST_ASSERT_EQUAL_UINT32(5U, count_clicks(&ts, &b, 0U, 1400U));
}

/* Design decision (2026-10-22): switching side while the lamp is lit keeps
 * lamp_on = 1, so there is no click at the switch. The next click comes at
 * the next lamp edge. */
static void test_switch_side_while_lit_has_no_extra_click(void)
{
    turn_signal_t ts; blink_t b;
    turn_signal_init(&ts); blink_init(&b, 0U);
    turn_signal_step(&ts, EVENT_LEFT);
    TEST_ASSERT_EQUAL_UINT32(1U, count_clicks(&ts, &b, 0U, 100U));     /* "tich" at 0       */
    turn_signal_step(&ts, EVENT_RIGHT);                                /* RIGHT on at once  */
    TEST_ASSERT_EQUAL_UINT32(0U, count_clicks(&ts, &b, 100U, 433U));   /* no click at 100   */
    TEST_ASSERT_EQUAL_UINT32(1U, count_clicks(&ts, &b, 433U, 440U));   /* "tac" at 100+333  */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_lamp_off_stays_silent);
    RUN_TEST(test_lamp_on_edge_gives_pulse_of_pulse_ms);
    RUN_TEST(test_lamp_off_edge_gives_pulse_too);
    RUN_TEST(test_any_nonzero_lamp_value_counts_as_on);
    RUN_TEST(test_edges_in_same_ms_give_one_pulse);
    RUN_TEST(test_tick_wraparound);
    RUN_TEST(test_null_gives_coil_off);
    RUN_TEST(test_left_for_1400ms_clicks_on_every_lamp_edge);
    RUN_TEST(test_off_state_is_silent_with_real_blink);
    RUN_TEST(test_hazard_clicks_once_per_edge_not_twice);
    RUN_TEST(test_switch_side_while_lit_has_no_extra_click);
    return UNITY_END();
}
