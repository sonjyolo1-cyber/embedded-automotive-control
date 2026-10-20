# Test plan: requirements traceability

Each requirement from `requirements.md` is checked in two ways where possible:

- **Unit test (PC):** Unity tests in `tests/`, run with GCC on the PC (build commands in `design.md`, section 7).
- **Board check:** manual scenario on the LaunchPad. KB n = scenario n in the 13-scenario checklist (Notion).

| ID | Unit test (PC) | Board check | Notes |
|----|----------------|-------------|-------|
| R1 | `test_off_press_left_turns_left_on`, `test_left_goes_off_at_half_period`, `test_left_on_again_after_full_period` | KB 2 + time 10 cycles | 1.5 Hz ±10% (1.35 to 1.65 Hz) means 10 cycles in 6.1 to 7.4 s. |
| R2 | `test_off_press_right_turns_right_on`, `test_right_lights_only_right_side` | KB 2 for the right side + time 10 cycles | |
| R3 | `test_left_press_left_again_turns_off`, `test_right_press_right_again_turns_off` | KB 4 | |
| R4 | `test_left_press_right_switches_to_right`, `test_left_to_right_restarts_phase_on`, `test_right_dropped_while_left_held` | KB 5, 11 | The "unless still held" part is R12. |
| R5 | `test_left_press_hazard_overrides_and_remembers_left`, `test_hazard_press_left_keeps_hazard`, `test_hazard_lights_both_sides_in_sync`, `test_left_hazard_hazard_resumes_left` | KB 6, 8 | |
| R6 | `test_hazard_lights_both_sides_in_sync`, `test_right_lights_only_right_side` | KB 6 (both sides in the same rhythm) | By design: one shared `phase_on` for both sides, so they cannot drift apart. |
| R7 | none yet (`test_buttons.c` later) | KB 13: 20 presses give 20 events; 3 s hold gives 1 event | A press shorter than 20 ms cannot be made reliably by hand, so "short press ignored" is not really checked yet. Needs `test_buttons.c` with fake ticks. |
| R8 | `test_left_lights_immediately_when_selected` (logic part only) | Temporary measuring code, 20 presses, pass if max dt < 100 ms (expected about 25 ms) | See "R8 measurement" below. |
| R9 | none | KB 1, 12 | Needs the real reset and pin reads. |
| R10 | none | none | Optional, after v1.1. Not implemented. |
| R11 | none | none | Stretch, Phase 6. Not implemented. |
| R12 | the 15 tests in `tests/test_press_filter.c`, e.g. `test_left_dropped_while_right_held`, `test_left_and_right_same_pass_none_held_gives_nothing` | KB 10, 11 | |
| R13 | `test_right_pressed_during_hazard_resumes_right`, `test_left_pressed_again_during_hazard_resumes_off`, `test_virtual_stalk_press_during_hazard_keeps_rhythm` | KB 7, 9 | |

## R8 measurement

Temporary code (method X), to be written on 2026-10-22 together with the R7 to R11 checks:

1. Record `t1` at a falling edge (press, active-low) that comes after at least 20 ms without any edge.
2. Record `t2` when the state changes.
3. Print `dt = t2 - t1` over UART.

Run: 20 presses, take the largest `dt`. **Pass:** max `dt` < 100 ms. Expected: about 25 ms (20 ms debounce plus a few ms of bounce).

Limits:
- Resolution is 1 ms (`g_ticks`).
- The software measures itself: time spent before the edge is seen (e.g. in an ISR) is not included.
  An oscilloscope on PB0 and PE0 would be the independent check.
