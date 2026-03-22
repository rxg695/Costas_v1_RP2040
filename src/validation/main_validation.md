# Validation Main Entrypoint

`src/validation/main_validation.c` is the dedicated firmware entrypoint used when `PIO_TIMER_CAPTURE_VALIDATION=ON`.

## Role

- Initializes stdio.
- Waits for USB CDC and presents an interactive validation menu.
- Lets user choose validation module and enter run-time parameters.
- Dispatches selected module run.
- Keeps validation startup/routing separate from production `src/main.c`.

## Why this file exists

- Cleanly separates production firmware entry from validation behavior.
- Allows toggling validation by CMake option without touching production main.
- Centralizes interactive validation UX in one place.

## Related files

- `src/main.c` — production entrypoint
- `src/validation/pio_timer_input_capture_validation.c` — validation implementation
- `src/validation/pio_timer_input_capture_validation.h` — validation module API
- `src/validation/pio_timer_output_compare_validation.c` — output compare validation implementation
- `src/validation/pio_alarm_timer_validation.c` — alarm timer validation implementation
- `src/validation/ad9850_validation.c` — AD9850 hardware SPI validation implementation
- `src/validation/scheduler_validation.c` — scheduler orchestration validation implementation
- `driver/pio_timer_output_compare/*` — output-compare validation target
- `driver/pio_alarm_timer/*` — alarm timer validation target
- `driver/ad9850_driver/*` — AD9850 transport validation target
- `src/scheduler/scheduler.*` — scheduler policy module under validation
- `CMakeLists.txt` — selects this entrypoint in validation mode

## Call flow

1. `src/validation/main_validation.c` initializes stdio and waits for USB CDC.
2. Shows menu for module selection:
	- input capture validation
	- output compare validation
	- pio alarm timer validation
	- ad9850 validation
	- scheduler validation
3. Prompts for module-specific parameters.
4. Runs selected validation routine.
5. Returns to menu when module run exits (`q` in module loop).
