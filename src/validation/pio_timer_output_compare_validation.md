# Output Compare Validation

Interactive hardware check for the output-compare driver.

## What it covers

- pin mapping for trigger and output
- one-shot mode
- continuous queueing mode
- conversion from nanoseconds to compare and pulse ticks
- FIFO-aware queueing during repeated tests

## Runtime controls

- `a` arm or queue one event
- `4` queue up to four identical events in continuous mode
- `s` queue the stop sentinel in continuous mode
- `q` return to the menu

This module is best used with a scope or logic analyzer so you can confirm both the trigger relationship and the generated pulse width.
