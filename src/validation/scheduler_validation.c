#include <stdio.h>
#include <stdlib.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "src/scheduler/scheduler.h"
#include "src/validation/scheduler_validation.h"

#define SCHEDULER_OUTPUT_SCALE 5u

static PIO resolve_pio(uint pio_index)
{
    return pio_index == 1 ? pio1 : pio0;
}

static spi_inst_t *resolve_spi(uint spi_index)
{
    return spi_index == 1 ? spi1 : spi0;
}

static uint32_t us_to_ticks(uint32_t us,
                            uint32_t sm_clk_hz)
{
    uint64_t ticks = ((uint64_t) us * (uint64_t) sm_clk_hz) / 1000000ull;
    if (ticks == 0u) {
        return 1u;
    }
    if (ticks > 0xFFFFFFFFull) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t) ticks;
}

static uint32_t us_to_scheduler_request_ticks(uint32_t us,
                                              uint32_t sm_clk_hz)
{
    uint32_t raw_ticks = us_to_ticks(us, sm_clk_hz);
    uint32_t request_ticks = raw_ticks / SCHEDULER_OUTPUT_SCALE;
    if (request_ticks == 0u) {
        request_ticks = 1u;
    }
    return request_ticks;
}

static const char *state_name(scheduler_state_t state)
{
    switch (state) {
    case SCHEDULER_STATE_INIT:
        return "INIT";
    case SCHEDULER_STATE_IDLE:
        return "IDLE";
    case SCHEDULER_STATE_PREPARE_PRELOAD:
        return "PREPARE_PRELOAD";
    case SCHEDULER_STATE_ARM:
        return "ARM";
    case SCHEDULER_STATE_END_OK:
        return "END_OK";
    case SCHEDULER_STATE_END_FAULT:
        return "END_FAULT";
    default:
        return "UNKNOWN";
    }
}

static const char *error_name(scheduler_error_t error)
{
    switch (error) {
    case SCHEDULER_ERROR_NONE:
        return "NONE";
    case SCHEDULER_ERROR_INVALID_ARG:
        return "INVALID_ARG";
    case SCHEDULER_ERROR_NOT_READY:
        return "NOT_READY";
    case SCHEDULER_ERROR_SEQUENCE_OVERFLOW:
        return "SEQUENCE_OVERFLOW";
    case SCHEDULER_ERROR_ALARM_ENQUEUE:
        return "ALARM_ENQUEUE";
    case SCHEDULER_ERROR_OUTPUT_ENQUEUE:
        return "OUTPUT_ENQUEUE";
    case SCHEDULER_ERROR_AD9850:
        return "AD9850";
    case SCHEDULER_ERROR_TIMER_FAULT:
        return "TIMER_FAULT";
    case SCHEDULER_ERROR_UNEXPECTED_ALARM_TICK:
        return "UNEXPECTED_ALARM_TICK";
    default:
        return "UNKNOWN";
    }
}

static bool read_line(char *buffer,
                      size_t buffer_size)
{
    size_t index = 0;
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            putchar('\n');
            buffer[index] = '\0';
            return true;
        }

        if ((ch == 8 || ch == 127) && index > 0) {
            index--;
            printf("\b \b");
            continue;
        }

        if (ch >= 32 && ch <= 126 && index < (buffer_size - 1)) {
            buffer[index++] = (char) ch;
            putchar(ch);
        }
    }
}

static uint32_t prompt_u32(const char *label,
                           uint32_t default_value)
{
    char line[64];
    printf("%s [%lu]: ", label, (unsigned long) default_value);
    fflush(stdout);
    read_line(line, sizeof(line));
    if (line[0] == '\0') {
        return default_value;
    }

    char *end_ptr = NULL;
    unsigned long value = strtoul(line, &end_ptr, 10);
    if (end_ptr == line || *end_ptr != '\0') {
        printf("Invalid input, using default %lu\n", (unsigned long) default_value);
        return default_value;
    }

    return (uint32_t) value;
}

static int32_t prompt_i32(const char *label,
                          int32_t default_value)
{
    char line[64];
    printf("%s [%ld]: ", label, (long) default_value);
    fflush(stdout);
    read_line(line, sizeof(line));
    if (line[0] == '\0') {
        return default_value;
    }

    char *end_ptr = NULL;
    long value = strtol(line, &end_ptr, 10);
    if (end_ptr == line || *end_ptr != '\0') {
        printf("Invalid input, using default %ld\n", (long) default_value);
        return default_value;
    }

    return (int32_t) value;
}

