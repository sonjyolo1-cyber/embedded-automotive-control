# press_filter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> **This project:** the owner writes the code to learn. Claude reviews each step and does not implement unless asked.

**Goal:** A pure C function that turns one loop pass of button presses into the events for `turn_signal_step`, dropping a LEFT/RIGHT press while the other side is held.

**Architecture:** New module `app/press_filter.{h,c}`, no hardware, next to `turn_signal` and `blink`. `main.c` (block B) fills a struct from `button_was_pressed` / `button_is_pressed`, calls the filter, and forwards each returned event to `turn_signal_step`. Every decision is in tested code; `main` only copies values.

**Tech Stack:** C99, gcc on the PC, Unity (`tests/unity`), Keil for the TM4C firmware.

**Spec:** `docs/design.md`, section 5, "LEFT + RIGHT conflict"

## Global Constraints

- Pure C in `app/`: do NOT include any register or driver header (`gpio.h`, `buttons.h`, `uart.h`, ...). Builds on the PC and the TM4C unchanged.
- Builds clean with `gcc -std=c99 -Wall -Wextra` (no warnings).
- Reuse `event_t` / `EVENT_COUNT` from `turn_signal.h`; do not define new event values.
- Flags are `uint8_t`, 1 = yes, 0 = no (same as `blink_out_t`).
- The 17 tests in `test_turn_signal` and the 13 tests in `test_blink` still pass.

## Review Focus

1. **Both edges in the same pass.** At the pass where a press is detected, the driver also reports that button as held (`stable` becomes 1 in the same `buttons_update`). So when both are pressed, both are also held, and rule 1 drops both: no extra rule is needed. Pinned by `test_left_and_right_same_pass_gives_nothing`.
2. **Hand on all three buttons.** HAZARD must still pass when LEFT and RIGHT are both held. Pinned by `test_hazard_passes_while_left_and_right_held`.
3. **Press already released when read** (e.g. a long UART print stalls the loop): `left_pressed = 1`, `left_held = 0`. The press still counts; only the *other* side's held flag matters. Pinned by `test_left_press_counts_even_if_left_already_released`.
4. **Fixed output order** LEFT, RIGHT, HAZARD (enum order), so a run on the board equals a run in the test. Pinned by `test_hazard_and_left_gives_left_then_hazard`.
5. **Holding one side for a long time** blocks the other side for as long as it is held, then the other side works again. Pinned by `test_right_works_again_after_left_released`.

---

### Task 1: press_filter module

**Files:**
- Create: `app/press_filter.h`
- Create: `app/press_filter.c`
- Test: `tests/test_press_filter.c`
- Modify: `docs/design.md` (section 5 point 5: name the API; section 7: add the test list and build command)

**Interfaces:**
- Consumes: `event_t`, `EVENT_LEFT`, `EVENT_RIGHT`, `EVENT_HAZARD`, `EVENT_COUNT` from `app/turn_signal.h`
- Produces:

```c
typedef struct {
    uint8_t left_pressed;    /* button_was_pressed(BUTTON_LEFT)   this pass */
    uint8_t right_pressed;   /* button_was_pressed(BUTTON_RIGHT)  this pass */
    uint8_t hazard_pressed;  /* button_was_pressed(BUTTON_HAZARD) this pass */
    uint8_t left_held;       /* button_is_pressed(BUTTON_LEFT)  now         */
    uint8_t right_held;      /* button_is_pressed(BUTTON_RIGHT) now         */
} press_filter_in_t;

/* Writes the events to send, in order LEFT, RIGHT, HAZARD; returns how many (0..EVENT_COUNT). */
uint8_t press_filter_events(const press_filter_in_t *in, event_t out[EVENT_COUNT]);
```

`main.c` block B (later integration) uses it like this:

```c
event_t ev[EVENT_COUNT];
uint8_t n = press_filter_events(&in, ev);
for (uint8_t i = 0U; i < n; i++) { turn_signal_step(&ts, ev[i]); }
```

- [ ] **Step 1: Write the header `app/press_filter.h`**

Include guard, `#include <stdint.h>`, `#include "turn_signal.h"`, the struct and prototype above, and a header comment with the rule in the same style as `blink.h` (what it takes, what it returns, the rule from design.md).

- [ ] **Step 2: Write the failing tests `tests/test_press_filter.c`**

Same layout as `test_blink.c`: `setUp` clears the input, a helper runs the filter, a macro checks the result.

```c
static press_filter_in_t in;
static event_t ev[EVENT_COUNT];
static uint8_t n;

void setUp(void)    { in = (press_filter_in_t){0}; n = 0xFFU; }
void tearDown(void) {}

static void run(void) { n = press_filter_events(&in, ev); }
```

