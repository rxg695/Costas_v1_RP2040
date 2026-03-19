#ifndef PIO_TIMER_INPUT_CAPTURE_H
#define PIO_TIMER_INPUT_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"

#define PIO_TIMER_INPUT_CAPTURE_TIMEOUT_SENTINEL 0xffffffffu

typedef struct {
    PIO pio;
    uint sm;
    uint offset;
    uint start_pin;
    uint stop_pin;
    uint32_t sm_clk_hz;
    uint32_t timeout_ns;
    uint32_t timeout_loops;
} pio_timer_input_capture_t;

// Initializes the PIO timer input capture block.
// Measures start_pin rising edge -> stop_pin rising edge.
void pio_timer_input_capture_init(pio_timer_input_capture_t *capture,
                                  PIO pio,
                                  uint sm,
                                  uint start_pin,
                                  uint stop_pin,
                                  uint32_t sm_clk_hz,
                                  uint32_t timeout_ns);

// Polls one capture result from RX FIFO.
// Returns true when a result is available, false otherwise.
// - timed_out=true  -> no stop edge before timeout
// - timed_out=false -> elapsed_ticks contains measured delay in timer ticks
bool pio_timer_input_capture_poll(pio_timer_input_capture_t *capture,
                                  uint32_t *elapsed_ticks,
                                  bool *timed_out);

// Converts measured ticks to nanoseconds for this capture configuration.
uint64_t pio_timer_input_capture_ticks_to_ns(const pio_timer_input_capture_t *capture,
                                             uint32_t ticks);

#endif
