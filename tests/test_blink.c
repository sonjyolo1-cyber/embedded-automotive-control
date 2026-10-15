/*
 * test_blink.c - blink logic with fake ticks. Runs on the PC.
 * No SysTick here: each test passes the time (now_ms) by hand.
 */
#include "unity.h"
#include "blink.h"
#include "turn_signal.h"

#define T  BLINK_HALF_PERIOD_MS   /* 333 */

static blink_t b;
static blink_out_t out;

void setUp(void)    { blink_init(&b, 0U); }
void tearDown(void) {}

/* Helper: one call, result kept in 'out'. */
static void at(state_t s, uint32_t now) { out = blink_update(&b, s, now); }

#define EXPECT(l, r)  do { TEST_ASSERT_EQUAL_UINT8((l), out.left);  \
                           TEST_ASSERT_EQUAL_UINT8((r), out.right); } while (0)

/* ================= OFF ================= */
static void test_off_keeps_all_lamps_off(void)
{
    at(STATE_OFF, 0U);      EXPECT(0, 0);
    at(STATE_OFF, T);       EXPECT(0, 0);
    at(STATE_OFF, 2U * T);  EXPECT(0, 0);
}

/* ================= LEFT: timing ================= */
static void test_left_lights_immediately_when_selected(void)
{
    at(STATE_LEFT, 1000U);              EXPECT(1, 0);
}

static void test_left_stays_on_until_half_period(void)
{
    at(STATE_LEFT, 1000U);
    at(STATE_LEFT, 1000U + T - 1U);     EXPECT(1, 0);
}

static void test_left_goes_off_at_half_period(void)
{
    at(STATE_LEFT, 1000U);
    at(STATE_LEFT, 1000U + T);          EXPECT(0, 0);
}

static void test_left_on_again_after_full_period(void)
{
    at(STATE_LEFT, 1000U);
    at(STATE_LEFT, 1000U + T);          /* off */
    at(STATE_LEFT, 1000U + 2U * T);     EXPECT(1, 0);
}

/* ================= RIGHT / HAZARD: which side ================= */
static void test_right_lights_only_right_side(void)
{
    at(STATE_RIGHT, 0U);                EXPECT(0, 1);
    at(STATE_RIGHT, T);                 EXPECT(0, 0);
}

static void test_hazard_lights_both_sides_in_sync(void)
{
    at(STATE_HAZARD, 0U);               EXPECT(1, 1);
    at(STATE_HAZARD, T);                EXPECT(0, 0);
    at(STATE_HAZARD, 2U * T);           EXPECT(1, 1);
}

/* ================= State changes restart the phase ================= */
static void test_left_to_right_restarts_phase_on(void)
{
    at(STATE_LEFT, 0U);
    at(STATE_LEFT, 400U);               /* left is in its off phase */
    at(STATE_RIGHT, 400U);              EXPECT(0, 1);   /* right on at once */
    at(STATE_RIGHT, 400U + T - 1U);     EXPECT(0, 1);
    at(STATE_RIGHT, 400U + T);          EXPECT(0, 0);
}

static void test_left_to_off_turns_off_immediately(void)
{
    at(STATE_LEFT, 0U);
    at(STATE_OFF, 100U);                EXPECT(0, 0);
}

/* Press LEFT during hazard: state stays HAZARD, so the rhythm must not jump. */
static void test_virtual_stalk_press_during_hazard_keeps_rhythm(void)
{
    turn_signal_t ts;
    turn_signal_init(&ts);

    turn_signal_step(&ts, EVENT_HAZARD);
    at(ts.state, 0U);                   EXPECT(1, 1);

    turn_signal_step(&ts, EVENT_LEFT);  /* only resume_state moves */
    at(ts.state, 200U);                 EXPECT(1, 1);
    at(ts.state, T);                    EXPECT(0, 0);   /* not restarted at 200 */
}

/* ================= Failure cases ================= */
/* The hand-traced table: main stalled 1000 ms (breakpoint, long UART print). */
static void test_catches_up_with_grid_after_stall(void)
{
    at(STATE_LEFT, 0U);                 EXPECT(1, 0);
    at(STATE_LEFT, 1000U);              /* flip, last = 333 */
    at(STATE_LEFT, 1000U);              /* flip, last = 666 */
    at(STATE_LEFT, 1000U);              /* flip, last = 999 */
    TEST_ASSERT_EQUAL_UINT32(999U, b.last_ms);
    at(STATE_LEFT, 1001U);              EXPECT(0, 0);   /* 999..1331 is off */
    at(STATE_LEFT, 3U * T + T);         EXPECT(1, 0);   /* on again at 1332 */
}

static void test_tick_wraparound(void)
{
    const uint32_t start = 0xFFFFFF00U;  /* 256 ms before the 32-bit wrap */

    at(STATE_LEFT, start);              EXPECT(1, 0);
    at(STATE_LEFT, start + T - 1U);     EXPECT(1, 0);   /* already wrapped */
    at(STATE_LEFT, start + T);          EXPECT(0, 0);
}

static void test_corrupted_state_gives_all_off(void)
{
    at(STATE_HAZARD, 0U);
    at((state_t)STATE_COUNT, 10U);      EXPECT(0, 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_off_keeps_all_lamps_off);
    RUN_TEST(test_left_lights_immediately_when_selected);
    RUN_TEST(test_left_stays_on_until_half_period);
    RUN_TEST(test_left_goes_off_at_half_period);
    RUN_TEST(test_left_on_again_after_full_period);
    RUN_TEST(test_right_lights_only_right_side);
    RUN_TEST(test_hazard_lights_both_sides_in_sync);
    RUN_TEST(test_left_to_right_restarts_phase_on);
    RUN_TEST(test_left_to_off_turns_off_immediately);
    RUN_TEST(test_virtual_stalk_press_during_hazard_keeps_rhythm);
    RUN_TEST(test_catches_up_with_grid_after_stall);
    RUN_TEST(test_tick_wraparound);
    RUN_TEST(test_corrupted_state_gives_all_off);
    return UNITY_END();
}
