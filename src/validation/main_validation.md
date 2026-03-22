# Validation Main Entrypoint

`src/validation/main_validation.c` is the firmware entry point used by the `validation` preset.

## Role

It does three things:

1. bring up stdio over USB
2. present the top-level validation menu
3. dispatch into the selected module

Keeping that logic separate from `src/main.c` makes it easy to switch between production and hardware bring-up builds without editing application code.

## Modules exposed by the menu

- input capture
- output compare
- alarm timer
- AD9850
- scheduler

Each module owns its own prompts and command loop. Returning with `q` drops back to the main menu.
