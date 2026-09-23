# Requirements — Turn Signal & Hazard Controller (Project 1)

## Scope

Turn signal and hazard indicator controller, built first on the TM4C123 LaunchPad
in bare-metal C (register-level, no HAL/CMSIS drivers). A port to the STM32 Blue
Pill happens later, in Phase 7.

## Functional Requirements

| ID   | Requirement                                                                          |
|------|---------------------------------------------------------------------------------------|
| R1   | Pressing LEFT shall flash both left lamps at 1.5 Hz ±10%, 50% duty.                  |
| R2   | Pressing RIGHT shall do the same for the right lamps.                                |
| R3   | Pressing an active LEFT/RIGHT again shall turn it off.                               |
| R4   | Pressing the opposite side shall switch immediately, starting with lamps ON.         |
| R5   | HAZARD shall flash all four lamps in sync and override LEFT/RIGHT; pressing it again returns to OFF. |
| R6   | Left and right lamps shall never flash independently at the same time (only in hazard). |
| R7   | Presses shorter than ~20 ms shall be ignored (debounce).                             |
| R8   | The first lamp-on shall follow a press within 100 ms.                                |
| R9   | After reset, all lamps shall be off.                                                 |
| R10 *(stretch)* | A tap while OFF flashes exactly 3 times (lane change).                    |
| R11 *(stretch)* | The buzzer shall tick with each flash.                                    |

## I/O List

**Inputs**
- LEFT button — digital input, pull-up, active-low
- RIGHT button — digital input, pull-up, active-low
- HAZARD button — digital input, pull-up, active-low

**Outputs**
- Front-left LED
- Rear-left LED
- Front-right LED
- Rear-right LED
- Buzzer *(stretch, supports R11)*

*(Exact GPIO pin assignments and the wiring schematic are a Phase 2 deliverable,
not Phase 1 — this list only fixes what signals exist, not where they connect.)*

## Open Questions & Decisions

| # | Question | Decision |
|---|----------|----------|
| 1 | What happens if LEFT is active and the driver presses RIGHT? | Switch immediately, new side starts ON (R4). |
| 2 | What happens if HAZARD is pressed during an active turn signal? | Hazard overrides the turn signal (R5). |
| 3 | What happens if two buttons are pressed at the same time? | Priority order: HAZARD wins. |
| 4 | What state should the system be in after reset? | OFF — all lamps off (R9). |
| 5 | The buttons are momentary, not a latching stalk — what does a press mean? | Each press toggles the corresponding state. |
| 6 | How fast should the lamps flash? | 1.5 Hz (333 ms on / 333 ms off), chosen to match real turn-signal rates (~60–120 flashes/min). |
| 7 | Which board is implemented first? | TM4C123 LaunchPad; the Blue Pill port is deferred to Phase 7. |
| 8 | What counts as a valid button press vs. contact bounce? | Ignore any press shorter than ~20 ms (R7). |
