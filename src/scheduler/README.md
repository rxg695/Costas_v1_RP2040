# PIO Event Scheduler (Firmware Module)

This module provides a firmware-layer scheduler for output-compare **continuous mode**.

It is not a hardware driver: it owns event sequence policy and IRQ-driven FIFO feeding.

## Files

- `pio_event_scheduler.h` — public scheduler API
- `pio_event_scheduler.c` — scheduler implementation

## Event model

Each event is:

- `compare_ticks` — delta ticks before pulse
- `pulse_ticks` — pulse high duration ticks

The scheduler pushes `(compare_ticks, pulse_ticks)` pairs into the output-compare TX FIFO.

Optional output-assert callback:

- Scheduler can register a GPIO rising-edge IRQ callback on the output pin.
- Callback fires on each actual output assertion (pulse start).
- This is the preferred integration point for loading next AD9850 word in firmware.

## Runtime model

1. Initialize with target `PIO` + `SM`.
2. Start with an event array (`start`).
3. Call `start` with an event array.
4. Module uses PIO TX-not-full IRQ service to push events when FIFO space is available.
5. Module appends a stop command (`compare_ticks=0`) after final event.
6. Optional output-assert callback handles per-pulse side effects.

## Notes

- Designed for low/moderate symbol-rate sequencing with CPU-fed FIFO.
- Uses IRQ-driven non-blocking writes (`pio_sm_put`) gated by FIFO level checks.
- Assumes output-compare SM is configured in continuous mode and TX FIFO is joined (`PIO_FIFO_JOIN_TX`).
