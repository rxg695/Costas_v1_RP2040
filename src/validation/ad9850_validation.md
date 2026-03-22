# AD9850 Validation Logic

This module provides interactive validation for `driver/ad9850_driver` over RP2040 hardware SPI.

## Purpose

Validate AD9850 transport behavior independent of scheduler logic:

- SPI initialization with runtime pin mapping
- Frequency to FTW conversion
- Frame build and write path
- Optional `FQ_UD` pulse path
- Optional reset pulse path

## Validation Methodology

1. Build and flash validation firmware.
2. Open USB serial terminal and select `4) AD9850 validation` from the main menu.
3. Enter board-specific SPI and pin configuration:
	- `spi_index`, `spi_baud_hz`, `sck_pin`, `mosi_pin`
	- optional `FQ_UD` and `RESET` enable/pin settings
	- `dds_sysclk_hz`, initial `frequency_hz`, initial `phase`, initial `power_down`
4. Confirm startup status line prints valid FTW and selected transport settings.
5. Run command checks in this order:
	- `e`: run serial-enable sequence:
		1) pulse `RST` high (if reset pin enabled)
		2) pulse `W_CLK` high (on SPI SCK pin)
		3) pulse `FQ_UD` high
	- `w`: verify SPI frame transfer path (no latch)
	- `p`: verify standalone `FQ_UD` pulse (if enabled)
	- `a`: verify write + latch combined path
	- `n`: start non-blocking write path (no latch pulse)
	- `m`: start non-blocking write + optional latch pulse
	- `v`: service non-blocking transfer once
	- `b`: print non-blocking busy flag
	- `x`: print non-blocking counters (start/completion)
	- `r`: verify reset pulse path (if enabled)
	- `+` / `-`: verify FTW updates track frequency changes
	- `d`: verify control-byte power-down bit toggling
	- `s`: verify current state print matches expected values

### Expected Observations

- Serial output reports `ok` for enabled transport paths.
- `ftw` value changes monotonically with `+` / `-` commands.
- `power_down` status toggles on each `d` command.
- If `FQ_UD` or `RESET` is disabled, commands report `failed_or_disabled`.
- Before `e`, write/apply commands are expected to fail (write guard active).

### Pass Criteria

- No command causes lockup or unexpected reboot.
- SPI write/apply operations are consistently successful.
- Reported status fields remain coherent across repeated command sequences.
- Scope/logic-analyzer traces (when connected) confirm expected SPI bytes and latch/reset pulse timing.

## Runtime controls

- `a`: apply frame (write + optional `FQ_UD` pulse)
- `w`: write frame only (no latch pulse)
- `p`: pulse `FQ_UD`
- `r`: pulse reset
- `e`: run serial-enable initialization sequence
- `n`: start non-blocking apply (no latch pulse)
- `m`: start non-blocking apply with latch pulse
- `v`: service non-blocking transfer once
- `b`: print non-blocking busy status
- `x`: print non-blocking counters
- `+`: frequency up by current step
- `-`: frequency down by current step
- `d`: toggle power-down bit
- `s`: print current status
- `q`: return to menu

## Notes

- This validation is intentionally scheduler-free.
- It is suitable for scope/logic-analyzer verification of SPI and latch timing.
- AD9850 write path is intentionally guarded: serial-enable must be executed before write/apply operations.
