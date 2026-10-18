# Development log

Turn signal & hazard controller on the EK-TM4C123GXL (TM4C123GH6PM), register level, no TivaWare.
Newest entries at the bottom. Entries before 2026-10-12 were written after the fact, from session notes,
because the code was not yet under version control.

---

## Phase 3: peripheral bring-up (2026-10-04 to 2026-10-11)

### 2026-10-04: SysTick timing fundamentals
- Practice: blink PF1 at 500 ms from a 1 ms SysTick tick.
- Design choices: tick-based timing (one 1 ms "system clock" shared by all tasks) instead of one timer per task;
  SysTick instead of a GPTM (3 registers, portable across Cortex-M); ISR only increments a counter, work happens in `main`.
- Learned: shared ISR/main variables must be `volatile`; elapsed time must be computed as `(now - last) >= N`,
  which stays correct when the 32-bit tick wraps (~49.7 days), while `now >= last + N` does not.
- Bug: LED glowed dimly instead of blinking. Cause: `last` was never updated, so the condition stayed true and the
  LED toggled on every loop pass. Fix: `last += PERIOD` after each toggle (also prevents drift).

### Step 3.2: SysTick 1 ms tick, 1.5 Hz blink
- Left lamps blink at 1.5 Hz (333 ms half period) on Port E.
- Bug: data bit for PE0 was set correctly in the debugger but the LED stayed off.
  Cause: breadboard GND wire in the wrong hole. Lesson: when the register is right, the fault is outside the chip;
  measure the pin with a multimeter before touching code.

### 2026-10-06, step 3.3a: button input through a driver
- `buttons.c/.h`: PB0-PB2 inputs, internal pull-ups, active-low.
- Hardware abstraction: the app calls `button_is_pressed()` and never sees electrical levels;
  only `BUTTON_ACTIVE_LEVEL` in `buttons.c` knows the button pulls to GND.
- `gpio_enable_clock()` now waits on PRGPIO before touching a port.

### 2026-10-07, step 3.3b: measuring contact bounce
- Counted level changes on PB0 with edge detection (`prev` initialised from a real read, not a constant).
- 10 presses gave **36 / 28 / 40** edges (ideal: 20). Contact bounce confirmed.

### 2026-10-08 to 10-09, step 3.4: debounce
- Rule: a button's clean state changes only after **no edge (rising or falling) for 20 ms**. Every edge restarts the timer.
- SysTick changed from a flag to a `volatile uint32_t g_ticks` counter, so no tick is lost if `main` is busy.
- `buttons_update(now_ms)` takes time as a parameter so the driver can be unit-tested on a PC.
- `button_was_pressed()` returns one event per press (released -> pressed on the clean state), cleared on read.
- Simulated on PC first: 7 ms and 18 ms bounce, 3 s hold, held during reset, tick wrap-around, 5 ms glitch: all pass.
- Bug 1: events kept counting while a button was held, LEDs flickered dimly.
  Cause: loose contact in the button path (resistor legs not seated). Found by bypassing the button and touching the
  PB0 wire directly to GND (stable). Fix: wire the button straight to GND, press it fully into the breadboard.
- Bug 2: occasionally one press gave two events.
  Measured bounce before changing anything (temporary code: span from first to last edge, 200 ms quiet window).
  Result: **max bounce 4-5 ms, 40/40 bursts**, so the pin was clean and the debounce window was not the problem.
  Cause: `last_edge_ms` had been retyped as `uint8_t`, which truncated the time and disabled the debounce.
  Fix: `uint32_t`. After the fix: **20 presses -> 20 events**.
- Decision: keep `DEBOUNCE_MS = 20` (4x margin over measured bounce, well under the ~50 ms a user notices).

### 2026-10-10 to 10-11, step 3.5: UART0 logging
- UART0 on PA0/PA1 through the LaunchPad virtual COM port, 115200 8N1, TX FIFO enabled.
- System clock is 50 MHz (`SystemCoreClock`). Divisor computed in integer math:
  `IBRD = 27`, `FBRD = 8`, actual baud 115207 (0.006 % error).
