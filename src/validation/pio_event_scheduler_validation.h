#ifndef PIO_EVENT_SCHEDULER_VALIDATION_H
#define PIO_EVENT_SCHEDULER_VALIDATION_H

#include <stdint.h>

#include "pico/stdlib.h"

typedef struct {
    uint pio_index;
    uint sm;
    uint trigger_pin;
    uint output_pin;
    uint32_t sm_clk_hz;
    uint32_t compare_ns;
    uint32_t pulse_ns;
    uint32_t event_count;
} pio_event_scheduler_validation_config_t;

// Runs scheduler validation until user requests stop over USB CDC.
void pio_event_scheduler_validation_run(const pio_event_scheduler_validation_config_t *config);

#endif
