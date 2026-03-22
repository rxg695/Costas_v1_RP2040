# PIO Alarm Timer Validation

Interactive test loop for the PIO alarm timer driver.

## What it exercises

- rearm command handling
- normal alarm firing
- burst scheduling
- descending-tick rejection in the host API

## Runtime controls

- `r` queue a rearm command
- `a` queue a single future alarm
- `b` queue a burst of alarms
- `d` intentionally queue a descending tick to test the guard path
- `q` return to the main menu

Results are printed from the RX IRQ callback path, so this module is useful for checking both the PIO program and the driver-side interrupt dispatch.
