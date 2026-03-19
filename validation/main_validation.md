# Validation Main Entrypoint

`main_validation.c` is the dedicated firmware entrypoint used when `PIO_TIMER_CAPTURE_VALIDATION=ON`.

## Role

- Initializes stdio.
- Waits for USB CDC and presents an interactive validation menu.
- Lets user choose validation module and enter run-time parameters.
- Dispatches selected module run.
- Keeps validation startup/routing separate from production `main.c`.

## Why this file exists

- Cleanly separates production firmware entry from validation behavior.
- Allows toggling validation by CMake option without touching production main.
- Centralizes interactive validation UX in one place.

## Related files

- `main.c` — production entrypoint
- `pio_timer_input_capture_validation.c` — validation implementation
- `pio_timer_input_capture_validation.h` — validation module API
- `driver/pio_timer_output_compare/*` — output-compare validation target
- `CMakeLists.txt` — selects this entrypoint in validation mode

## Call flow

1. `main_validation.c` initializes stdio and waits for USB CDC.
2. Shows menu for module selection:
	- input capture validation
	- output compare validation
3. Prompts for module-specific parameters.
4. Runs selected validation routine.
5. Returns to menu when module run exits (`q` in module loop).
