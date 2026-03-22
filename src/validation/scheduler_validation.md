# Scheduler Validation

Interactive test module for the scheduler prototype in `src/scheduler/`.

## Submenu modes

- `a` build the output-compare and alarm sequences and print them
- `b` run initialization only and print the resulting state
- `c` initialize, prepare, and preload the scheduler
- `d` run timer orchestration through to completion
- `e` run the fuller integration path with interactive control

## Default inputs

- `dt0 = 15 us`
- `symbol_count = 8`
- `dts = 10 us`
- `load_offset = 7 us`
- `ad9850_spi_baud_hz = 10 MHz`
- default frequencies: `1500 900 1800 600 2400 2100 1200 0`

Default pin mapping:

- `SCK = GP6`
- `MOSI = GP7`
- `FQ_UD = GP5`
- `RESET = GP9`
- `PPS = GP8`

## Sequence model

For `N` symbols, the validation code builds:

- output sequence: `out[0] = dt0`, then `out[k] = out[k - 1] + dts[k - 1]`
- alarm sequence: `alarm[0] = dt0 - load_offset`, then `alarm[k] = alarm[k - 1] + dts[k - 1]`, plus one final trailing marker

This module is useful for checking scheduler state transitions and timing assumptions before integrating more application logic around it.
