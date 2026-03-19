#ifndef PIO_TIMER_OUTPUT_COMPARE_H
#define PIO_TIMER_OUTPUT_COMPARE_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

typedef enum {
    PIO_TIMER_OUTPUT_COMPARE_MODE_ONE_SHOT = 0,
    PIO_TIMER_OUTPUT_COMPARE_MODE_CONTINUOUS = 1,
} pio_timer_output_compare_mode_t;

// Public queue sizing constants for firmware schedulers.
// Driver config uses PIO FIFO TX join mode on RP2040.
#define PIO_TIMER_OUTPUT_COMPARE_TX_FIFO_WORDS 8u
#define PIO_TIMER_OUTPUT_COMPARE_WORDS_PER_EVENT 2u

// Special compare value used only in continuous mode to stop event playback.
#define PIO_TIMER_OUTPUT_COMPARE_STOP_COMPARE_TICKS 0u

// Initializes the pio_timer_output_compare PIO state machine.
// - trigger pin is selected at runtime
// - output pin is selected at runtime and driven by SET PINS
// - sm_clk_hz selects timing resolution
// - mode selects one-shot trigger mode or continuous PPS-gated event stream mode
void pio_timer_output_compare_init(PIO pio,
                                   uint sm,
                                   uint offset,
                                   uint trigger_pin,
                                   uint output_pin,
                                   float sm_clk_hz,
                                   pio_timer_output_compare_mode_t mode);

// Arms one output-compare event.
// - compare_ticks is the number of countdown loop iterations before pulse start
// - pulse_ticks is the high-time loop count for pulse duration
// - in one-shot mode: event fires on next trigger edge
// - in continuous mode: event is appended to current stream
void pio_timer_output_compare_arm(PIO pio,
                                  uint sm,
                                  uint32_t compare_ticks,
                                  uint32_t pulse_ticks);

// Enqueues one event in continuous mode.
// compare_ticks uses delta from previous event (or PPS edge for first event).
void pio_timer_output_compare_queue_event(PIO pio,
                                          uint sm,
                                          uint32_t compare_ticks,
                                          uint32_t pulse_ticks);

// Enqueues a stop command in continuous mode.
// When consumed, playback stops and waits for the next trigger edge.
void pio_timer_output_compare_queue_stop(PIO pio,
                                         uint sm);

// Converts nanoseconds to output-compare ticks for a given SM clock.
// Helper applies to both compare delay and pulse width tick values.
uint32_t pio_timer_output_compare_ns_to_ticks(uint32_t sm_clk_hz,
                                              uint64_t duration_ns);

#endif
