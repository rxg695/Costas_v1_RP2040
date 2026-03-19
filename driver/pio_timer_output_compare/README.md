# PIO Timer Output Compare Driver

This driver schedules a programmable output pulse after a trigger edge using a PIO state machine.

- Trigger event: rising edge on `trigger_pin`
- Compare value: host-provided countdown ticks
- Pulse width: host-provided high-time ticks
- Output action: set output high, hold for pulse ticks, then low

## Files

- `pio_timer_output_compare.h` — public C API
- `pio_timer_output_compare.c` — driver implementation
- `pio_timer_output_compare.pio` — PIO program
- `driver_manifest.cmake` — build manifest for driver aggregation

## API

### `pio_timer_output_compare_init(...)`
Initializes one PIO state machine instance for output compare.

- `output_pin` — runtime-configurable output pin controlled via `SET PINS`
- `sm_clk_hz` — state machine clock frequency

### `pio_timer_output_compare_arm(...)`
- `compare_ticks` is the number of countdown loop iterations before pulse start.
- `pulse_ticks` is the number of loop iterations to hold the output high.
- Two TX words arm one trigger->pulse cycle.

## Timing model

The compare delay loop is:

- `jmp x-- compare_count`

The pulse hold loop is:

- `jmp y-- pulse_count`

One decrement happens per loop iteration in each loop. Timing is approximately:

- delay ≈ `compare_ticks / sm_clk_hz` seconds
- pulse width ≈ `pulse_ticks / sm_clk_hz` seconds

(plus fixed program overhead around edge detect and pin update instructions).

## PIO behavior summary

1. Hold output low while idle.
2. Pull `compare_ticks` from TX FIFO.
3. Pull `pulse_ticks` from TX FIFO.
4. Wait for clean trigger rising edge (`wait 0 pin 0`, then `wait 1 pin 0`).
5. Count down compare ticks.
6. Drive output high.
7. Count down pulse width ticks.
8. Drive output low.
9. Return to idle and wait for next arm.

## Notes

- Trigger pin is selected by mapping PIO IN base in SM config.
- Pulse width is API-configurable per armed event.
- Include path from app code:
  - `#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"`
