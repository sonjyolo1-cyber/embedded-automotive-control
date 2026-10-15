# Design: turn signal & hazard controller

Target: EK-TM4C123GXL LaunchPad (TM4C123GH6PM, Cortex-M4F), register-level C, no TivaWare.
Host tests: GCC 14.2 (MSYS2 UCRT64) + Unity.

Status: Phase 3 (bring-up) done. Phase 4 (application logic) in progress.

---

## 1. Hardware

### Pin map

| Signal | Pin | Direction | Notes |
|---|---|---|---|
| Front-left lamp | PE0 | output | LED + series resistor to GND, active-high |
| Rear-left lamp | PE1 | output | |
| Front-right lamp | PE2 | output | |
| Rear-right lamp | PE3 | output | |
| Buzzer (stretch goal) | PE4 | output | |
| LEFT button | PB0 | input | to GND, internal pull-up, active-low |
| RIGHT button | PB1 | input | to GND, internal pull-up, active-low |
| HAZARD button | PB2 | input | to GND, internal pull-up, active-low |
| UART0 RX / TX | PA0 / PA1 | alt. function 1 | LaunchPad virtual COM port (debug USB) |

Buttons are wired straight to GND (no external resistor). A loose resistor in this path caused
intermittent contact during bring-up (see devlog).

### Clock

`SystemCoreClock` = **50 MHz** (PLL configured by `SystemInit` in the startup code).
All timing derives from it at run time, nothing hard-codes 16 MHz.

---

## 2. Timing

| Item | Value | Why |
|---|---|---|
| SysTick tick | 1 ms | one shared time base for all tasks |
| Tick storage | `volatile uint32_t g_ticks` counter (not a flag) | a flag loses ticks if `main` is busy > 1 ms; a counter does not |
| Elapsed time | `(now - last) >= N` | correct across the 32-bit wrap (~49.7 days); `now >= last + N` is not |
| Debounce window | 20 ms | measured bounce max 4-5 ms (20 presses, 40/40 bursts): 4x margin, well under ~50 ms a user notices |
| Blink | 1.5 Hz, 333 ms half period | `last += 333` after each toggle, so lateness does not accumulate |

ISRs do one thing only (SysTick: `g_ticks++`). All work happens in the `main` super-loop.

---

## 3. Buttons (debounce)

A button's clean state changes only after its pin shows **no edge (rising or falling) for 20 ms**.
Every edge restarts the timer.

Per-button state: `raw_prev`, `last_edge_ms` (`uint32_t`, same type as `g_ticks`), `stable`, `pressed_event`.
Initial values come from a real pin read, so a button held during reset gives no event.

API: `buttons_update(now_ms)` every loop pass; `button_is_pressed()` = clean state;
`button_was_pressed()` = one event per press, cleared on read. Time is passed in so the driver can be
unit-tested on a PC.

---

## 4. UART log

UART0, 115200 8N1, TX FIFO on. 50 MHz: `IBRD = 27`, `FBRD = 8` (actual 115207 baud, 0.006 % error),
computed at run time from `SystemCoreClock` in integer math: `brd_x64 = (clock*4 + baud/2) / baud`.

TX by polling: each log line fits (or nearly fits) the 16-byte FIFO, so `main` barely waits.
Switch to interrupt + ring buffer if a single print grows beyond ~50 characters (~4 ms of waiting).

---

## 5. Turn signal state machine

Lives in `app/turn_signal.c`: pure C, no register headers, unit-tested on the PC.

### Decisions

1. **Toggle.** Pressing the button of the side that is already on turns it off.
   Momentary buttons have no centre position like a real stalk, and no extra OFF button is needed.
2. **Hazard has the highest priority.** While hazard is on, LEFT/RIGHT never change the lamps.
   A safety function must not be cancelled by an accidental press.
3. **Virtual stalk.** While hazard is on, LEFT/RIGHT still move a remembered stalk position
   (`resume_state`) through the same toggle table. Pressing HAZARD again returns to it,
   like a real car whose latched stalk takes over when hazard goes off.
   - Chosen over "always return to OFF" because it matches real driver expectations.
   - Chosen over separate `HAZARD_FROM_x` states because it scales: new modes add one state, not one per mode.
   - Toggle (not "set") for the virtual stalk: the driver can still cancel the remembered side during hazard,
     and a button means the same thing at all times.

### Stalk table (toggle)

| stalk \ press | LEFT | RIGHT |
|---|---|---|
| **OFF** | LEFT | RIGHT |
| **LEFT** | OFF | RIGHT |
| **RIGHT** | LEFT | OFF |

### Full behaviour