- TX by polling: each log line (~14-22 chars) fits or nearly fits the 16-byte FIFO, so `main` barely waits.
  Switch to interrupt + ring buffer if a single print grows beyond ~50 chars.
- Verified in PuTTY: startup banner, exactly one line per press, one line for a 3 s hold.

**Phase 3 complete.**

---

## Phase 4: application logic (from 2026-10-12)

### 2026-10-12: repo and host test setup
- Repo moved to `F:\Code\embedded-automotive-control` (off OneDrive). Keil project under `firmware/`.
- Unity added as a git submodule under `tests/unity`. Host compiler: GCC 14.2.0 (MSYS2 UCRT64).
- First host test `tests/test_smoke.c`: 2 tests, 0 failures (includes the tick wrap-around rule).
- Clone with `git clone --recursive`, or run `git submodule update --init` after a plain clone.

### 2026-10-13 to 10-14: turn signal state machine with hazard
- `app/turn_signal.c`: pure C, stalk toggle table (3 x 2 lookup) + if-logic for HAZARD.
- Decisions: toggle semantics (press the active side again = off); HAZARD has the highest priority;
  LEFT/RIGHT during hazard move a virtual stalk (`resume_state`) through the same table, HAZARD again returns to it.
- Guards for out-of-range event / corrupted state (C does not check array bounds).
- `tests/test_turn_signal.c`: 17 tests, 0 failures.
- Why host tests instead of Keil: pure logic needs no chip; runs in under a second, no flashing, no board.

### 2026-10-16: blink logic (fake ticks)
- `app/blink.c`: input `state` + `now_ms`, output on/off per side; `main` only writes the result to the lamps.
- Remembers `prev_state` (detect change, like `raw_prev`), `last_ms`, `phase_on`.
- State change restarts the phase ON: the lamp lights at once, so the driver does not press again
  (which, with toggle semantics, would switch the signal off).
- Hand-traced a 1000 ms stall: `last += 333` flips the phase three times in a row until it catches up with the grid;
  final phase correct, harmless for LEDs, noted as a risk for the buzzer.
- `tests/test_blink.c`: 13 tests, 0 failures. Checked that the tests catch a broken version
  (`last = now` instead of `last += 333`; no restart on state change).

### 2026-10-18: press filter (LEFT + RIGHT conflict), test first
- `app/press_filter.c`: pure function, takes "pressed this pass" + "held now", returns the events for
  `turn_signal_step` in the order LEFT, RIGHT, HAZARD. No state.
- Rules: the later button loses while the other is held; both edges in the same pass drop both;
  HAZARD always passes. Rule 2 coded as its own check, so it does not rely on the driver setting "held"
  in the same pass (it does today, but would not if `buttons_update` moved into the SysTick ISR).
- TDD: tests written first; red with a `return 0` stub (4 pass, 8 fail), then also checked against a
  version that forgets the conflict rule (5 fail), so every test has been red at least once.
  Rule 2 and the NULL guard were each added red first (wrong count, segfault), then green.
- `n = 0xFF` in `setUp`: a value the filter never returns, so a test that forgets to call it fails.
- Corrected a wrong assumption from the plan: "pressed but already released" cannot happen with the
  current driver (update and reads in the same pass). Test kept as a guard, comment fixed.
- `tests/test_press_filter.c`: 15 tests, 0 failures. All host suites pass: smoke 2, turn_signal 17,
  blink 13, press_filter 15 (`-Wall -Wextra -pedantic`, no warnings).
- Keil project uses ARM Compiler 6 (armclang 6.24) in C99 mode (`buttons.c` already relies on it with
  `for (uint8_t i ...)`). `press_filter.c` still declares its locals at the top so it also builds as C90
  (checked with `gcc -std=c90 -pedantic`).
