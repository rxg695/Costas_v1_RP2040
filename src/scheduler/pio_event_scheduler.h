#ifndef PIO_EVENT_SCHEDULER_H
#define PIO_EVENT_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hardware/pio.h"

typedef void (*pio_event_scheduler_output_assert_callback_t)(void *user_data);

typedef struct {
    uint32_t compare_ticks;
    uint32_t pulse_ticks;
} pio_event_scheduler_event_t;

typedef struct {
    PIO pio;
    uint sm;

    bool output_assert_irq_enabled;
    uint output_assert_pin;
    pio_event_scheduler_output_assert_callback_t output_assert_callback;
    void *output_assert_user_data;

    const pio_event_scheduler_event_t *events;
    size_t event_count;

    size_t next_event_to_queue;
    bool stop_queued;
    bool active;
} pio_event_scheduler_t;

// Initializes scheduler state for one output-compare state machine.
void pio_event_scheduler_init(pio_event_scheduler_t *scheduler,
                              PIO pio,
                              uint sm);

// Starts scheduling of an event array.
// This does not block and does not reset the state machine; caller is expected
// to have already configured output compare in continuous mode.
// Event feeding is IRQ-driven once started.
void pio_event_scheduler_start(pio_event_scheduler_t *scheduler,
                               const pio_event_scheduler_event_t *events,
                               size_t event_count);

// Optional manual pump/kick for pending events.
// Not required during normal IRQ-driven operation.
void pio_event_scheduler_poll(pio_event_scheduler_t *scheduler);

// Registers a callback invoked from GPIO IRQ context on each output-pin rising edge.
// Caller callback must be short and IRQ-safe.
void pio_event_scheduler_set_output_assert_callback(
    pio_event_scheduler_t *scheduler,
    uint output_pin,
    pio_event_scheduler_output_assert_callback_t callback,
    void *user_data);

// Disables output-assert GPIO IRQ callback for this scheduler.
void pio_event_scheduler_clear_output_assert_callback(pio_event_scheduler_t *scheduler);

// Requests scheduler stop. Already queued events continue; stop command is
// queued when FIFO space allows.
void pio_event_scheduler_request_stop(pio_event_scheduler_t *scheduler);

// Returns true when scheduler has no active queueing work.
// (All events + stop command have been queued to PIO FIFO.)
bool pio_event_scheduler_is_idle(const pio_event_scheduler_t *scheduler);

#endif
