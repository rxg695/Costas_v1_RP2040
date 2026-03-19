#include <stdio.h>

#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"
#include "pio_timer_output_compare.pio.h"
#include "src/validation/pio_timer_output_compare_validation.h"

static PIO resolve_pio(uint pio_index) {
    return pio_index == 1 ? pio1 : pio0;
}

static bool queue_pair_if_room(PIO pio,
                               uint sm,
                               uint32_t compare_ticks,
                               uint32_t pulse_ticks)
{
    // Driver config joins FIFO to TX, giving 8 words total.
    // One event uses 2 words => require at least 2 free words.
    const uint tx_fifo_capacity_words = 8u;
    uint tx_level = pio_sm_get_tx_fifo_level(pio, sm);
    if (tx_level > (tx_fifo_capacity_words - 2u)) {
        return false;
    }

    pio_sm_put(pio, sm, compare_ticks);
    pio_sm_put(pio, sm, pulse_ticks);
    return true;
}

void pio_timer_output_compare_validation_run(const pio_timer_output_compare_validation_config_t *config)
{
    static bool program_loaded[2] = {false, false};
    static uint program_offset[2] = {0, 0};

    PIO pio = resolve_pio(config->pio_index);
    uint slot = config->pio_index == 1 ? 1u : 0u;

    if (!program_loaded[slot]) {
        program_offset[slot] = pio_add_program(pio, &pio_timer_output_compare_program);
        program_loaded[slot] = true;
    }

    pio_timer_output_compare_mode_t mode =
        config->continuous_mode ? PIO_TIMER_OUTPUT_COMPARE_MODE_CONTINUOUS
                                : PIO_TIMER_OUTPUT_COMPARE_MODE_ONE_SHOT;

    pio_timer_output_compare_init(pio,
                                  config->sm,
                                  program_offset[slot],
                                  config->trigger_pin,
                                  config->output_pin,
                                  (float) config->sm_clk_hz,
                                  mode);

    uint32_t compare_ticks = pio_timer_output_compare_ns_to_ticks(config->sm_clk_hz, config->compare_ns);
    uint32_t pulse_ticks = pio_timer_output_compare_ns_to_ticks(config->sm_clk_hz, config->pulse_ns);
    if (pulse_ticks == 0) {
        pulse_ticks = 1;
    }

    printf("\nOutput compare validation active\n");
    printf("PIO%u SM%u trigger=GP%u output=GP%u sm_clk=%lu Hz\n",
           config->pio_index,
           config->sm,
           config->trigger_pin,
           config->output_pin,
           (unsigned long) config->sm_clk_hz);
    printf("mode=%s\n", config->continuous_mode ? "continuous" : "one-shot");
    printf("compare=%lu ns (%lu ticks), pulse=%lu ns (%lu ticks)\n",
           (unsigned long) config->compare_ns,
           (unsigned long) compare_ticks,
           (unsigned long) config->pulse_ns,
           (unsigned long) pulse_ticks);
    if (config->continuous_mode) {
        printf("Press 'a' to queue 1 event, '4' to queue 4 events, 's' to queue stop, 'q' to return to menu\n");
    } else {
        printf("Press 'a' to arm one event, 'q' to return to menu\n");
    }

    uint32_t armed_count = 0;
    while (true) {
        int ch = getchar_timeout_us(10000);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == 'a' || ch == 'A') {
            if (config->continuous_mode) {
                if (!queue_pair_if_room(pio, config->sm, compare_ticks, pulse_ticks)) {
                    printf("TX FIFO full, trigger/consume events then retry\n");
                    continue;
                }
            } else {
                pio_timer_output_compare_arm(pio, config->sm, compare_ticks, pulse_ticks);
            }
            armed_count++;
            printf("queued=%lu\n", (unsigned long) armed_count);
        } else if ((ch == '4') && config->continuous_mode) {
            uint32_t queued_now = 0;
            for (uint32_t index = 0; index < 4u; ++index) {
                if (!queue_pair_if_room(pio, config->sm, compare_ticks, pulse_ticks)) {
                    break;
                }
                queued_now++;
                armed_count++;
            }
            printf("queued_now=%lu total_queued=%lu\n",
                   (unsigned long) queued_now,
                   (unsigned long) armed_count);
            if (queued_now < 4u) {
                printf("TX FIFO capacity reached before 4 events; trigger/consume then queue remaining\n");
            }
        } else if ((ch == 's' || ch == 'S') && config->continuous_mode) {
            if (!queue_pair_if_room(pio,
                                    config->sm,
                                    PIO_TIMER_OUTPUT_COMPARE_STOP_COMPARE_TICKS,
                                    0u)) {
                printf("TX FIFO full, could not queue stop command\n");
                continue;
            }
            printf("stop queued\n");
        } else if (ch == 'q' || ch == 'Q') {
            if (config->continuous_mode) {
                if (!queue_pair_if_room(pio,
                                        config->sm,
                                        PIO_TIMER_OUTPUT_COMPARE_STOP_COMPARE_TICKS,
                                        0u)) {
                    printf("TX FIFO full, stop not queued on exit\n");
                }
            }
            printf("Leaving output compare validation\n");
            break;
        }
    }
}
