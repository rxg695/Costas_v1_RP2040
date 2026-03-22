#ifndef PIO_TIMER_OUTPUT_COMPARE_H
#define PIO_TIMER_OUTPUT_COMPARE_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

/**
 * @brief Operating mode for the output-compare state machine.
 */
typedef enum {
    PIO_TIMER_OUTPUT_COMPARE_MODE_ONE_SHOT = 0,
    PIO_TIMER_OUTPUT_COMPARE_MODE_CONTINUOUS = 1,
} pio_timer_output_compare_mode_t;

/** Number of TX words available when the FIFO is joined for TX use. */
#define PIO_TIMER_OUTPUT_COMPARE_TX_FIFO_WORDS 8u
/** Number of words consumed by one queued event. */
#define PIO_TIMER_OUTPUT_COMPARE_WORDS_PER_EVENT 2u

/** Compare value reserved for the continuous-mode stop sentinel. */
#define PIO_TIMER_OUTPUT_COMPARE_STOP_COMPARE_TICKS 0u

/**
 * @brief Initializes the output-compare state machine.
 *
 * @param offset Offset returned when the PIO program was loaded.
 * @param trigger_pin Pin used as the trigger input.
 * @param output_pin Pin driven high for the generated pulse.
 * @param sm_clk_hz Requested state-machine clock.
 * @param mode One-shot or continuous playback mode.
 */
void pio_timer_output_compare_init(PIO pio,
                                   uint sm,
                                   uint offset,
                                   uint trigger_pin,
                                   uint output_pin,
                                   float sm_clk_hz,
                                   pio_timer_output_compare_mode_t mode);

/**
 * @brief Arms one event in one-shot mode or appends one event in continuous mode.
 */
void pio_timer_output_compare_arm(PIO pio,
                                  uint sm,
                                  uint32_t compare_ticks,
                                  uint32_t pulse_ticks);

/**
 * @brief Enqueues one event in continuous mode.
 */
void pio_timer_output_compare_queue_event(PIO pio,
                                          uint sm,
                                          uint32_t compare_ticks,
                                          uint32_t pulse_ticks);

/**
 * @brief Non-blocking version of @ref pio_timer_output_compare_queue_event.
 *
 * @return true when both words were queued, false if the TX FIFO lacked room.
 */
bool pio_timer_output_compare_try_queue_event(PIO pio,
                                              uint sm,
                                              uint32_t compare_ticks,
                                              uint32_t pulse_ticks);

/**
 * @brief Enqueues the continuous-mode stop sentinel.
 */
void pio_timer_output_compare_queue_stop(PIO pio,
                                         uint sm);

/**
 * @brief Non-blocking version of @ref pio_timer_output_compare_queue_stop.
 */
bool pio_timer_output_compare_try_queue_stop(PIO pio,
                                             uint sm);

/**
 * @brief Converts nanoseconds to state-machine ticks.
 */
uint32_t pio_timer_output_compare_ns_to_ticks(uint32_t sm_clk_hz,
                                              uint64_t duration_ns);

#endif
