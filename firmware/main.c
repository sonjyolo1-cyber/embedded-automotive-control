#include "lamps.h"
#include "systick.h"
#include "buttons.h"
#include "uart.h"
#include "watchdog.h"
#include "turn_signal.h"
#include "blink.h"
#include "press_filter.h"
#include "buzzer.h"

extern uint32_t SystemCoreClock;
extern void SystemCoreClockUpdate(void);

/*
 * Turn signal & hazard controller, EK-TM4C123GXL.
 * Every decision lives in app/ (unit-tested on the PC). main only moves values,
 * once per loop pass:
 *   g_ticks -> buttons -> press_filter -> turn_signal -> blink -> lamps + buzzer
 *           -> one UART line if the state changed -> feed the watchdog
 */

#define UART_BAUD        115200U
#define WDT_TIMEOUT_MS   500U   /* longest normal loop pass is ~1 ms */

static const char *const s_state_name[STATE_COUNT] = { "OFF", "LEFT", "RIGHT", "HAZARD" };

/* Print "state=<name>\r\n". Longest line 14 chars: fits the 16-byte TX FIFO. */
static void log_state(state_t s)
{
	uart_puts("state=");
	uart_puts((s < STATE_COUNT) ? s_state_name[s] : "?");
	uart_puts("\r\n");
}

/* Print "reset: watchdog" / "reset: pin" / ... once at start-up. */
static void log_reset_cause(uint32_t cause)
{
	uart_puts("reset:");
	if ((cause & RESET_CAUSE_WDT0)      != 0U) { uart_puts(" watchdog");  }
	if ((cause & RESET_CAUSE_POWER_ON)  != 0U) { uart_puts(" power-on");  }
	if ((cause & RESET_CAUSE_PIN)       != 0U) { uart_puts(" pin");       }
	if ((cause & RESET_CAUSE_BROWN_OUT) != 0U) { uart_puts(" brown-out"); }
	if ((cause & RESET_CAUSE_SOFTWARE)  != 0U) { uart_puts(" software");  }
	uart_puts("\r\n");
}

int main(void)
{
	turn_signal_t ts;
	blink_t blink;
	buzzer_t bz;
	state_t logged_state;
	uint32_t reset_cause;

	/* Read the reset cause first, before any init code. */
	reset_cause = watchdog_take_reset_cause();

	SystemCoreClockUpdate();
	uart_init(SystemCoreClock, UART_BAUD);
	lamp_init();
	buttons_init();
	systick_init(SystemCoreClock);

	uart_puts("\r\n--- Turn signal & hazard controller ---\r\n");
	uart_puts("SystemCoreClock = ");
	uart_put_uint(SystemCoreClock);
	uart_puts(" Hz\r\n");
	log_reset_cause(reset_cause);

	turn_signal_init(&ts);
	blink_init(&blink, g_ticks);
	buzzer_init(&bz, g_ticks);
	logged_state = ts.state;
	log_state(logged_state);

	/* Start the watchdog last, after all init and the banner. */
	watchdog_init(SystemCoreClock, WDT_TIMEOUT_MS);

	while (1) {
		uint32_t now = g_ticks;         /* one time value for the whole pass */
		press_filter_in_t in;
		event_t ev[EVENT_COUNT];
		uint8_t n;
		uint8_t i;
		blink_out_t out;

		/* 1. Debounce. Must run in the same pass as the reads below, so a
		 *    press always comes with held = 1. */
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

		/* 5. Buzzer follows the lamp output: short pulse at every lamp edge (R11). */
		lamp_set(LAMP_BUZZER, buzzer_update(&bz, (uint8_t)(out.left || out.right), now));

		/* 6. Log only on a change: one short line, not one per pass. */
		if (ts.state != logged_state) {
			logged_state = ts.state;
			log_state(logged_state);
		}

		/* 7. Last line of the pass: only reached if every step above ran. */
		watchdog_feed();
	}
}