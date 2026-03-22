#include <stdio.h>

#include "driver/ad9850_driver/ad9850_driver.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "src/validation/ad9850_validation.h"

static uint32_t ftw_from_freq(uint32_t frequency_hz,
                              uint32_t dds_sysclk_hz)
{
    uint64_t numerator = ((uint64_t) frequency_hz) << 32;
    return (uint32_t) (numerator / (uint64_t) dds_sysclk_hz);
}

static void print_byte_binary(uint8_t value)
{
    for (int bit = 7; bit >= 0; --bit) {
        putchar((value & (1u << bit)) ? '1' : '0');
    }
}

static void print_frame_spi_order_binary(const ad9850_frame_t *frame)
{
    if (frame == NULL) {
        return;
    }

    printf("spi_bytes_bin b0=");
    print_byte_binary(frame->bytes[0]);
    printf(" b1=");
    print_byte_binary(frame->bytes[1]);
    printf(" b2=");
    print_byte_binary(frame->bytes[2]);
    printf(" b3=");
    print_byte_binary(frame->bytes[3]);
    printf(" b4=");
    print_byte_binary(frame->bytes[4]);
    printf("\n");
}

static void print_status(uint32_t frequency_hz,
                         uint32_t step_hz,
                         uint8_t phase,
                         bool power_down,
                         uint32_t ftw,
                                 const ad9850_frame_t *frame,
                                 bool serial_enabled,
                                 bool nb_busy)
{
     printf("ad9850_status freq=%lu step=%lu phase=%u power_down=%u serial_enabled=%u nb_busy=%u ftw=0x%08lx\n",
           (unsigned long) frequency_hz,
           (unsigned long) step_hz,
           (unsigned) phase,
           power_down ? 1u : 0u,
              serial_enabled ? 1u : 0u,
              nb_busy ? 1u : 0u,
           (unsigned long) ftw);

    if (frame != NULL) {
        print_frame_spi_order_binary(frame);
    }
}

