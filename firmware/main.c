#include "gpio.h"
#include "lamps.h"
#include "systick.h"
#include "buttons.h"
#include "uart.h"
#include "turn_signal.h"
#include "blink.h"
#include "press_filter.h"

extern uint32_t SystemCoreClock;
extern void SystemCoreClockUpdate();

/*
 * Phase 4: turn signal + hazard on the board.
 * Every decision lives in app/ (unit-tested on the PC). main only moves values,
 * once per loop pass:
 *   g_ticks -> buttons -> press_filter -> turn_signal -> blink -> lamps,
 *   and one UART line each time the state changes.
 */

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
	logged_state = ts.state;
	log_state(logged_state);

	while(1) {
		uint32_t now = g_ticks;         /* one time value for the whole pass */
		press_filter_in_t in;
		event_t ev[EVENT_COUNT];
		uint8_t n;
		uint8_t i;
		blink_out_t out;

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

		/* 5. Log only on a change: one short line, not one per pass. */
		if (ts.state != logged_state) {
			logged_state = ts.state;
			log_state(logged_state);
		}
	}
}
