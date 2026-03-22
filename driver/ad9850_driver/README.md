# AD9850 Driver (HW SPI)

Low-level AD9850 driver built on RP2040 hardware SPI (`hardware/spi.h`).

## Scope

- Build AD9850 40-bit frame (`FTW + control`)
- Explicit serial-enable sequence API (`RST`, `W_CLK`, `FQ_UD`)
- Write frame over hardware SPI (`spi_write_blocking`)
- Optional `FQ_UD` pulse
- Optional hardware reset pulse
- Frequency Hz to FTW conversion

## Non-goals

- No scheduler/timeline policy
- No queueing or DMA orchestration in this layer

## Frame format

Bytes are sent in AD9850 order:

1. FTW[7:0]
2. FTW[15:8]
3. FTW[23:16]
4. FTW[31:24]
5. control (`phase[4:0] << 3`, power-down bit `0x04`)

## Typical usage

1. `ad9850_driver_init(...)`
2. `ad9850_driver_serial_enable(...)`
3. `ad9850_driver_frequency_hz_to_ftw(...)`
4. `ad9850_driver_make_frame(...)`
5. `ad9850_driver_apply_frame_blocking(..., true)`

## Serial-enable contract

`ad9850_driver_serial_enable(...)` executes:

1. Master reset pulse on `RST` (active-high, if configured)
2. One `W_CLK` pulse on the configured SCK pin
3. One `FQ_UD` pulse

After reset, write operations are locked until serial-enable is executed again.
