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
- `mode` — one-shot or continuous stream mode

### `pio_timer_output_compare_arm(...)`
- `compare_ticks` is the number of countdown loop iterations before pulse start.
- `pulse_ticks` is the number of loop iterations to hold the output high.
- In one-shot mode, two TX words arm one trigger->pulse cycle.
- In continuous mode, this appends one scheduled event to the active stream.

### `pio_timer_output_compare_queue_event(...)`
- Explicit helper for continuous mode event enqueue.
- Event format is `(compare_ticks, pulse_ticks)`.

### `pio_timer_output_compare_queue_stop(...)`
- Continuous-mode stream terminator.
- Internally emits stop sentinel pair `(compare_ticks=0, pulse_ticks=0)`.

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
- SM config joins FIFO to TX-only, giving 8 TX words (4 full events queued) on RP2040.
- Include path from app code:
  - `#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"`

## Continuous scheduling concept (for sub-µs symbol timing)

For Costas symbol timing with low jitter, this module can be evolved into two operating modes:

- **One-shot mode** (current behavior):
  - wait for trigger edge
  - consume one `(compare_ticks, pulse_ticks)` pair
  - emit one pulse
- **Continuous mode** (proposed):
  - arm once from PPS
  - consume successive delta/event words from TX FIFO
  - emit pulse per scheduled event until explicit stop command

This keeps orchestration in software while keeping inter-symbol timing in hardware (PIO + optional DMA feed), avoiding CPU re-arm jitter in tight loops.

## Counter range, overflow, and maximum supported timings

The compare and pulse counters are 32-bit values (`uint32_t`), so each per-event delay and per-event pulse width is limited by:

- `max_ticks = 2^32 - 1 = 4,294,967,295`

With state machine clock `sm_clk_hz`, the per-field maximum duration is:

- `max_duration_s = max_ticks / sm_clk_hz`
- `max_duration_ns = floor(max_ticks * 1e9 / sm_clk_hz)`

### Practical maxima per event

- `sm_clk_hz = 125,000,000` -> max per delay/pulse `~34.359738 s`
- `sm_clk_hz = 100,000,000` -> max per delay/pulse `~42.949673 s`
- `sm_clk_hz = 10,000,000` -> max per delay/pulse `~429.496730 s` (`~7.16 min`)
- `sm_clk_hz = 1,000,000` -> max per delay/pulse `~4294.967295 s` (`~71.58 min`)

### `ns_to_ticks` overflow boundary

`pio_timer_output_compare_ns_to_ticks()` computes:

- `ticks = (duration_ns * sm_clk_hz) / 1e9`

and returns `uint32_t`.

So `duration_ns` must satisfy:

- `duration_ns <= floor((2^32 - 1) * 1e9 / sm_clk_hz)`

Above that, cast-to-`uint32_t` truncation occurs and timing wraps.

### Sequence-level limit in continuous mode

In continuous mode, the **per-event** limit remains 32-bit as above, but total sequence length can be much longer as long as FIFO/DMA keeps feeding the next event before underflow.
