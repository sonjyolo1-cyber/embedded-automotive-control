#include "gpio.h"
#include "lamps.h"
#include "systick.h"
#include "buttons.h"
#include "uart.h"
#include "turn_signal.h"
#include "blink.h"
#include "press_filter.h"
#include "buzzer.h"

extern uint32_t SystemCoreClock;
extern void SystemCoreClockUpdate();

/*
 * Phase 4: turn signal + hazard on the board.
 * Every decision lives in app/ (unit-tested on the PC). main only moves values,
 * once per loop pass:
 *   g_ticks -> buttons -> press_filter -> turn_signal -> blink -> lamps -> buzzer,
 *   and one UART line each time the state changes.
 */

/* ------------------------------------------------------------------------
 * R8 latency measurement (Phase 5).
 * R8 measured 2026-10-21: dt = 20 ms on 20/20 presses. Kept for re-measurement on the Blue Pill port.
 * 1 = measure press-to-lamp latency on LEFT (PB0) and print it.
 * 0 = normal build: every line below between #if/#endif is not compiled.
 * ------------------------------------------------------------------------ */
#define MEASURE_LATENCY  0

#if MEASURE_LATENCY
#define LAT_PORTB_BASE   0x40005000UL  /* GPIO Port B (APB), same base as buttons.c */
#define LAT_LEFT_PIN     0U            /* PB0 = LEFT, active-low                   */
#define LAT_QUIET_MS     20U           /* = DEBOUNCE_MS: quiet this long = new press */

static uint8_t  lat_prev;              /* raw PB0 level on the previous pass        */
static uint32_t lat_last_edge;         /* time of the latest edge (any bounce)      */
static uint32_t lat_t1;                /* time of the FIRST falling edge of a press */
static uint8_t  lat_armed;             /* 1 = t1 captured, waiting for the state change */
static uint32_t lat_max;               /* largest dt seen since reset               */
#endif

static const char *const s_state_name[STATE_COUNT] = { "OFF", "LEFT", "RIGHT", "HAZARD" };

/* Print "state=<name>\r\n". Longest line 14 chars: fits the 16-byte TX FIFO. */
static void log_state(state_t s)
{
	uart_puts("state=");
	uart_puts((s < STATE_COUNT) ? s_state_name[s] : "?");
	uart_puts("\r\n");
}

int main(){
	turn_signal_t ts;
	blink_t blink;
	buzzer_t bz;
	state_t logged_state;

	SystemCoreClockUpdate();
	uart_init(SystemCoreClock, 115200U);
	lamp_init();
	buttons_init();
	systick_init(SystemCoreClock);

	uart_puts("\r\n--- Phase 4: turn signal + hazard ---\r\n");
	uart_puts("SystemCoreClock = ");
	uart_put_uint(SystemCoreClock);
	uart_puts(" Hz\r\n");

	turn_signal_init(&ts);
	blink_init(&blink, g_ticks);
	buzzer_init(&bz, g_ticks);
	logged_state = ts.state;
	log_state(logged_state);

#if MEASURE_LATENCY
	/* Start from a real read, like raw_prev in buttons.c (held at reset = no edge). */
	lat_prev      = gpio_read_pin(LAT_PORTB_BASE, LAT_LEFT_PIN);
	lat_last_edge = g_ticks;
	lat_armed     = 0U;
	lat_max       = 0U;
	uart_puts("R8: press LEFT only\r\n");
#endif

	while(1) {
		uint32_t now = g_ticks;         /* one time value for the whole pass */
		press_filter_in_t in;
		event_t ev[EVENT_COUNT];
		uint8_t n;
		uint8_t i;
		blink_out_t out;
#if MEASURE_LATENCY
		uint8_t lat_raw;

		/* 0. t1: first falling edge after LAT_QUIET_MS without any edge.
		 *    Later bounce edges only move lat_last_edge, never t1. */
		lat_raw = gpio_read_pin(LAT_PORTB_BASE, LAT_LEFT_PIN);
		if (lat_raw != lat_prev) {
			if ((lat_raw == 0U) && ((now - lat_last_edge) >= LAT_QUIET_MS)) {
				lat_t1    = now;
				lat_armed = 1U;
			}
			lat_last_edge = now;
			lat_prev      = lat_raw;
		}
#endif

		/* 1. Debounce. Must run in the same pass as the reads below, so a
		 *    press always comes with held = 1 (design.md 5, conflict point 6). */
		buttons_update(now);

		/* 2. Read all three every pass: button_was_pressed clears on read. */
		in.left_pressed   = button_was_pressed(BUTTON_LEFT);
		in.right_pressed  = button_was_pressed(BUTTON_RIGHT);
		in.hazard_pressed = button_was_pressed(BUTTON_HAZARD);
		in.left_held      = button_is_pressed(BUTTON_LEFT);
		in.right_held     = button_is_pressed(BUTTON_RIGHT);

		/* 3. Drop conflicting presses, then feed the state machine in order. */
		n = press_filter_events(&in, ev);
		for (i = 0U; i < n; i++) {
			turn_signal_step(&ts, ev[i]);
		}

		/* 4. Blink every pass, even with no press, or the lamps freeze. */
		out = blink_update(&blink, ts.state, now);
		lamp_set_left(out.left);
		lamp_set_right(out.right);

		/* 4b. Buzzer follows the LAMP output (not ts.state): silent in OFF,
		 *     a short pulse on PE4 at every lamp edge = "tick" / "tock" (R11). */
		lamp_set(LAMP_BUZZER, buzzer_update(&bz, (uint8_t)(out.left || out.right), now));

		/* 5. Log only on a change: one short line, not one per pass. */
		if (ts.state != logged_state) {
			logged_state = ts.state;
			log_state(logged_state);
#if MEASURE_LATENCY
			/* t2 = now: blink lit the lamp in step 4 of this same pass. */
			if (lat_armed != 0U) {
				uint32_t dt = now - lat_t1;
				lat_armed = 0U;
				if (dt > lat_max) {
					lat_max = dt;
				}
				uart_puts("dt=");
				uart_put_uint(dt);
				uart_puts(" max=");
				uart_put_uint(lat_max);
				uart_puts("\r\n");
			}
#endif
		}
	}
}