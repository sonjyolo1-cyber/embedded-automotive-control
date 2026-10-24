# Test plan: requirements traceability

Verified on the board on 2026-10-21. Unit tests are Unity tests in `tests/`, run with GCC on the PC
(build commands in `design.md`, section 7). Evidence files are in `docs/evidence/`.
Video = `evidence/evidence_R1-R6_R13.mp4` (29.97 fps).

| ID | Requirement | Group | Unit test (PC) | Board check | Result | Evidence |
|----|-------------|-------|----------------|-------------|--------|----------|
| R1 | Pressing LEFT shall flash both left lamps at 1.5 Hz ±10%, 50% duty. | C | `test_left_goes_off_at_half_period`, `test_left_on_again_after_full_period` | Count video frames (29.97 fps) | PASS: 12 cycles = 8.01 s -> 667 ms (1.50 Hz), on 334 ms (duty 50%) | `evidence/R1_R2_R5_led_timeline.png`, video 6.7-15 s |
| R2 | Pressing RIGHT shall do the same for the right lamps. | C | `test_off_press_right_turns_right_on`, `test_right_lights_only_right_side` | Count video frames | PASS: 13 cycles = 8.67 s -> 667 ms | video 15-24 s |
| R3 | Pressing an active LEFT/RIGHT again shall turn it off. | C | `test_left_press_left_again_turns_off`, `test_right_press_right_again_turns_off` | Video | PASS | video 35-38 s |
| R4 | Pressing the opposite side shall switch immediately, starting with lamps ON, unless the active side's button is still held (R12). | C | `test_left_press_right_switches_to_right`, `test_left_to_right_restarts_phase_on`, `test_right_dropped_while_left_held` | Video | PASS | video 15.2 s and 35.2 s |
| R5 | HAZARD shall flash all four lamps in sync and override LEFT/RIGHT; pressing HAZARD again returns to the virtual stalk position (R13). | C | `test_left_press_hazard_overrides_and_remembers_left`, `test_hazard_press_left_keeps_hazard`, `test_hazard_lights_both_sides_in_sync`, `test_left_hazard_hazard_resumes_left` | Video | PASS: both sides light in the same frame | video 24-31 s |
| R6 | Left and right lamps shall never flash independently at the same time (only in hazard). | A + design | `test_hazard_lights_both_sides_in_sync`, `test_right_lights_only_right_side`; design: one shared `phase_on` for both sides | LED timeline | PASS | `evidence/R1_R2_R5_led_timeline.png` |
| R7 | Presses shorter than ~20 ms shall be ignored (debounce); each press gives exactly one event, also when held. | B | none yet (needs `test_buttons.c`) | UART log | PASS for "1 press = 1 event" (20/20) and "3 s hold = 1 event". "Press < 20 ms ignored" NOT checked: cannot be made reliably by hand | `evidence/R7_R12_uart_log.txt` |
| R8 | The first lamp-on shall follow a press within 100 ms. | C | `test_left_lights_immediately_when_selected` | Temporary latency code (`MEASURE_LATENCY`) + UART | PASS: dt = 20 ms on 20/20 presses (1 ms resolution) | `evidence/R8_latency_log.txt` |
| R9 | After reset, all lamps shall be off, also when a button is held during reset. | B | none | Reset with LEFT held + UART | PASS | `evidence/R9_reset_held.txt` |
| R10 | *(optional, after v1.1)* A tap while OFF flashes exactly 3 times (lane change). | - | - | - | Not tested (optional, after v1.1) | - |
| R11 | *(stretch, Phase 6)* The buzzer shall tick with each flash. | C | the 11 tests in `tests/test_buzzer.c`, e.g. `test_left_for_1400ms_clicks_on_every_lamp_edge`, `test_off_state_is_silent_with_real_blink`, `test_hazard_clicks_once_per_edge_not_twice` | By ear, 2026-10-24: OFF silent, LEFT and HAZARD click on every lamp edge, no double click in HAZARD | PASS (by ear, no recording) | - |
| R12 | A LEFT/RIGHT press shall be ignored while the other side's button is held; LEFT and RIGHT in the same pass are both ignored; HAZARD is always accepted. | C | the 15 tests in `tests/test_press_filter.c` | UART log | PASS (a, b, c); case "RIGHT touches first" not run yet | `evidence/R7_R12_uart_log.txt` |
| R13 | During HAZARD, LEFT/RIGHT shall move a virtual stalk with the same toggle rules, without changing the lamps. | C | `test_right_pressed_during_hazard_resumes_right`, `test_left_pressed_again_during_hazard_resumes_off`, `test_virtual_stalk_press_during_hazard_keeps_rhythm` | Video | PASS, 3 cases (RIGHT->OFF, OFF->RIGHT, RIGHT->LEFT) | video 24-31 s, 38-46 s |

## Open items

- **R7:** `test_buttons.c` with fake ticks, to check that a press shorter than 20 ms is ignored.
- **R12:** board case "hand across both, RIGHT touches first" (expected `state=RIGHT`).
