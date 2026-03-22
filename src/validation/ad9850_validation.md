# AD9850 Validation

Interactive bring-up for the AD9850 driver without involving the scheduler.

## What to verify

- SPI pin mapping and baud rate
- frequency-to-FTW conversion
- frame writes with and without `FQ_UD`
- optional reset pin behavior
- non-blocking transfer path

## Suggested check order

1. start the module and confirm the printed configuration looks right
2. run `e` to perform the serial-enable sequence
3. use `w` and `a` to confirm blocking writes
4. use `n`, `m`, and `v` to step through the non-blocking path
5. adjust frequency with `+` and `-` and verify the FTW changes as expected

## Runtime controls

- `e` serial-enable sequence
- `w` write frame without latch pulse
- `a` write frame and pulse `FQ_UD`
- `p` pulse `FQ_UD`
- `r` pulse reset
- `n` start non-blocking write
- `m` start non-blocking write with latch pulse
- `v` service the non-blocking state machine once
- `b` print busy state
- `x` print non-blocking counters
- `+` and `-` adjust frequency
- `d` toggle power-down
- `s` print current state
- `q` return to the menu

Before `e`, write commands should fail. That is expected and confirms the write guard is active.
