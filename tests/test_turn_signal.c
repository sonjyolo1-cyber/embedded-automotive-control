/*
 * test_turn_signal.c - one test per cell of the transition table,
 * plus the out-of-range guards. Runs on the PC.
 */
#include "unity.h"
#include "turn_signal.h"

void setUp(void) {}
void tearDown(void) {}

/* ---- Row OFF ---- */
static void test_off_press_left_turns_left_on(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, state_machine_step(STATE_OFF, EVENT_LEFT));
}

static void test_off_press_right_turns_right_on(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, state_machine_step(STATE_OFF, EVENT_RIGHT));
}

/* ---- Row LEFT ---- */
static void test_left_press_left_again_turns_off(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_OFF, state_machine_step(STATE_LEFT, EVENT_LEFT));
}

static void test_left_press_right_switches_to_right(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, state_machine_step(STATE_LEFT, EVENT_RIGHT));
}

/* ---- Row RIGHT ---- */
static void test_right_press_left_switches_to_left(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, state_machine_step(STATE_RIGHT, EVENT_LEFT));
}

static void test_right_press_right_again_turns_off(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_OFF, state_machine_step(STATE_RIGHT, EVENT_RIGHT));
}

/* ---- Guards: never read outside the table ---- */
static void test_unknown_event_keeps_state(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT,
        state_machine_step(STATE_RIGHT, (event_t)EVENT_COUNT));
}

static void test_unknown_state_is_returned_unchanged(void)
{
    TEST_ASSERT_EQUAL_INT(STATE_COUNT,
        state_machine_step((state_t)STATE_COUNT, EVENT_LEFT));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_off_press_left_turns_left_on);
    RUN_TEST(test_off_press_right_turns_right_on);
    RUN_TEST(test_left_press_left_again_turns_off);
    RUN_TEST(test_left_press_right_switches_to_right);
    RUN_TEST(test_right_press_left_switches_to_left);
    RUN_TEST(test_right_press_right_again_turns_off);
    RUN_TEST(test_unknown_event_keeps_state);
    RUN_TEST(test_unknown_state_is_returned_unchanged);
    return UNITY_END();
}