Tests and their assertions:

| Test | Input (all others 0) | Expect |
|---|---|---|
| `test_nothing_pressed_gives_nothing` | | `n == 0` |
| `test_left_alone_gives_left` | `left_pressed = 1, left_held = 1` | `n == 1`, `ev[0] == EVENT_LEFT` |
| `test_right_alone_gives_right` | `right_pressed = 1, right_held = 1` | `n == 1`, `ev[0] == EVENT_RIGHT` |
| `test_hazard_alone_gives_hazard` | `hazard_pressed = 1` | `n == 1`, `ev[0] == EVENT_HAZARD` |
| `test_left_dropped_while_right_held` | `left_pressed = 1, left_held = 1, right_held = 1` | `n == 0` |
| `test_right_dropped_while_left_held` | `right_pressed = 1, right_held = 1, left_held = 1` | `n == 0` |
| `test_left_and_right_same_pass_gives_nothing` | `left_pressed = 1, right_pressed = 1, left_held = 1, right_held = 1` | `n == 0` |
| `test_hazard_and_left_gives_left_then_hazard` | `left_pressed = 1, left_held = 1, hazard_pressed = 1` | `n == 2`, `ev[0] == EVENT_LEFT`, `ev[1] == EVENT_HAZARD` |
| `test_hazard_passes_while_left_and_right_held` | `hazard_pressed = 1, left_held = 1, right_held = 1` | `n == 1`, `ev[0] == EVENT_HAZARD` |
| `test_hazard_passes_when_left_right_conflict` | `left_pressed = 1, right_pressed = 1, left_held = 1, right_held = 1, hazard_pressed = 1` | `n == 1`, `ev[0] == EVENT_HAZARD` |
| `test_left_press_counts_even_if_left_already_released` | `left_pressed = 1` | `n == 1`, `ev[0] == EVENT_LEFT` |
| `test_right_works_again_after_left_released` | first `right_pressed = 1, right_held = 1, left_held = 1` → `n == 0`; then `left_held = 0`, `right_pressed = 1` → `n == 1`, `ev[0] == EVENT_RIGHT` |

Use `TEST_ASSERT_EQUAL_UINT8` for `n` and `TEST_ASSERT_EQUAL_INT` for events (as in `test_turn_signal.c`). `main()` with `UNITY_BEGIN()`, one `RUN_TEST` per test, `return UNITY_END();`.

- [ ] **Step 3: Run the tests to see them fail**

Create an empty `app/press_filter.c` (only `#include "press_filter.h"`), then:

```
gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_press_filter.c app/press_filter.c tests/unity/src/unity.c -o build/test_press_filter.exe
```

Expected: link error `undefined reference to 'press_filter_events'`. That is the "red".

- [ ] **Step 4: Implement `press_filter_events` in `app/press_filter.c`**

Check LEFT, then RIGHT, then HAZARD, appending to `out` and counting. A LEFT press is kept only if `right_held == 0`, a RIGHT press only if `left_held == 0`; HAZARD always. No state, no `static` variables.

- [ ] **Step 5: Run the tests to see them pass**

```
gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_press_filter.c app/press_filter.c tests/unity/src/unity.c -o build/test_press_filter.exe
build\test_press_filter.exe
```

Expected: `12 Tests 0 Failures 0 Ignored`, `OK`, and no gcc warnings.

- [ ] **Step 6: Re-run the existing suites**

```
gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_turn_signal.c app/turn_signal.c tests/unity/src/unity.c -o build/test_turn_signal.exe
build\test_turn_signal.exe
gcc -std=c99 -Wall -Wextra -I tests/unity/src -I app tests/test_blink.c app/blink.c app/turn_signal.c tests/unity/src/unity.c -o build/test_blink.exe
build\test_blink.exe
```

Expected: `17 Tests 0 Failures`, then `13 Tests 0 Failures`.

- [ ] **Step 7: Update `docs/design.md`**

Section 5 "LEFT + RIGHT conflict" point 5: name `press_filter_events` and `press_filter_in_t`. Section 7: add a line `Host (tests/test_press_filter.c, 12 tests, all pass): ...` and the build command from Step 5.

- [ ] **Step 8: Commit**

```
git add app/press_filter.h app/press_filter.c tests/test_press_filter.c docs/design.md docs/superpowers/plans/2026-10-18-press-filter.md
git commit -m "Phase 4: press_filter drops LEFT/RIGHT while the other is held, 12 unit tests pass"
```
