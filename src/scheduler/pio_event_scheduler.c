#include "src/scheduler/pio_event_scheduler.h"

#include "driver/pio_timer_output_compare/pio_timer_output_compare.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

static pio_event_scheduler_t *scheduler_registry[2][4] = {0};
static pio_event_scheduler_t *output_irq_registry[32] = {0};
static bool irq_handler_installed[2] = {false, false};
static bool gpio_callback_installed = false;

static inline bool tx_has_room_for_words(PIO pio,
                                         uint sm,
                                         uint words)
{
    uint tx_level = pio_sm_get_tx_fifo_level(pio, sm);
    return tx_level <= (PIO_TIMER_OUTPUT_COMPARE_TX_FIFO_WORDS - words);
}

static enum pio_interrupt_source tx_not_full_source_for_sm(uint sm)
{
    switch (sm) {
    case 0:
        return pis_sm0_tx_fifo_not_full;
    case 1:
        return pis_sm1_tx_fifo_not_full;
    case 2:
        return pis_sm2_tx_fifo_not_full;
    default:
        return pis_sm3_tx_fifo_not_full;
    }
}

static int pio_index_from_instance(PIO pio)
{
    return pio == pio1 ? 1 : 0;
}

static void scheduler_pump(pio_event_scheduler_t *scheduler)
{
    if (!scheduler->active) {
        return;
    }

    while (scheduler->next_event_to_queue < scheduler->event_count &&
           tx_has_room_for_words(scheduler->pio,
                                 scheduler->sm,
                                 PIO_TIMER_OUTPUT_COMPARE_WORDS_PER_EVENT)) {
        const pio_event_scheduler_event_t *event =
            &scheduler->events[scheduler->next_event_to_queue];
        pio_sm_put(scheduler->pio, scheduler->sm, event->compare_ticks);
        pio_sm_put(scheduler->pio, scheduler->sm, event->pulse_ticks);
        scheduler->next_event_to_queue++;
    }

    if (!scheduler->stop_queued &&
        scheduler->next_event_to_queue >= scheduler->event_count &&
        tx_has_room_for_words(scheduler->pio,
                              scheduler->sm,
                              PIO_TIMER_OUTPUT_COMPARE_WORDS_PER_EVENT)) {
        pio_sm_put(scheduler->pio,
                   scheduler->sm,
                   PIO_TIMER_OUTPUT_COMPARE_STOP_COMPARE_TICKS);
        pio_sm_put(scheduler->pio, scheduler->sm, 0u);
        scheduler->stop_queued = true;
    }

    uint tx_level_after = pio_sm_get_tx_fifo_level(scheduler->pio, scheduler->sm);

    if (scheduler->stop_queued && tx_level_after == 0u) {
        enum pio_interrupt_source source = tx_not_full_source_for_sm(scheduler->sm);
        pio_set_irq0_source_enabled(scheduler->pio, source, false);
        scheduler->active = false;
    }
}

static void pio_irq0_dispatch(PIO pio)
{
    int pio_index = pio_index_from_instance(pio);
    for (uint sm = 0; sm < 4; ++sm) {
        pio_event_scheduler_t *scheduler = scheduler_registry[pio_index][sm];
        if (scheduler != NULL && scheduler->active) {
            scheduler_pump(scheduler);
        }
    }
}

static void pio0_irq0_handler(void)
{
    pio_irq0_dispatch(pio0);
}

static void pio1_irq0_handler(void)
{
    pio_irq0_dispatch(pio1);
}

static void ensure_irq_handler_installed(PIO pio)
{
    int pio_index = pio_index_from_instance(pio);
    if (irq_handler_installed[pio_index]) {
        return;
    }

    if (pio_index == 0) {
        irq_set_exclusive_handler(PIO0_IRQ_0, pio0_irq0_handler);
        irq_set_enabled(PIO0_IRQ_0, true);
    } else {
        irq_set_exclusive_handler(PIO1_IRQ_0, pio1_irq0_handler);
        irq_set_enabled(PIO1_IRQ_0, true);
    }

    irq_handler_installed[pio_index] = true;
}

static void output_pin_irq_callback(uint gpio, uint32_t events)
{
    if ((events & GPIO_IRQ_EDGE_RISE) == 0u || gpio >= 32u) {
        return;
    }

    pio_event_scheduler_t *scheduler = output_irq_registry[gpio];
    if (scheduler != NULL && scheduler->output_assert_callback != NULL) {
        scheduler->output_assert_callback(scheduler->output_assert_user_data);
    }
}

void pio_event_scheduler_init(pio_event_scheduler_t *scheduler,
                              PIO pio,
                              uint sm)
{
    scheduler->pio = pio;
    scheduler->sm = sm;
    scheduler->events = NULL;
    scheduler->event_count = 0;
    scheduler->next_event_to_queue = 0;
    scheduler->stop_queued = false;
    scheduler->active = false;
    scheduler->output_assert_irq_enabled = false;
    scheduler->output_assert_pin = 0u;
    scheduler->output_assert_callback = NULL;
    scheduler->output_assert_user_data = NULL;

    ensure_irq_handler_installed(pio);
    scheduler_registry[pio_index_from_instance(pio)][sm] = scheduler;
}

void pio_event_scheduler_start(pio_event_scheduler_t *scheduler,
                               const pio_event_scheduler_event_t *events,
                               size_t event_count)
{
    scheduler->events = events;
    scheduler->event_count = event_count;
    scheduler->next_event_to_queue = 0;
    scheduler->stop_queued = false;
    scheduler->active = true;

    enum pio_interrupt_source source = tx_not_full_source_for_sm(scheduler->sm);
    pio_set_irq0_source_enabled(scheduler->pio, source, true);

    // Prime immediately so startup does not wait for first IRQ service point.
    scheduler_pump(scheduler);
}

void pio_event_scheduler_poll(pio_event_scheduler_t *scheduler)
{
    scheduler_pump(scheduler);
}

void pio_event_scheduler_set_output_assert_callback(
    pio_event_scheduler_t *scheduler,
    uint output_pin,
    pio_event_scheduler_output_assert_callback_t callback,
    void *user_data)
{
    if (!gpio_callback_installed) {
        gpio_set_irq_callback(&output_pin_irq_callback);
        irq_set_enabled(IO_IRQ_BANK0, true);
        gpio_callback_installed = true;
    }

    scheduler->output_assert_pin = output_pin;
    scheduler->output_assert_callback = callback;
    scheduler->output_assert_user_data = user_data;
    scheduler->output_assert_irq_enabled = true;

    output_irq_registry[output_pin] = scheduler;
    gpio_set_irq_enabled(output_pin, GPIO_IRQ_EDGE_RISE, true);
}

void pio_event_scheduler_clear_output_assert_callback(pio_event_scheduler_t *scheduler)
{
    if (!scheduler->output_assert_irq_enabled) {
        return;
    }

    gpio_set_irq_enabled(scheduler->output_assert_pin, GPIO_IRQ_EDGE_RISE, false);
    output_irq_registry[scheduler->output_assert_pin] = NULL;
    scheduler->output_assert_irq_enabled = false;
    scheduler->output_assert_callback = NULL;
    scheduler->output_assert_user_data = NULL;
}

void pio_event_scheduler_request_stop(pio_event_scheduler_t *scheduler)
{
    if (!scheduler->active) {
        return;
    }

    // Trim unqueued tail; stop command will be queued next pump/IRQ.
    scheduler->event_count = scheduler->next_event_to_queue;
}

bool pio_event_scheduler_is_idle(const pio_event_scheduler_t *scheduler)
{
    return !scheduler->active;
}