static void prompt_common_config(scheduler_validation_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->output_compare_pio_index =
        prompt_u32("Output compare PIO index (0 or 1)", cfg->output_compare_pio_index);
    cfg->alarm_timer_pio_index =
        prompt_u32("Alarm timer PIO index (0 or 1)", cfg->alarm_timer_pio_index);
    cfg->output_compare_sm = prompt_u32("Output compare SM", cfg->output_compare_sm);
    cfg->alarm_timer_sm = prompt_u32("Alarm timer SM", cfg->alarm_timer_sm);
    cfg->pps_pin = prompt_u32("PPS pin", cfg->pps_pin);
    cfg->trigger_pin = prompt_u32("Trigger pin", cfg->trigger_pin);
    cfg->output_pin = prompt_u32("Output pin", cfg->output_pin);
    cfg->sm_clk_hz = prompt_u32("SM clock (Hz)", cfg->sm_clk_hz);
    cfg->output_pulse_us = prompt_u32("Output pulse (us)", cfg->output_pulse_us);

    cfg->ad9850_spi_index = prompt_u32("AD9850 SPI index (0 or 1)", cfg->ad9850_spi_index);
    cfg->ad9850_spi_baud_hz = prompt_u32("AD9850 SPI baud (Hz)", cfg->ad9850_spi_baud_hz);
    cfg->ad9850_sck_pin = prompt_u32("AD9850 SCK pin", cfg->ad9850_sck_pin);
    cfg->ad9850_mosi_pin = prompt_u32("AD9850 MOSI pin", cfg->ad9850_mosi_pin);
    cfg->ad9850_fqud_pin = prompt_u32("AD9850 FQ_UD pin", cfg->ad9850_fqud_pin);
    cfg->ad9850_reset_pin = prompt_u32("AD9850 reset pin", cfg->ad9850_reset_pin);
    cfg->ad9850_sysclk_hz = prompt_u32("AD9850 sysclk (Hz)", cfg->ad9850_sysclk_hz);

    cfg->dt0_us = prompt_u32("dt0 (us)", cfg->dt0_us);
    cfg->symbol_count = prompt_u32("symbol_count", cfg->symbol_count);
    cfg->dts_us = prompt_u32("dts (us, all symbols)", cfg->dts_us);
    cfg->load_offset_us = prompt_i32("load_offset (us)", cfg->load_offset_us);
}

static void fill_prepare_request(const scheduler_validation_config_t *cfg,
                                 scheduler_prepare_request_t *request,
                                 uint32_t *dts_ticks,
                                 uint32_t *freq_hz)
{
    static const uint32_t defaults[8] = {1500u, 900u, 1800u, 600u, 2400u, 2100u, 1200u, 0u};

    uint32_t symbol_count = cfg->symbol_count == 0u ? 8u : cfg->symbol_count;
    if (symbol_count > 8u) {
        symbol_count = 8u;
    }

    for (uint32_t i = 0u; i < symbol_count; ++i) {
        dts_ticks[i] = us_to_scheduler_request_ticks(cfg->dts_us, cfg->sm_clk_hz);
        freq_hz[i] = defaults[i];
    }

    request->symbol_count = symbol_count;
    request->dt0 = us_to_scheduler_request_ticks(cfg->dt0_us, cfg->sm_clk_hz);
    request->dts = dts_ticks;
    request->load_offset = (int32_t) us_to_scheduler_request_ticks(
        (uint32_t) (cfg->load_offset_us < 0 ? -cfg->load_offset_us : cfg->load_offset_us),
        cfg->sm_clk_hz);
    if (cfg->load_offset_us < 0) {
        request->load_offset = -request->load_offset;
    }
    request->ftw_frames = NULL;
    request->freq_hz = freq_hz;
}

static bool init_scheduler(const scheduler_validation_config_t *cfg,
                           scheduler_t *scheduler)
{
    scheduler_config_t scheduler_cfg = {
        .output_compare_pio = resolve_pio(cfg->output_compare_pio_index),
        .alarm_timer_pio = resolve_pio(cfg->alarm_timer_pio_index),
        .output_compare_sm = cfg->output_compare_sm,
        .alarm_timer_sm = cfg->alarm_timer_sm,
        .trigger_pin = cfg->trigger_pin,
        .output_pin = cfg->output_pin,
        .pps_pin = cfg->pps_pin,
        .sm_clk_hz = cfg->sm_clk_hz,
        .output_pulse_ticks = us_to_ticks(cfg->output_pulse_us, cfg->sm_clk_hz),
        .ad9850_spi = resolve_spi(cfg->ad9850_spi_index),
        .ad9850_spi_baud_hz = cfg->ad9850_spi_baud_hz,
        .ad9850_sck_pin = cfg->ad9850_sck_pin,
        .ad9850_mosi_pin = cfg->ad9850_mosi_pin,
        .ad9850_use_fqud_pin = cfg->ad9850_use_fqud_pin,
        .ad9850_fqud_pin = cfg->ad9850_fqud_pin,
        .ad9850_use_reset_pin = cfg->ad9850_use_reset_pin,
        .ad9850_reset_pin = cfg->ad9850_reset_pin,
        .ad9850_sysclk_hz = cfg->ad9850_sysclk_hz,
    };

    return scheduler_init(scheduler, &scheduler_cfg);
}

