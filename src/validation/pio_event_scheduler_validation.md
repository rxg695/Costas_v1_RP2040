# PIO Event Scheduler Validation Logic

This document describes scheduler validation in `pio_event_scheduler_validation.c`.

Companion API header: `pio_event_scheduler_validation.h`.

## Purpose

Validates firmware-level event sequencing using:

- `src/scheduler/pio_event_scheduler`
- `driver/pio_timer_output_compare` in continuous mode

## Runtime flow

1. Resolve selected PIO instance.
2. Load output-compare PIO program for that PIO block (lazy).
3. Initialize output compare in continuous mode.
4. Build a fixed event array (`event_count`) from configured compare/pulse timings.
5. Register output-assert callback on output-pin rising edge.
6. Enter command loop:
   - `g`: start scheduler with the prepared event array
   - `x`: request scheduler stop
   - `q`: request stop and return to menu

## Notes

- Event count is clamped to an internal max (`128`) for static allocation safety.
- Timing remains hardware-driven by PIO once events are queued.
- Scheduler feeding is non-blocking and IRQ-driven after `start`.
- Validation prints callback counts as `scheduler_output_assert=N`.
