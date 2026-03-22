# PIO Alarm Timer

PIO-based timer that accepts alarm commands from the CPU and reports the result back through the RX FIFO.

## What the module provides

- one free-running timer inside a PIO state machine
- a host-to-PIO command channel through the TX FIFO
- a PIO-to-host result channel through the RX FIFO
- optional RX IRQ callback dispatch in the driver layer
- PPS-gated rearm behavior

## Command and result words

Commands sent by the host:

- `0`: rearm and wait for the next PPS release
- `T > 0`: schedule an alarm at tick `T`

Results returned by the PIO program:

- `0xFFFFFFFF`: rearm acknowledged
- `0`: alarm was already late when it was evaluated
- `T`: alarm fired at tick `T`

## Public types

### `pio_alarm_timer_enqueue_status_t`

Return value used by `pio_alarm_timer_queue_alarm(...)`.

- `PIO_ALARM_TIMER_ENQUEUE_OK`: command accepted
- `PIO_ALARM_TIMER_ENQUEUE_ERR_NOT_INIT`: driver not initialized
- `PIO_ALARM_TIMER_ENQUEUE_ERR_ZERO_TICK`: `0` is reserved for rearm
- `PIO_ALARM_TIMER_ENQUEUE_ERR_NON_MONOTONIC`: requested tick was lower than the last accepted one
- `PIO_ALARM_TIMER_ENQUEUE_ERR_TX_FULL`: TX FIFO had no room

### `pio_alarm_timer_result_kind_t` and `pio_alarm_timer_result_t`

Decoded form of one RX word returned by the PIO program.

- `REARM_ACK`: the rearm command completed
- `LATE`: the alarm was already stale when evaluated
- `FIRED`: the alarm reached its requested tick

### `pio_alarm_timer_t`

Runtime driver state. The caller owns this object and must keep it alive for as long as any IRQ callback remains registered.

## API reference

### `pio_alarm_timer_init(...)`

Initializes one state machine using a previously loaded `pio_alarm_timer_program` offset.

Important parameters:

- `offset`: instruction-memory location of the loaded PIO program
- `pps_pin`: pin sampled by `WAIT PIN` during rearm
- `sm_clk_hz`: state-machine clock used for the timer counter

### `pio_alarm_timer_queue_rearm(...)`

Queues the special rearm command and clears the host-side monotonic history. Returns `false` if the timer is not initialized or the TX FIFO is full.

### `pio_alarm_timer_queue_alarm(...)`

Queues one future alarm tick.

The driver rejects descending tick values because the PIO program assumes a forward-moving schedule. If the caller tries to enqueue a lower value than the last accepted alarm, the driver first queues a rearm and then returns `PIO_ALARM_TIMER_ENQUEUE_ERR_NON_MONOTONIC`.

### `pio_alarm_timer_try_read_result(...)`

Reads one raw 32-bit result word if one is available.

### `pio_alarm_timer_decode_result(...)`

Converts a raw result word into `pio_alarm_timer_result_t`.

### `pio_alarm_timer_try_read_decoded_result(...)`

Convenience helper that combines read and decode in one call.

### `pio_alarm_timer_set_rx_irq_callback(...)`

Registers a callback on the PIO IRQ0 line so results can be consumed in interrupt context. The callback must stay short and non-blocking.

### `pio_alarm_timer_clear_rx_irq_callback(...)`

Disables the IRQ dispatch path for that timer instance.

## Behavior

- the timer runs inside one state machine
- rearm waits for a PPS edge before the timer starts accepting new alarms
- the host API tracks the last queued alarm and rejects descending values
- on a descending request, the driver forces a rearm so the host and PIO state stay aligned

## Practical notes

The PIO program is intentionally small. It does not try to solve every stale-command case in hardware, so the host-side monotonic check is part of the design, not a temporary workaround.

## Integration notes

- The timer does not reset between alarm commands; only a rearm command restarts the PPS-gated path.
- The driver’s monotonic guard is not optional. If a caller wants to start over with an earlier schedule, it should explicitly rearm first.
- The IRQ dispatch code installs one shared handler per PIO block and routes callbacks by state-machine index.

Use the validation build if you want to verify PPS rearm behavior and late-alarm handling on hardware.
