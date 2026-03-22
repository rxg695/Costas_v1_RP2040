# PIO Timer Input Capture

Driver for measuring the time between a start edge and a stop edge with one PIO state machine.

## What it measures

- start condition: rising edge on `start_pin`
- stop condition: rising edge on `stop_pin`
- result: elapsed ticks or a timeout sentinel

## Public types and constants

### `PIO_TIMER_INPUT_CAPTURE_TIMEOUT_SENTINEL`

Raw RX value used by the PIO program to report a timeout.

### `pio_timer_input_capture_t`

Per-instance runtime state. This struct stores the selected PIO block, state machine, pins, configured clock, timeout window, and derived timeout loop count.

## API reference

### `pio_timer_input_capture_init(...)`

Initializes the state machine, loads the PIO program, configures the two input pins, computes the timeout loop count, and starts the state machine.

Parameters that matter most:

- `start_pin`: rising edge that starts the measurement
- `stop_pin`: rising edge that ends the measurement
- `sm_clk_hz`: timing base for the loop counter
- `timeout_ns`: maximum measurement window before a timeout is reported

### `pio_timer_input_capture_poll(...)`

Non-blocking poll for one capture result.

When it returns `true`:

- `timed_out = true` means no stop edge arrived before the timeout
- `timed_out = false` means `elapsed_ticks` contains the measured interval

### `pio_timer_input_capture_ticks_to_ns(...)`

Converts reported ticks into nanoseconds using the configured clock and the known loop structure.

## How timing is counted

The capture loop uses two state-machine cycles per decrement, so one reported tick corresponds to:

- `2 / sm_clk_hz` seconds

`pio_timer_input_capture_ticks_to_ns(...)` applies that conversion for the configured instance.

## Runtime behavior

1. load the timeout count once during initialization
2. wait for a clean start edge
3. count until the stop edge arrives or the timeout expires
4. push the result to the RX FIFO
5. wait for the stop pin to return low before re-arming

## Typical usage

1. initialize the capture instance
2. poll for results in the main loop or a dedicated task
3. convert successful results to nanoseconds as needed
4. track timeout rate separately from valid captures

This driver is meant to be polled from firmware or exercised through the validation build while checking timing with a known signal source.

## Integration notes

- Initialization currently loads the PIO program itself, so repeated setup on the same PIO block must be managed carefully.
- The driver assumes exclusive use of the selected state machine.
- Timeout statistics are often as useful as the measured intervals; if captures are missing entirely, look at signal integrity and edge conditioning before blaming jitter.