void ad9850_validation_run(const ad9850_validation_config_t *config)
{
    if (config == NULL) {
        printf("ad9850_validation: null config\n");
        return;
    }

    uint32_t dds_sysclk_hz = config->dds_sysclk_hz == 0u ? 125000000u : config->dds_sysclk_hz;
    uint32_t spi_baud_hz = config->spi_baud_hz == 0u ? 1000000u : config->spi_baud_hz;
    uint8_t phase = (uint8_t) (config->phase & 0x1Fu);
    uint32_t frequency_hz = config->frequency_hz == 0u ? 1000000u : config->frequency_hz;
    uint32_t step_hz = 1000u;
    bool power_down = config->power_down;

    spi_inst_t *spi = config->spi_index == 1u ? spi1 : spi0;

    ad9850_driver_t driver;
    ad9850_driver_config_t driver_cfg = {
        .spi = spi,
        .spi_baud_hz = spi_baud_hz,
        .sck_pin = config->sck_pin,
        .mosi_pin = config->mosi_pin,
        .use_fqud_pin = config->use_fqud_pin,
        .fqud_pin = config->fqud_pin,
        .use_reset_pin = config->use_reset_pin,
        .reset_pin = config->reset_pin,
        .dds_sysclk_hz = dds_sysclk_hz,
    };

    if (!ad9850_driver_init(&driver, &driver_cfg)) {
        printf("ad9850 init failed\n");
        return;
    }

    uint32_t ftw = ftw_from_freq(frequency_hz, dds_sysclk_hz);
    ad9850_frame_t frame;
    bool frame_ok = ad9850_driver_make_frame(ftw, phase, power_down, &frame);
    uint32_t nb_start_ok_count = 0u;
    uint32_t nb_start_busy_count = 0u;
    uint32_t nb_start_fail_count = 0u;
    uint32_t nb_complete_ok_count = 0u;
    uint32_t nb_complete_fail_count = 0u;

    printf("\nAD9850 validation active\n");
    printf("spi=%s baud=%lu sck=GP%u mosi=GP%u fqud=%s(GP%u) reset=%s(GP%u) sysclk=%lu\n",
           spi == spi0 ? "spi0" : "spi1",
           (unsigned long) spi_baud_hz,
           config->sck_pin,
           config->mosi_pin,
           config->use_fqud_pin ? "on" : "off",
           config->fqud_pin,
           config->use_reset_pin ? "on" : "off",
           config->reset_pin,
           (unsigned long) dds_sysclk_hz);
    printf("commands: a=apply w=write p=pulse_fqud r=reset e=serial_enable n=nb_apply m=nb_apply_latch v=nb_service b=nb_busy x=nb_stats +=freq_up -=freq_down d=toggle_pd s=status q=return\n");

    if (!frame_ok) {
        printf("initial frame build failed\n");
    } else {
        print_status(frequency_hz,
                     step_hz,
                     phase,
                     power_down,
                     ftw,
                     &frame,
                     driver.serial_enabled,
                     ad9850_driver_is_nonblocking_busy(&driver));
    }

    while (true) {
        ad9850_driver_service_nonblocking(&driver);
        bool nb_success = false;
        if (ad9850_driver_take_nonblocking_result(&driver, &nb_success)) {
            if (nb_success) {
                nb_complete_ok_count++;
            } else {
                nb_complete_fail_count++;
            }
            printf("nb_complete=%s\n", nb_success ? "ok" : "failed");
        }

        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            tight_loop_contents();
            continue;
        }

        if (ch == 'a' || ch == 'A') {
            bool ok = frame_ok && ad9850_driver_apply_frame_blocking(&driver, &frame, config->use_fqud_pin);
            printf("apply=%s\n", ok ? "ok" : "failed");
        } else if (ch == 'w' || ch == 'W') {
            bool ok = frame_ok && ad9850_driver_write_frame_blocking(&driver, &frame);
            printf("write=%s\n", ok ? "ok" : "failed");
        } else if (ch == 'p' || ch == 'P') {
            bool ok = ad9850_driver_pulse_fqud(&driver);
            printf("pulse_fqud=%s\n", ok ? "ok" : "failed_or_disabled");
        } else if (ch == 'r' || ch == 'R') {
            bool ok = ad9850_driver_reset(&driver);
            printf("reset=%s\n", ok ? "ok" : "failed_or_disabled");
        } else if (ch == 'e' || ch == 'E') {
            bool ok = ad9850_driver_serial_enable(&driver);
            printf("serial_enable=%s\n", ok ? "ok" : "failed");
        } else if (ch == 'n' || ch == 'N') {
            bool ok = frame_ok && ad9850_driver_start_apply_nonblocking(&driver, &frame, false);
            if (ok) {
                nb_start_ok_count++;
            } else if (ad9850_driver_is_nonblocking_busy(&driver)) {
                nb_start_busy_count++;
            } else {
                nb_start_fail_count++;
            }
            printf("nb_apply=%s\n", ok ? "started" : (ad9850_driver_is_nonblocking_busy(&driver) ? "busy" : "failed"));
        } else if (ch == 'm' || ch == 'M') {
            bool ok = frame_ok && ad9850_driver_start_apply_nonblocking(&driver, &frame, config->use_fqud_pin);
            if (ok) {
                nb_start_ok_count++;
            } else if (ad9850_driver_is_nonblocking_busy(&driver)) {
                nb_start_busy_count++;
            } else {
                nb_start_fail_count++;
            }
            printf("nb_apply_latch=%s\n", ok ? "started" : (ad9850_driver_is_nonblocking_busy(&driver) ? "busy" : "failed"));
        } else if (ch == 'v' || ch == 'V') {
            ad9850_driver_service_nonblocking(&driver);
            printf("nb_service\n");
        } else if (ch == 'b' || ch == 'B') {
            printf("nb_busy=%u\n", ad9850_driver_is_nonblocking_busy(&driver) ? 1u : 0u);
        } else if (ch == 'x' || ch == 'X') {
            printf("nb_stats start_ok=%lu start_busy=%lu start_fail=%lu complete_ok=%lu complete_fail=%lu\n",
                   (unsigned long) nb_start_ok_count,
                   (unsigned long) nb_start_busy_count,
                   (unsigned long) nb_start_fail_count,
                   (unsigned long) nb_complete_ok_count,
                   (unsigned long) nb_complete_fail_count);
        } else if (ch == '+') {
            frequency_hz += step_hz;
            ftw = ftw_from_freq(frequency_hz, dds_sysclk_hz);
            frame_ok = ad9850_driver_make_frame(ftw, phase, power_down, &frame);
            print_status(frequency_hz,
                         step_hz,
                         phase,
                         power_down,
                         ftw,
                         &frame,
                         driver.serial_enabled,
                         ad9850_driver_is_nonblocking_busy(&driver));
        } else if (ch == '-') {
            frequency_hz = frequency_hz > step_hz ? (frequency_hz - step_hz) : 1u;
            ftw = ftw_from_freq(frequency_hz, dds_sysclk_hz);
            frame_ok = ad9850_driver_make_frame(ftw, phase, power_down, &frame);
            print_status(frequency_hz,
                         step_hz,
                         phase,
                         power_down,
                         ftw,
                         &frame,
                         driver.serial_enabled,
                         ad9850_driver_is_nonblocking_busy(&driver));
        } else if (ch == 'd' || ch == 'D') {
            power_down = !power_down;
            frame_ok = ad9850_driver_make_frame(ftw, phase, power_down, &frame);
            print_status(frequency_hz,
                         step_hz,
                         phase,
                         power_down,
                         ftw,
                         &frame,
                         driver.serial_enabled,
                         ad9850_driver_is_nonblocking_busy(&driver));
        } else if (ch == 's' || ch == 'S') {
            print_status(frequency_hz,
                         step_hz,
                         phase,
                         power_down,
                         ftw,
                         &frame,
                         driver.serial_enabled,
                         ad9850_driver_is_nonblocking_busy(&driver));
        } else if (ch == 'q' || ch == 'Q') {
            printf("Leaving AD9850 validation\n");
            break;
        }
    }

    ad9850_driver_deinit(&driver);
}