| In state | Event | New state | `resume_state` after |
|---|---|---|---|
| OFF / LEFT / RIGHT | LEFT or RIGHT | stalk table | unchanged |
| OFF | HAZARD | HAZARD | OFF |
| LEFT | HAZARD | HAZARD | LEFT |
| RIGHT | HAZARD | HAZARD | RIGHT |
| HAZARD | LEFT | HAZARD | stalk table(`resume_state`, LEFT) |
| HAZARD | RIGHT | HAZARD | stalk table(`resume_state`, RIGHT) |
| HAZARD | HAZARD | `resume_state` | unchanged |

### Implementation

- `typedef struct { state_t state; state_t resume_state; } turn_signal_t;`
- Stalk table as a `static const` 3 x 2 lookup array (lives in flash). HAZARD handled by `if` logic
  because it reads and writes `resume_state`.
- Guards: out-of-range event or corrupted state leaves the struct unchanged. C does not check array
  bounds; an unchecked index would silently read another cell or memory outside the table.
- `*_COUNT` enum members size the arrays. Enum order matters: `STATE_HAZARD` / `EVENT_HAZARD`
  follow the stalk states / events.

### Open points

- Two button events in the same loop pass: processing order changes the result for LEFT + RIGHT.
  `main.c` will process in a fixed order (decided at integration). HAZARD + LEFT gives the same result
  in either order (tested).
- "Tap = blink 3 times" (Phase 6) is a conditional transition and will not fit the lookup table alone.

---

## 5b. Blink logic

Lives in `app/blink.c`: pure C, unit-tested on the PC with fake ticks (each test passes `now_ms` by hand).

### Decisions

1. **Input = `state` + `now_ms`.** `main` reads `g_ticks` and passes the value in; the module never reads
   `g_ticks` itself, so it links and runs on the PC. It does not see buttons: `turn_signal` already turned
   presses into `state` (e.g. LEFT during hazard leaves `state` = HAZARD, so blink needs no extra rule).
2. **Output = on/off per side** (`blink_out_t { left, right }`), not one on/off value.
   Every decision (which side follows the phase) is inside tested code; `main` only writes the result.
3. **Memory:** `prev_state` (detect a state change, same idea as `raw_prev` in the button driver),
   `last_ms` (start of the current phase), `phase_on` (no register to toggle, so the module keeps the phase).
4. **State change restarts the phase ON**, counted from that moment. The lamp lights at once; a delay of up to
   333 ms could make the driver press again, which with toggle semantics would switch the signal off.
5. **Phase flips with `last_ms += 333`** (fixed grid, no drift, wrap-around safe), at most one flip per call.

| state | left | right |
|---|---|---|
| OFF | 0 | 0 |
| LEFT | `phase_on` | 0 |
| RIGHT | 0 | `phase_on` |
| HAZARD | `phase_on` | `phase_on` |

API: `blink_init(&b, now_ms)` (starts as if OFF, so the first press counts as a change);
`blink_out_t blink_update(&b, state, now_ms)` once per loop pass. Corrupted state -> `{0, 0}`.

### Risks

- After `main` stalls (breakpoint, long UART print), consecutive calls flip the phase several times within
  microseconds until `last_ms` catches up with the grid. Final phase is correct and invisible on LEDs, but a
  buzzer that clicks on every flip would click several times: revisit in Phase 5.
- If `main` forgets to call `blink_update` every pass, the lamps freeze: checked on the board.

---

## 6. Software structure

```
app/            pure C application logic, no hardware (turn_signal, blink)
firmware/       Keil project: startup, drivers (gpio, lamps, systick, buttons, uart), main.c
tests/          Unity tests run on the PC; tests/unity is a git submodule
docs/           design.md, devlog.md
build/          host test executables (git-ignored)
```

Data flow (planned for integration):

```
SysTick ISR -> g_ticks -> buttons_update() -> button_was_pressed() -> turn_signal_step()
                                                                      -> blink logic -> lamps
```

---

## 7. Tests

Host (`tests/test_turn_signal.c`, 17 tests, all pass):
stalk table (6), entering hazard (3), hazard priority (2), leaving hazard via virtual stalk (3),
HAZARD + LEFT in either order (1), input guards (2).

Host (`tests/test_blink.c`, 13 tests, all pass, fake ticks):
OFF all off, LEFT on at once / off at +333 / on at +666, RIGHT right side only, HAZARD both in sync,
LEFT -> RIGHT restarts the phase, LEFT -> OFF off at once, LEFT pressed during hazard keeps the rhythm,
catch-up after a 1000 ms stall, 32-bit tick wrap, corrupted state.

Build and run from the repo root:

```
gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_turn_signal.c app/turn_signal.c tests/unity/src/unity.c -o build/test_turn_signal.exe
build\test_turn_signal.exe

gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_blink.c app/blink.c app/turn_signal.c tests/unity/src/unity.c -o build/test_blink.exe
build\test_blink.exe
```

Hardware checks done in Phase 3: 20 presses -> 20 events, 3 s hold -> 1 event, held during reset -> 0 events,
UART one line per press.
