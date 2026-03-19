#include <stdio.h>

#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"
#include "pio_timer_output_compare.pio.h"
#include "src/scheduler/pio_event_scheduler.h"
#include "src/validation/pio_event_scheduler_validation.h"

#define SCHEDULER_VALIDATION_MAX_EVENTS 128u

typedef struct {
    volatile uint32_t output_assert_count;
    uint32_t output_assert_logged_count;
} scheduler_validation_runtime_t;

static PIO resolve_pio(uint pio_index)
{
    return pio_index == 1 ? pio1 : pio0;
}

static void on_output_assert(void *user_data)
{
    scheduler_validation_runtime_t *runtime = (scheduler_validation_runtime_t *) user_data;
    runtime->output_assert_count++;
}

void pio_event_scheduler_validation_run(const pio_event_scheduler_validation_config_t *config)
{
    static bool program_loaded[2] = {false, false};
    static uint program_offset[2] = {0, 0};

    PIO pio = resolve_pio(config->pio_index);
    uint slot = config->pio_index == 1 ? 1u : 0u;

    if (!program_loaded[slot]) {
        program_offset[slot] = pio_add_program(pio, &pio_timer_output_compare_program);
        program_loaded[slot] = true;
    }

    pio_timer_output_compare_init(pio,
                                  config->sm,
                                  program_offset[slot],
                                  config->trigger_pin,
                                  config->output_pin,
                                  (float) config->sm_clk_hz,
                                  PIO_TIMER_OUTPUT_COMPARE_MODE_CONTINUOUS);

    uint32_t compare_ticks = pio_timer_output_compare_ns_to_ticks(config->sm_clk_hz, config->compare_ns);
    uint32_t pulse_ticks = pio_timer_output_compare_ns_to_ticks(config->sm_clk_hz, config->pulse_ns);
    if (pulse_ticks == 0u) {
        pulse_ticks = 1u;
    }

    uint32_t event_count = config->event_count;
    if (event_count > SCHEDULER_VALIDATION_MAX_EVENTS) {
        event_count = SCHEDULER_VALIDATION_MAX_EVENTS;
    }

    pio_event_scheduler_event_t events[SCHEDULER_VALIDATION_MAX_EVENTS];
    for (uint32_t index = 0; index < event_count; ++index) {
        events[index].compare_ticks = compare_ticks;
        events[index].pulse_ticks = pulse_ticks;
    }

    pio_event_scheduler_t scheduler;
    scheduler_validation_runtime_t runtime = {
        .output_assert_count = 0u,
        .output_assert_logged_count = 0u,
    };

    pio_event_scheduler_init(&scheduler, pio, config->sm);
    pio_event_scheduler_set_output_assert_callback(&scheduler,
                                                   config->output_pin,
                                                   on_output_assert,
                                                   &runtime);

    printf("\nScheduler validation active\n");
    printf("PIO%u SM%u trigger=GP%u output=GP%u sm_clk=%lu Hz\n",
           config->pio_index,
           config->sm,
           config->trigger_pin,
           config->output_pin,
           (unsigned long) config->sm_clk_hz);
    printf("Output assert callback enabled on GP%u rising edges\n",
           config->output_pin);
    printf("events=%lu compare=%lu ns (%lu ticks) pulse=%lu ns (%lu ticks)\n",
           (unsigned long) event_count,
           (unsigned long) config->compare_ns,
           (unsigned long) compare_ticks,
           (unsigned long) config->pulse_ns,
           (unsigned long) pulse_ticks);
    printf("Press 'g' to start sequence, 'x' to request stop, 'q' to return\n");

    uint32_t sequence_count = 0;
    bool was_active = false;
    while (true) {
        while (runtime.output_assert_logged_count < runtime.output_assert_count) {
            runtime.output_assert_logged_count++;
            printf("scheduler_output_assert=%lu\n",
                   (unsigned long) runtime.output_assert_logged_count);
        }

        bool active = !pio_event_scheduler_is_idle(&scheduler);
        if (was_active && !active) {
            printf("sequence_queued\n");
        }
        was_active = active;

        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == 'g' || ch == 'G') {
            if (!pio_event_scheduler_is_idle(&scheduler)) {
                printf("scheduler busy\n");
                continue;
            }

            pio_event_scheduler_start(&scheduler, events, event_count);
            sequence_count++;
            printf("sequence_started=%lu\n", (unsigned long) sequence_count);
        } else if (ch == 'x' || ch == 'X') {
            pio_event_scheduler_request_stop(&scheduler);
            printf("stop requested\n");
        } else if (ch == 'q' || ch == 'Q') {
            pio_event_scheduler_request_stop(&scheduler);
            printf("Leaving scheduler validation\n");
            break;
        }
    }

    pio_event_scheduler_clear_output_assert_callback(&scheduler);
}