static void print_scheduler_report(const scheduler_t *scheduler,
                                   const char *label)
{
    if (scheduler == NULL) {
        return;
    }

    printf("[%s] state=%s error=%s prepared=%u symbol_count=%lu next_alarm=%lu next_write=%lu alarms=%lu rearm_ack=%lu out_feed=%lu alarm_feed=%lu\n",
           label,
           state_name(scheduler_get_state(scheduler)),
           error_name(scheduler_get_last_error(scheduler)),
           scheduler->prepared ? 1u : 0u,
           (unsigned long) scheduler->symbol_count,
           (unsigned long) scheduler->next_alarm_index,
           (unsigned long) scheduler->next_write_symbol,
           (unsigned long) scheduler->alarm_fired_count,
           (unsigned long) scheduler->rearm_ack_count,
           (unsigned long) scheduler->output_feed_count,
           (unsigned long) scheduler->alarm_feed_count);
}

static void print_report_legend(void)
{
    printf("fields: state=state_machine error=last_error prepared=prepare_done symbol_count=burst_symbols next_alarm=next_expected_alarm_idx next_write=next_ad9850_write_idx alarms=alarm_fired_count rearm_ack=rearm_ack_count out_feed=output_words_fed alarm_feed=alarm_words_fed\n");
}

static void run_sequence_building(scheduler_validation_config_t *cfg)
{
    prompt_common_config(cfg);

    scheduler_t scheduler;
    if (!init_scheduler(cfg, &scheduler)) {
        printf("init failed\n");
        return;
    }

    scheduler_prepare_request_t request;
    uint32_t dts_ticks[8];
    uint32_t freq_hz[8];
    fill_prepare_request(cfg, &request, dts_ticks, freq_hz);

    bool ok = scheduler_prepare(&scheduler, &request);
    printf("prepare=%s\n", ok ? "ok" : "failed");
    print_scheduler_report(&scheduler, "sequence");

    if (ok) {
        printf("output_compare_sequence: ");
        for (uint32_t i = 0u; i < request.symbol_count; ++i) {
            printf("%lu ", (unsigned long) scheduler.output_compare_sequence[i]);
        }
        printf("\n");

        printf("alarm_timer_sequence: ");
        for (uint32_t i = 0u; i < (request.symbol_count + 1u); ++i) {
            printf("%lu ", (unsigned long) scheduler.alarm_timer_sequence[i]);
        }
        printf("\n");
    }
}

static void run_initialization(scheduler_validation_config_t *cfg)
{
    prompt_common_config(cfg);

    scheduler_t scheduler;
    bool ok = init_scheduler(cfg, &scheduler);
    printf("init=%s\n", ok ? "ok" : "failed");
    if (ok) {
        print_scheduler_report(&scheduler, "init");
    }
}

static void run_prepare_preload(scheduler_validation_config_t *cfg)
{
    prompt_common_config(cfg);

    scheduler_t scheduler;
    if (!init_scheduler(cfg, &scheduler)) {
        printf("init failed\n");
        return;
    }

    scheduler_prepare_request_t request;
    uint32_t dts_ticks[8];
    uint32_t freq_hz[8];
    fill_prepare_request(cfg, &request, dts_ticks, freq_hz);

    bool ok = scheduler_prepare(&scheduler, &request);
    printf("prepare=%s\n", ok ? "ok" : "failed");
    print_scheduler_report(&scheduler, "prepare");

    if (ok) {
        scheduler.state = SCHEDULER_STATE_END_OK;
        bool reset_ok = scheduler_reset(&scheduler);
        printf("cleanup_reset=%s\n", reset_ok ? "ok" : "failed");
        print_scheduler_report(&scheduler, "after_cleanup");
    }
}

