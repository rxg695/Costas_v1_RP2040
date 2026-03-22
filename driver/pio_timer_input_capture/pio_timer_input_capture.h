#ifndef PIO_TIMER_INPUT_CAPTURE_H
#define PIO_TIMER_INPUT_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"

/** Raw RX value used by the PIO program to report a timeout. */
#define PIO_TIMER_INPUT_CAPTURE_TIMEOUT_SENTINEL 0xffffffffu

/**
 * @brief Runtime state for one input-capture instance.
 */
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

/**
 * @brief Initializes one input-capture state machine.
 *
 * Measures the interval from a rising edge on @p start_pin to a rising edge on
 * @p stop_pin, with a timeout defined by @p timeout_ns.
 */
void pio_timer_input_capture_init(pio_timer_input_capture_t *capture,
                                  PIO pio,
                                  uint sm,
                                  uint start_pin,
                                  uint stop_pin,
                                  uint32_t sm_clk_hz,
                                  uint32_t timeout_ns);

/**
 * @brief Polls one result from the RX FIFO.
 *
 * @return true when a result was returned, false when no result is available.
 */
bool pio_timer_input_capture_poll(pio_timer_input_capture_t *capture,
                                  uint32_t *elapsed_ticks,
                                  bool *timed_out);

/**
 * @brief Converts measured ticks to nanoseconds.
 */
uint64_t pio_timer_input_capture_ticks_to_ns(const pio_timer_input_capture_t *capture,
                                             uint32_t ticks);

#endif
