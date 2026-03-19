#ifndef PIO_TIMER_OUTPUT_COMPARE_H
#define PIO_TIMER_OUTPUT_COMPARE_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

// Initializes the pio_timer_output_compare PIO state machine.
// - trigger pin is selected at runtime
// - output pin is selected at runtime and driven by SET PINS
// - sm_clk_hz selects timing resolution
void pio_timer_output_compare_init(PIO pio,
                                   uint sm,
                                   uint offset,
                                   uint trigger_pin,
                                   uint output_pin,
                                   float sm_clk_hz);

// Arms one output-compare event.
// - compare_ticks is the number of countdown loop iterations before pulse start
// - pulse_ticks is the high-time loop count for pulse duration
void pio_timer_output_compare_arm(PIO pio,
                                  uint sm,
                                  uint32_t compare_ticks,
                                  uint32_t pulse_ticks);

// Converts nanoseconds to output-compare ticks for a given SM clock.
// Helper applies to both compare delay and pulse width tick values.
uint32_t pio_timer_output_compare_ns_to_ticks(uint32_t sm_clk_hz,
                                              uint64_t duration_ns);

#endif
