/*
 * test_smoke.c - checks that GCC + Unity work on the PC.
 * Phase 4, day 1. Runs on the PC, NOT on the board: no registers here.
 */
#include <stdint.h>
#include "unity.h"

/* Unity calls these before/after every test. Empty for now. */
void setUp(void) {}
void tearDown(void) {}

/* 1. The simplest possible test: proves the framework runs. */
static void test_unity_works(void)
{
    TEST_ASSERT_EQUAL_UINT32(2U, 1U + 1U);
}

/* 2. The tick wrap-around rule from the PF1 lesson:
 *    (now - last) gives the real elapsed time even after g_ticks wraps.
 *    last = 4294967000, now = 200 (wrapped)  ->  496 ms really passed. */
static void test_tick_elapsed_survives_wraparound(void)
{
    uint32_t last = 4294967000U;
    uint32_t now  = 200U;

    TEST_ASSERT_EQUAL_UINT32(496U, now - last);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unity_works);
    RUN_TEST(test_tick_elapsed_survives_wraparound);
    return UNITY_END();
}
