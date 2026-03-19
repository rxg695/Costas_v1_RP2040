#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"

#include "hardware/clocks.h"
#include "pio_timer_output_compare.pio.h"

void pio_timer_output_compare_init(PIO pio,
                                   uint sm,
                                   uint offset,
                                   uint trigger_pin,
                                   uint output_pin,
                                   float sm_clk_hz)
{
    // Prepare trigger pin for PIO input sampling.
    pio_gpio_init(pio, trigger_pin);
    gpio_set_dir(trigger_pin, GPIO_IN);

    // Prepare output pin for PIO control.
    pio_gpio_init(pio, output_pin);
    gpio_set_dir(output_pin, GPIO_OUT);
    pio_sm_set_consecutive_pindirs(pio, sm, output_pin, 1, true);

    // Build SM config for output-compare program.
    pio_sm_config config = pio_timer_output_compare_program_get_default_config(offset);

    // SET instructions target the selected output pin.
    sm_config_set_set_pins(&config, output_pin, 1);

    // WAIT PIN 0 samples this configured trigger pin.
    sm_config_set_in_pins(&config, trigger_pin);

    // Timing base: SM runs at requested clock.
    float clkdiv = (float) clock_get_hz(clk_sys) / sm_clk_hz;
    sm_config_set_clkdiv(&config, clkdiv);

    // Initialize and force idle low state.
    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_pins_with_mask(pio, sm, 0u, 1u << output_pin);

    // Start the state machine.
    pio_sm_set_enabled(pio, sm, true);
}

void pio_timer_output_compare_arm(PIO pio,
                                  uint sm,
                                  uint32_t compare_ticks,
                                  uint32_t pulse_ticks)
{
    // Load compare start delay for next trigger event.
    pio_sm_put_blocking(pio, sm, compare_ticks);
    // Load pulse high duration for next trigger event.
    pio_sm_put_blocking(pio, sm, pulse_ticks);
}

uint32_t pio_timer_output_compare_ns_to_ticks(uint32_t sm_clk_hz,
                                              uint64_t duration_ns)
{
    return (uint32_t) ((duration_ns * (uint64_t) sm_clk_hz) / 1000000000ull);
}
