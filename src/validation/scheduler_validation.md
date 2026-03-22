# Scheduler Validation Logic

Interactive validation module for `src/scheduler/scheduler`.

## Submenu modes

- `a` Sequence building
  - Prompts parameters, runs scheduler init+prepare, prints `output_compare_sequence[]` and `alarm_timer_sequence[]`.
- `b` Initialization
  - Prompts parameters, runs init block, prints state and counters.
- `c` Prepare/preload
  - Prompts parameters, runs init + prepare/preload, prints report, performs cleanup reset path.
- `d` Timer orchestration
  - Prompts parameters, runs init + prepare; on `arm` waits for autonomous end-of-sequence and prints report.
- `e` Full integration
  - Prompts parameters and provides `i/p/a/s` command flow.
  - `arm` runs autonomously until end state.

## Defaults

- `dt0 = 15 us`
- `symbol_count = 8`
- `dts = 10 us`
- `load_offset = 7 us`
- `ad9850_spi_baud_hz = 10 MHz`
- default frequency array: `1500 900 1800 600 2400 2100 1200 0`
- Pins:
  - `SCK = GP6`
  - `MOSI = GP7`
  - `FQ_UD = GP5`
  - `RESET = GP9`
  - `PPS = GP8`

## Notes

- Scheduler runtime is interrupt-driven (no scheduler polling loop).
- This module is for interactive validation and reporting, not production control flow.
- Sequence construction uses recurrence:
  - output (N): `out[0]=dt0`; `out[k]=out[k-1]+dts[k-1]` for `k=1..N-1`
  - alarm (N+1): `alarm[0]=dt0-load_offset`; `alarm[k]=alarm[k-1]+dts[k-1]` for `k=1..N-1`; `alarm[N]=alarm[N-1]+load_offset`
- Alarm sequence has `symbol_count+1` entries, and the final marker is computed from recurrence as `alarm[N-1]+load_offset`.
- Validation menu time inputs (`dt0`, `dts`, `load_offset`) are converted from microseconds into scheduler request ticks using the current scheduler output scaling (`5x` output compare scaling in scheduler build path), so entered times map to expected real-time behavior.

## Possible Future Fixes

- First-burst latency optimization: preload symbol 0 AD9850 frame before entering ARM, then start IRQ-driven writes from symbol 1 onward. This can reduce one-time startup skew on the first burst while preserving the existing alarm-driven cadence for the rest of the sequence.
