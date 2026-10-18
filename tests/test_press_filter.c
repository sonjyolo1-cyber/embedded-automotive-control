/*
 * test_press_filter.c - LEFT + RIGHT conflict filter. Runs on the PC.
 * Each test fills one loop pass of button flags by hand, runs the filter once
 * and checks how many events came out (n) and which ones, in order (ev[]).
 */
#include <stddef.h>
#include "unity.h"
#include "press_filter.h"
#include "turn_signal.h"

static press_filter_in_t in;
static event_t ev[EVENT_COUNT];
static uint8_t n;

/* n = 0xFF: a value the filter never returns (valid 0..EVENT_COUNT),
 * so a test that forgets run() fails instead of passing by luck. */
void setUp(void)    { in = (press_filter_in_t){0}; n = 0xFFU; }
void tearDown(void) {}

/* Helper: one filter call, result kept in 'n' and 'ev'. */
static void run(void) { n = press_filter_events(&in, ev); }

/* ================= single button ================= */
static void test_nothing_pressed_gives_nothing(void)
{
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

static void test_left_alone_gives_left(void)
{
    in.left_pressed = 1U;
    in.left_held    = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_LEFT, ev[0]);
}

static void test_right_alone_gives_right(void)
{
    in.right_pressed = 1U;
    in.right_held    = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_RIGHT, ev[0]);
}

static void test_hazard_alone_gives_hazard(void)
{
    in.hazard_pressed = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_HAZARD, ev[0]);
}

/* ================= LEFT + RIGHT conflict ================= */
/* Rule 1: the later button loses. */
static void test_left_dropped_while_right_held(void)
{
    in.left_pressed = 1U;
    in.left_held    = 1U;
    in.right_held   = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

static void test_right_dropped_while_left_held(void)
{
    in.right_pressed = 1U;
    in.right_held    = 1U;
    in.left_held     = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

/* Rule 2: both edges in the same pass. The driver reports both as held too,
 * so rule 1 already drops both. */
static void test_left_and_right_same_pass_gives_nothing(void)
{
    in.left_pressed  = 1U;
    in.right_pressed = 1U;
    in.left_held     = 1U;
    in.right_held    = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

/* Rule 2 coded on its own, not left to the driver: both edges, neither held
 * (e.g. if buttons_update ever moves into an ISR). */
static void test_left_and_right_same_pass_none_held_gives_nothing(void)
{
    in.left_pressed  = 1U;
    in.right_pressed = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

/* ================= HAZARD is never in conflict ================= */
/* Fixed output order LEFT, RIGHT, HAZARD, so the board runs like the test. */
static void test_hazard_and_left_gives_left_then_hazard(void)
{
    in.left_pressed   = 1U;
    in.left_held      = 1U;
    in.hazard_pressed = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(2U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_LEFT,   ev[0]);
    TEST_ASSERT_EQUAL_INT(EVENT_HAZARD, ev[1]);
}

/* Hand across all three buttons. */
static void test_hazard_passes_while_left_and_right_held(void)
{
    in.hazard_pressed = 1U;
    in.left_held      = 1U;
    in.right_held     = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_HAZARD, ev[0]);
}

static void test_hazard_passes_when_left_right_conflict(void)
{
    in.left_pressed   = 1U;
    in.right_pressed  = 1U;
    in.left_held      = 1U;
    in.right_held     = 1U;
    in.hazard_pressed = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_HAZARD, ev[0]);
}

/* ================= corner cases ================= */
/* Cannot happen with the current driver: buttons_update and the reads run in the
 * same pass, so pressed = 1 always comes with held = 1, and a stalled main also
 * stalls buttons_update. Kept as a guard: only the OTHER side's held flag may
 * drop a press, never the button's own held flag. */
static void test_left_press_counts_even_if_left_already_released(void)
{
    in.left_pressed = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_LEFT, ev[0]);
}

/* Holding LEFT blocks RIGHT only while it is held. */
static void test_right_works_again_after_left_released(void)
{
    in.right_pressed = 1U;
    in.right_held    = 1U;
    in.left_held     = 1U;
    run();
    TEST_ASSERT_EQUAL_UINT8(0U, n);

    in.left_held = 0U;                  /* next press, LEFT released */
    run();
    TEST_ASSERT_EQUAL_UINT8(1U, n);
    TEST_ASSERT_EQUAL_INT(EVENT_RIGHT, ev[0]);
}

/* ================= bad arguments ================= */
/* Same guard as turn_signal_step / blink_update: on the chip a NULL access is a HardFault. */
static void test_null_input_gives_nothing(void)
{
    n = press_filter_events(NULL, ev);
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

static void test_null_output_gives_nothing(void)
{
    in.left_pressed = 1U;
    in.left_held    = 1U;
    n = press_filter_events(&in, NULL);
    TEST_ASSERT_EQUAL_UINT8(0U, n);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_nothing_pressed_gives_nothing);
    RUN_TEST(test_left_alone_gives_left);
    RUN_TEST(test_right_alone_gives_right);
    RUN_TEST(test_hazard_alone_gives_hazard);
    RUN_TEST(test_left_dropped_while_right_held);
    RUN_TEST(test_right_dropped_while_left_held);
    RUN_TEST(test_left_and_right_same_pass_gives_nothing);
    RUN_TEST(test_left_and_right_same_pass_none_held_gives_nothing);
    RUN_TEST(test_hazard_and_left_gives_left_then_hazard);
    RUN_TEST(test_hazard_passes_while_left_and_right_held);
    RUN_TEST(test_hazard_passes_when_left_right_conflict);
    RUN_TEST(test_left_press_counts_even_if_left_already_released);
    RUN_TEST(test_right_works_again_after_left_released);
    RUN_TEST(test_null_input_gives_nothing);
    RUN_TEST(test_null_output_gives_nothing);
    return UNITY_END();
}
