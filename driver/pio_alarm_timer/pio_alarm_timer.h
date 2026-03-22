#ifndef PIO_ALARM_TIMER_H
#define PIO_ALARM_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/pio.h"
#include "pico/stdlib.h"

/** Host-to-PIO command word that rearms the timer on the next PPS cycle. */
#define PIO_ALARM_TIMER_CMD_REARM 0u

/** PIO-to-host result word used when an alarm was already late. */
#define PIO_ALARM_TIMER_RESULT_LATE 0u
/** PIO-to-host result word used to acknowledge a rearm command. */
#define PIO_ALARM_TIMER_RESULT_REARM_ACK 0xFFFFFFFFu

/**
 * @brief Result of attempting to queue an alarm command.
 */
typedef enum {
    PIO_ALARM_TIMER_ENQUEUE_OK = 0,
    PIO_ALARM_TIMER_ENQUEUE_ERR_NOT_INIT,
    PIO_ALARM_TIMER_ENQUEUE_ERR_ZERO_TICK,
    PIO_ALARM_TIMER_ENQUEUE_ERR_NON_MONOTONIC,
    PIO_ALARM_TIMER_ENQUEUE_ERR_TX_FULL,
} pio_alarm_timer_enqueue_status_t;

/**
 * @brief Decoded meaning of one RX FIFO result word.
 */
typedef enum {
    PIO_ALARM_TIMER_RESULT_KIND_REARM_ACK = 0,
    PIO_ALARM_TIMER_RESULT_KIND_LATE,
    PIO_ALARM_TIMER_RESULT_KIND_FIRED,
} pio_alarm_timer_result_kind_t;

/**
 * @brief Decoded result returned by the PIO program.
 */
typedef struct {
    pio_alarm_timer_result_kind_t kind;
    uint32_t tick;
} pio_alarm_timer_result_t;

/**
 * @brief Callback signature used for IRQ-driven result dispatch.
 */
typedef void (*pio_alarm_timer_rx_callback_t)(const pio_alarm_timer_result_t *result,
                                              void *user_data);

/**
 * @brief Runtime state for one PIO alarm timer instance.
 */
typedef struct {
    PIO pio;
    uint sm;
    bool initialized;

    /** Tracks the last accepted alarm tick for monotonicity checking. */
    bool has_last_alarm;
    uint32_t last_alarm_tick;

    /** IRQ callback registration state. */
    bool rx_irq_enabled;
    pio_alarm_timer_rx_callback_t rx_callback;
    void *rx_user_data;
} pio_alarm_timer_t;

/**
 * @brief Initializes one state machine for the alarm timer program.
 *
 * @param offset Offset returned when the PIO program was loaded.
 * @param pps_pin Pin sampled by the PIO WAIT instructions during rearm.
 * @param sm_clk_hz Requested state-machine clock.
 */
void pio_alarm_timer_init(pio_alarm_timer_t *timer,
                          PIO pio,
                          uint sm,
                          uint offset,
                          uint pps_pin,
                          float sm_clk_hz);

/**
 * @brief Queues a rearm command.
 *
 * On success, clears the host-side monotonic guard history.
 */
bool pio_alarm_timer_queue_rearm(pio_alarm_timer_t *timer);

/**
 * @brief Queues an alarm tick and rejects descending values.
 */
pio_alarm_timer_enqueue_status_t pio_alarm_timer_queue_alarm(pio_alarm_timer_t *timer,
                                                             uint32_t alarm_tick);

/**
 * @brief Reads one raw result from the RX FIFO when available.
 */
bool pio_alarm_timer_try_read_result(pio_alarm_timer_t *timer,
                                     uint32_t *result_out);

/**
 * @brief Decodes a raw result word into @ref pio_alarm_timer_result_t.
 */
void pio_alarm_timer_decode_result(uint32_t raw_result,
                                   pio_alarm_timer_result_t *decoded_out);

/**
 * @brief Reads and decodes one result from the RX FIFO.
 */
bool pio_alarm_timer_try_read_decoded_result(pio_alarm_timer_t *timer,
                                             pio_alarm_timer_result_t *decoded_out);

/**
 * @brief Enables IRQ-driven RX callback dispatch on the PIO IRQ0 line.
 */
void pio_alarm_timer_set_rx_irq_callback(pio_alarm_timer_t *timer,
                                         pio_alarm_timer_rx_callback_t callback,
                                         void *user_data);

/**
 * @brief Disables IRQ-driven RX callback dispatch for this instance.
 */
void pio_alarm_timer_clear_rx_irq_callback(pio_alarm_timer_t *timer);

#endif
