# Input Capture Validation

Continuous validation loop for the input-capture driver.

## What it reports

- measured delay from `start_pin` to `stop_pin`
- timeout count
- per-block statistics
- run-wide min, max, and jitter

## How it runs

The module initializes the capture state machine, polls for results, and prints a summary every configured sample block. Timeouts are counted separately so you can tell whether bad statistics come from jitter or from outright missed captures.

## Use case

This is the right validation mode when you want to compare repeated measurements from a stable signal source and watch both the spread and the timeout rate over time.
