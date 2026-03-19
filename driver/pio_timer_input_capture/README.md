# PIO Timer Input Capture Driver

This driver measures elapsed time between two GPIO rising edges using a PIO state machine.

- Start event: rising edge on `start_pin`
- Stop event: rising edge on `stop_pin`
- Output: elapsed timer ticks (or timeout)

## Files

- `pio_timer_input_capture.h` — public C API
- `pio_timer_input_capture.c` — driver implementation
- `pio_timer_input_capture.pio` — PIO program
- `driver_manifest.cmake` — build manifest for driver aggregation

## API

### `pio_timer_input_capture_init(...)`
Initializes one PIO state machine instance for capture.

Parameters:
- `capture` — persistent driver context
- `pio`, `sm` — target PIO block and state machine index
- `start_pin`, `stop_pin` — runtime-configurable input pins
- `sm_clk_hz` — state machine clock frequency
- `timeout_ns` — timeout window per measurement

### `pio_timer_input_capture_poll(...)`
Non-blocking poll for one capture result.

- Returns `false` if RX FIFO has no result yet.
- Returns `true` when a result is available.
- On timeout, `timed_out=true` and `elapsed_ticks=0`.
- On valid capture, `timed_out=false` and `elapsed_ticks` is filled.

### `pio_timer_input_capture_ticks_to_ns(...)`
Converts captured ticks to nanoseconds based on configured `sm_clk_hz`.

## Timing model

The count loop in PIO is:

- `jmp pin got_stop`
- `jmp x-- count_loop`

This loop consumes 2 SM cycles per decrement, so:

- tick period = `2 / sm_clk_hz` seconds
- elapsed time (ns) = `ticks * 2e9 / sm_clk_hz`

Timeout loop count is derived as:

- `timeout_loops = timeout_ns * sm_clk_hz / 2e9`

## PIO behavior summary

1. Pull timeout loop count from TX FIFO once at startup.
2. Wait for clean rising edge on start pin (`wait 0 pin 0`, then `wait 1 pin 0`).
3. Count down until stop pin rises (`jmp pin`) or timeout expires.
4. Push result to RX FIFO:
   - remaining counter on success
   - `0xffffffff` timeout sentinel on timeout
5. Re-arm only after stop pin returns low.

## Notes

- Driver assumes one-time initialization per SM and continuous polling in main loop.
- Avoid reusing the same SM for another program unless disabled/reinitialized.
- Include path from app code:
  - `#include "driver/pio_timer_input_capture/pio_timer_input_capture.h"`
