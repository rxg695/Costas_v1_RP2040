# Output Compare Validation Logic

This document describes the validation loop implemented in `pio_timer_output_compare_validation.c`.

Companion API header: `pio_timer_output_compare_validation.h`.

## Purpose

Runs interactive validation of the `pio_timer_output_compare` driver by:

- configuring trigger/output pins and timing parameters,
- selecting one-shot or continuous mode,
- arming or queueing compare/pulse events on command,
- allowing repeated arming while monitoring behavior on hardware.

## Runtime flow

1. Resolve target PIO instance (`pio0` or `pio1`).
2. Lazily load the `pio_timer_output_compare` PIO program for that PIO block.
3. Initialize selected state machine with configured pins and SM clock.
4. Convert nanosecond timing inputs into ticks:
   - `compare_ns -> compare_ticks`
   - `pulse_ns -> pulse_ticks`
5. Enter command loop:
   - one-shot mode: `a` arms one output compare event
   - continuous mode:
     - `a` queues one event
       - `4` queues up to four identical events
     - `s` queues stop command
   - `q`: stop validation and return to main menu

## Invocation

This logic is called from `src/validation/main_validation.c` through:

- `pio_timer_output_compare_validation_run(&config)`

Config fields:

- `pio_index`, `sm`
- `trigger_pin`, `output_pin`
- `continuous_mode`
- `sm_clk_hz`
- `compare_ns`, `pulse_ns`

## Notes

- Program offset is cached per PIO block (`pio0`/`pio1`) for repeated runs.
- `pulse_ticks` is clamped to at least 1 tick to avoid zero-length pulse requests.
- Validation queue operations are FIFO-capacity aware to avoid blocking if PPS has not yet triggered event consumption.
