/*
 * test_turn_signal.c - every row of the behaviour table in design.md,
 * the hazard scenarios, and the input guards. Runs on the PC.
 */
#include "unity.h"
#include "turn_signal.h"

static turn_signal_t ts;

void setUp(void)    { turn_signal_init(&ts); }   /* every test starts OFF/OFF */
void tearDown(void) {}

/* Helper: apply a sequence of presses. */
static void press(event_t e) { turn_signal_step(&ts, e); }

/* ================= Stalk table (no hazard) ================= */
static void test_off_press_left_turns_left_on(void)
{
    press(EVENT_LEFT);
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, ts.state);
}

static void test_off_press_right_turns_right_on(void)
{
    press(EVENT_RIGHT);
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, ts.state);
}

static void test_left_press_left_again_turns_off(void)
{
    press(EVENT_LEFT); press(EVENT_LEFT);
    TEST_ASSERT_EQUAL_INT(STATE_OFF, ts.state);
}

static void test_left_press_right_switches_to_right(void)
{
    press(EVENT_LEFT); press(EVENT_RIGHT);
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, ts.state);
}

static void test_right_press_left_switches_to_left(void)
{
    press(EVENT_RIGHT); press(EVENT_LEFT);
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, ts.state);
}

static void test_right_press_right_again_turns_off(void)
{
    press(EVENT_RIGHT); press(EVENT_RIGHT);
    TEST_ASSERT_EQUAL_INT(STATE_OFF, ts.state);
}

/* ================= Entering hazard ================= */
static void test_off_press_hazard_turns_hazard_on_and_remembers_off(void)
{
    press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, ts.state);
    TEST_ASSERT_EQUAL_INT(STATE_OFF, ts.resume_state);
}

static void test_left_press_hazard_overrides_and_remembers_left(void)
{
    press(EVENT_LEFT); press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, ts.state);
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, ts.resume_state);
}

static void test_right_press_hazard_overrides_and_remembers_right(void)
{
    press(EVENT_RIGHT); press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, ts.state);
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, ts.resume_state);
}

/* ================= Hazard has priority ================= */
static void test_hazard_press_left_keeps_hazard(void)
{
    press(EVENT_HAZARD); press(EVENT_LEFT);
    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, ts.state);
}

static void test_hazard_press_right_keeps_hazard(void)
{
    press(EVENT_HAZARD); press(EVENT_RIGHT);
    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, ts.state);
}

/* ================= Leaving hazard: back to the virtual stalk ================= */
static void test_left_hazard_hazard_resumes_left(void)
{
    press(EVENT_LEFT); press(EVENT_HAZARD); press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, ts.state);
}

static void test_right_pressed_during_hazard_resumes_right(void)
{
    press(EVENT_LEFT); press(EVENT_HAZARD);
    press(EVENT_RIGHT);                     /* stalk moved to RIGHT */
    press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, ts.state);
}

static void test_left_pressed_again_during_hazard_resumes_off(void)
{
    press(EVENT_LEFT); press(EVENT_HAZARD);
    press(EVENT_LEFT);                      /* stalk toggled back to centre */
    press(EVENT_HAZARD);
    TEST_ASSERT_EQUAL_INT(STATE_OFF, ts.state);
}

/* ================= Same pass: HAZARD and LEFT, both orders ================= */
static void test_hazard_and_left_same_result_in_either_order(void)
{
    turn_signal_t a, b;

    turn_signal_init(&a);
    turn_signal_step(&a, EVENT_LEFT);
    turn_signal_step(&a, EVENT_HAZARD);

    turn_signal_init(&b);
    turn_signal_step(&b, EVENT_HAZARD);
    turn_signal_step(&b, EVENT_LEFT);

    TEST_ASSERT_EQUAL_INT(STATE_HAZARD, a.state);
    TEST_ASSERT_EQUAL_INT(a.state, b.state);
    TEST_ASSERT_EQUAL_INT(STATE_LEFT, a.resume_state);
    TEST_ASSERT_EQUAL_INT(a.resume_state, b.resume_state);
}

/* ================= Guards: never read outside the table ================= */
static void test_unknown_event_changes_nothing(void)
{
    press(EVENT_RIGHT);
    turn_signal_step(&ts, (event_t)EVENT_COUNT);
    TEST_ASSERT_EQUAL_INT(STATE_RIGHT, ts.state);
    TEST_ASSERT_EQUAL_INT(STATE_OFF, ts.resume_state);
}

static void test_corrupted_state_changes_nothing(void)
{
    ts.state = (state_t)STATE_COUNT;
    press(EVENT_LEFT);
    TEST_ASSERT_EQUAL_INT(STATE_COUNT, ts.state);
}

int main(void)
{
    UNITY_BEGIN();
    /* stalk table */
    RUN_TEST(test_off_press_left_turns_left_on);
    RUN_TEST(test_off_press_right_turns_right_on);
    RUN_TEST(test_left_press_left_again_turns_off);
    RUN_TEST(test_left_press_right_switches_to_right);
    RUN_TEST(test_right_press_left_switches_to_left);
    RUN_TEST(test_right_press_right_again_turns_off);
    /* entering hazard */
    RUN_TEST(test_off_press_hazard_turns_hazard_on_and_remembers_off);
    RUN_TEST(test_left_press_hazard_overrides_and_remembers_left);
    RUN_TEST(test_right_press_hazard_overrides_and_remembers_right);
    /* hazard priority */
    RUN_TEST(test_hazard_press_left_keeps_hazard);
    RUN_TEST(test_hazard_press_right_keeps_hazard);
    /* leaving hazard */
    RUN_TEST(test_left_hazard_hazard_resumes_left);
    RUN_TEST(test_right_pressed_during_hazard_resumes_right);
    RUN_TEST(test_left_pressed_again_during_hazard_resumes_off);
    /* same pass */
    RUN_TEST(test_hazard_and_left_same_result_in_either_order);
    /* guards */
    RUN_TEST(test_unknown_event_changes_nothing);
    RUN_TEST(test_corrupted_state_changes_nothing);
    return UNITY_END();
}