static void run_timer_orchestration(scheduler_validation_config_t *cfg)
{
    prompt_common_config(cfg);

    scheduler_t scheduler;
    if (!init_scheduler(cfg, &scheduler)) {
        printf("init failed\n");
        return;
    }

    scheduler_prepare_request_t request;
    uint32_t dts_ticks[8];
    uint32_t freq_hz[8];
    fill_prepare_request(cfg, &request, dts_ticks, freq_hz);

    if (!scheduler_prepare(&scheduler, &request)) {
        printf("prepare failed\n");
        print_scheduler_report(&scheduler, "prepare_fail");
        return;
    }

    print_report_legend();

    printf("timer orchestration ready. command: a=arm q=return\n");
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == 'a' || ch == 'A') {
            bool arm_ok = scheduler_arm(&scheduler);
            printf("arm=%s\n", arm_ok ? "ok" : "failed");
            if (!arm_ok) {
                print_scheduler_report(&scheduler, "arm_fail");
                continue;
            }

            while (scheduler_get_state(&scheduler) == SCHEDULER_STATE_ARM) {
                tight_loop_contents();
            }

            print_scheduler_report(&scheduler, "end");
            if (scheduler_get_state(&scheduler) == SCHEDULER_STATE_END_OK) {
                bool reset_ok = scheduler_reset(&scheduler);
                printf("cleanup_reset=%s\n", reset_ok ? "ok" : "failed");
                print_scheduler_report(&scheduler, "cleanup");
            }
            return;
        }

        if (ch == 'q' || ch == 'Q') {
            printf("timer orchestration aborted\n");
            return;
        }
    }
}

static void run_full_integration(scheduler_validation_config_t *cfg)
{
    prompt_common_config(cfg);

    scheduler_t scheduler;
    bool initialized = false;

    print_report_legend();
    printf("full integration commands: i=init p=prepare a=arm s=status q=return\n");
    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == 'i' || ch == 'I') {
            initialized = init_scheduler(cfg, &scheduler);
            printf("init=%s\n", initialized ? "ok" : "failed");
            if (initialized) {
                print_scheduler_report(&scheduler, "init");
            }
        } else if (ch == 'p' || ch == 'P') {
            if (!initialized) {
                printf("init first\n");
                continue;
            }

            scheduler_prepare_request_t request;
            uint32_t dts_ticks[8];
            uint32_t freq_hz[8];
            fill_prepare_request(cfg, &request, dts_ticks, freq_hz);

            bool ok = scheduler_prepare(&scheduler, &request);
            printf("prepare=%s\n", ok ? "ok" : "failed");
            print_scheduler_report(&scheduler, "prepare");
        } else if (ch == 'a' || ch == 'A') {
            if (!initialized) {
                printf("init first\n");
                continue;
            }

            bool ok = scheduler_arm(&scheduler);
            printf("arm=%s\n", ok ? "ok" : "failed");
            if (!ok) {
                print_scheduler_report(&scheduler, "arm_fail");
                continue;
            }

            while (scheduler_get_state(&scheduler) == SCHEDULER_STATE_ARM) {
                tight_loop_contents();
            }

            print_scheduler_report(&scheduler, "end");
            if (scheduler_get_state(&scheduler) == SCHEDULER_STATE_END_OK) {
                scheduler.state = SCHEDULER_STATE_END_OK;
                (void) scheduler_reset(&scheduler);
                print_scheduler_report(&scheduler, "idle_after_end");
            }
        } else if (ch == 's' || ch == 'S') {
            if (initialized) {
                print_scheduler_report(&scheduler, "status");
            } else {
                printf("status: not initialized\n");
            }
        } else if (ch == 'q' || ch == 'Q') {
            printf("Leaving scheduler validation\n");
            return;
        }
    }
}

void scheduler_validation_run(const scheduler_validation_config_t *config)
{
    if (config == NULL) {
        printf("scheduler_validation: null config\n");
        return;
    }

    scheduler_validation_config_t cfg = *config;

    printf("\nScheduler validation submenu\n");
    while (true) {
        printf("a) Sequence building\n");
        printf("b) Initialization\n");
        printf("c) Prepare/preload\n");
        printf("d) Timer orchestration\n");
        printf("e) Full integration\n");
        printf("q) Return\n");
        printf("Select: ");

        char line[16];
        read_line(line, sizeof(line));

        if (line[0] == 'a' || line[0] == 'A') {
            run_sequence_building(&cfg);
        } else if (line[0] == 'b' || line[0] == 'B') {
            run_initialization(&cfg);
        } else if (line[0] == 'c' || line[0] == 'C') {
            run_prepare_preload(&cfg);
        } else if (line[0] == 'd' || line[0] == 'D') {
            run_timer_orchestration(&cfg);
        } else if (line[0] == 'e' || line[0] == 'E') {
            run_full_integration(&cfg);
        } else if (line[0] == 'q' || line[0] == 'Q') {
            break;
        } else {
            printf("Unknown selection\n");
        }

        printf("\n");
    }
}
