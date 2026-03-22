#ifndef AD9850_DRIVER_H
#define AD9850_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"

// AD9850 serial frame, transmitted LSB byte first.
// bytes[0..3]: FTW least-significant to most-significant byte.
// bytes[4]: control byte (phase and power-down bits).
typedef struct {
    uint8_t bytes[5];
} ad9850_frame_t;

// Static configuration for one AD9850 hardware-SPI driver instance.
typedef struct {
    // RP2040 SPI peripheral instance (spi0 or spi1).
    spi_inst_t *spi;
    // SPI baud rate used for 40-bit AD9850 frame writes.
    uint32_t spi_baud_hz;

    // SPI pins used for AD9850 serial clock and data.
    uint sck_pin;
    uint mosi_pin;

    // Optional FQ_UD latch pin.
    bool use_fqud_pin;
    uint fqud_pin;

    // Optional hardware reset pin.
    bool use_reset_pin;
    uint reset_pin;

    // DDS reference/system clock in Hz (used for Hz->FTW conversion).
    uint32_t dds_sysclk_hz;
} ad9850_driver_config_t;

// Runtime state for one initialized driver instance.
typedef struct {
    bool initialized;
    // Write/apply operations are locked until serial-enable sequence succeeds.
    bool serial_enabled;
    spi_inst_t *spi;
    uint32_t dds_sysclk_hz;

    // SCK pin is reused to generate a manual W_CLK pulse during serial-enable.
    uint sck_pin;

    bool use_fqud_pin;
    uint fqud_pin;

    bool use_reset_pin;
    uint reset_pin;

    // Non-blocking runtime transfer state.
    bool tx_active;
    bool tx_pending_pulse_fqud;
    bool tx_last_success;
    bool tx_result_ready;
    uint8_t tx_next_index;
    ad9850_frame_t tx_frame_shadow;
} ad9850_driver_t;

// Initializes hardware SPI and optional control pins.
// Returns false on invalid parameters.
bool ad9850_driver_init(ad9850_driver_t *driver,
                        const ad9850_driver_config_t *config);

// Deinitializes SPI peripheral state for this driver instance.
void ad9850_driver_deinit(ad9850_driver_t *driver);

// Builds one AD9850 frame from FTW + control fields.
// phase must be in [0..31].
bool ad9850_driver_make_frame(uint32_t ftw,
                              uint8_t phase,
                              bool power_down,
                              ad9850_frame_t *frame_out);

// Converts frequency in Hz to FTW using configured dds_sysclk_hz.
// FTW = floor((frequency_hz * 2^32) / dds_sysclk_hz)
bool ad9850_driver_frequency_hz_to_ftw(const ad9850_driver_t *driver,
                                       uint32_t frequency_hz,
                                       uint32_t *ftw_out);

// Writes one 40-bit frame over hardware SPI (blocking).
// Guarded: returns false until ad9850_driver_serial_enable(...) has run.
bool ad9850_driver_write_frame_blocking(const ad9850_driver_t *driver,
                                        const ad9850_frame_t *frame);

// Pulses FQ_UD if use_fqud_pin is enabled.
bool ad9850_driver_pulse_fqud(const ad9850_driver_t *driver);

// Performs AD9850 serial interface enable sequence:
// 1) pulse RST high (if configured)
// 2) pulse W_CLK high (on configured SCK pin)
// 3) pulse FQ_UD high (if configured)
// On success, driver write path is unlocked.
bool ad9850_driver_serial_enable(ad9850_driver_t *driver);

// Writes frame and optionally pulses FQ_UD.
// Guarded through ad9850_driver_write_frame_blocking(...).
bool ad9850_driver_apply_frame_blocking(const ad9850_driver_t *driver,
                                        const ad9850_frame_t *frame,
                                        bool pulse_fqud);

// Starts non-blocking frame apply operation for runtime ISR-driven orchestration.
// Returns false if driver is not ready, arguments are invalid, or a previous
// transfer is still active.
bool ad9850_driver_start_apply_nonblocking(ad9850_driver_t *driver,
                                           const ad9850_frame_t *frame,
                                           bool pulse_fqud);

// Services a pending non-blocking transfer state machine.
// Intended for IRQ-driven call sites.
void ad9850_driver_service_nonblocking(ad9850_driver_t *driver);

// Reports whether a non-blocking transfer is currently in progress.
bool ad9850_driver_is_nonblocking_busy(const ad9850_driver_t *driver);

// Returns completion status of most recent non-blocking transfer.
// Returns false when no completion has occurred since the last call.
bool ad9850_driver_take_nonblocking_result(ad9850_driver_t *driver,
                                           bool *success_out);

// Pulses optional reset pin if configured.
// Reset clears serial-enabled state; serial-enable must be re-run before writes.
bool ad9850_driver_reset(ad9850_driver_t *driver);

#endif
