# Scheduler Module

Scheduler prototype that coordinates three lower-level pieces:

- output compare for pulse generation
- alarm timer for timed callbacks relative to PPS
- AD9850 writes for symbol updates

This module is the first place in the tree where timing policy starts to exist. The drivers below it stay hardware-focused; the scheduler decides when those pieces are used together.

## Role in the system

The scheduler is meant to turn a symbol plan into two synchronized timing streams:

- an output-compare stream used to generate timed pulses
- an alarm stream used to trigger AD9850 writes at the right points in the sequence

At prepare time it builds those sequences in memory. At arm time it re-initializes the relevant state machines, queues the initial commands, and then lets the interrupt-driven feed path finish the run.

## Public types

### `scheduler_state_t`

Lifecycle state for one scheduler instance.

- `SCHEDULER_STATE_INIT`
- `SCHEDULER_STATE_IDLE`
- `SCHEDULER_STATE_PREPARE_PRELOAD`
- `SCHEDULER_STATE_ARM`
- `SCHEDULER_STATE_END_OK`
- `SCHEDULER_STATE_END_FAULT`

### `scheduler_error_t`

Last-error code reported by the scheduler.

- `SCHEDULER_ERROR_NONE`
- `SCHEDULER_ERROR_INVALID_ARG`
- `SCHEDULER_ERROR_NOT_READY`
- `SCHEDULER_ERROR_SEQUENCE_OVERFLOW`
- `SCHEDULER_ERROR_ALARM_ENQUEUE`
- `SCHEDULER_ERROR_OUTPUT_ENQUEUE`
- `SCHEDULER_ERROR_AD9850`
- `SCHEDULER_ERROR_TIMER_FAULT`
- `SCHEDULER_ERROR_UNEXPECTED_ALARM_TICK`

### `scheduler_config_t`

Static configuration for one initialized scheduler.

It includes:

- PIO blocks and state-machine indices for output compare and alarm timer
- trigger, output, and PPS pins
- state-machine clock and output pulse width
- AD9850 SPI and control-pin configuration

### `scheduler_prepare_request_t`

Per-run sequence description.

- `symbol_count`: number of symbols in the prepared burst
- `dt0`: initial delay before the first event
- `dts`: per-symbol spacing array
- `load_offset`: offset between alarm scheduling and output scheduling
- `ftw_frames`: optional prebuilt DDS frames
- `freq_hz`: optional frequency list used when frames are not supplied

Exactly one of `ftw_frames` or `freq_hz` must be provided.

### `scheduler_t`

Full runtime object. The caller owns this struct.

Besides public state and config, it contains:

- the embedded alarm timer and AD9850 driver instances
- software FIFOs used to stage output and alarm words
- the prepared timing arrays and DDS frames
- counters used by validation and debugging

## API reference

### `scheduler_init(...)`

Initializes the scheduler and the driver instances it owns.

What happens here:

1. validate the configuration
2. load the output-compare and alarm-timer PIO programs if needed
3. initialize both state machines
4. initialize the AD9850 driver and run serial-enable
5. install the shared PIO IRQ feeders
6. leave the scheduler in `IDLE`

Returns `false` if configuration is invalid or any required lower-level setup fails.

### `scheduler_prepare(...)`

Builds the internal timing arrays and DDS frame list for one burst.

Important behavior:

- clears the software FIFOs
- derives the output-compare and alarm sequences from `dt0`, `dts`, and `load_offset`
- precomputes DDS frames either from `ftw_frames` or from `freq_hz`
- leaves the scheduler in `IDLE` with `prepared = true` on success

### `scheduler_arm(...)`

Starts one prepared run.

Before arming, the scheduler resets both PIO state machines to a known state so stale edges or FIFO state from the previous run cannot leak into the next one. It then queues the rearm command for the alarm timer, enables FIFO-driven IRQ feeding, and lets the run proceed.

### `scheduler_reset(...)`

Returns the scheduler to a reusable state after an `END_OK` or `END_FAULT` terminal condition.

### `scheduler_set_fault_callback(...)`

Registers an optional callback invoked when the scheduler enters the fault state.

### `scheduler_on_tx_fifo_not_full_irq(...)`

IRQ entry point used to keep the output and alarm FIFOs fed.

### `scheduler_on_alarm_result(...)`

Callback adapter used by the embedded alarm timer instance to report results back into the scheduler.

### `scheduler_get_state(...)` and `scheduler_get_last_error(...)`

Simple accessors used by validation code and higher-level orchestration.

## Sequence construction

For a request with `N` symbols, the scheduler builds:

- output compare sequence of length `N`
- alarm sequence of length `N + 1`

The output side currently uses a `5x` scale factor relative to the alarm request ticks. That scaling is part of the module’s current timing model and is reflected in the validation tooling.

## State machine

Typical successful flow:

1. `scheduler_init(...)`
2. `scheduler_prepare(...)`
3. `scheduler_arm(...)`
4. autonomous interrupt-driven run
5. `END_OK`
6. `scheduler_reset(...)`

If any driver or queueing step fails, the scheduler enters `END_FAULT`, records the error, and optionally raises the fault callback.

## Notes on ownership and concurrency

- one scheduler owns one output-compare SM and one alarm-timer SM
- the same PIO block may be used for both, but not the same state machine
- the module assumes it owns those hardware resources exclusively while active
- the AD9850 driver instance is embedded inside the scheduler and should not be manipulated externally once initialized

## Validation support

Interactive scheduler bring-up lives in [src/validation/scheduler_validation.md](../validation/scheduler_validation.md) and [src/validation/scheduler_validation.c](../validation/scheduler_validation.c).

Use that path when you need to:

- inspect the generated timing arrays
- verify init and prepare behavior
- run the interrupt-driven timing path without the rest of the application stack

## Current limitations

- `symbol_count` is capped by `SCHEDULER_MAX_SYMBOLS`
- validation currently seeds only a short default frequency list
- the scheduler stops at DDS and timing orchestration; it does not yet control rig/PTT state or RF-on capture in the production path
- the output scaling and some startup behavior are still prototype-level decisions rather than a finalized interface contract