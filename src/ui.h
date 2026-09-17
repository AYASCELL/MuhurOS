#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODAL_NONE = 0,
    MODAL_TUTANAK,
    MODAL_PIN,
    MODAL_SETTINGS
} MODAL_TYPE;

typedef struct {
    uint32_t days;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    int progress_percent;
    int auto_shutdown_remaining;
    bool is_paused;
    MODAL_TYPE active_modal;
    int selected_button; // 0: Tutanak, 1: Shutdown
    char pin_buffer[8];
    int pin_len;
    bool pin_error;
    int settings_field; // 0: end_year, 1: end_month, 2: end_day, 3: auto_shutdown_seconds
    bool settings_saved;
} UI_STATE;

extern UI_STATE g_ui;

void ui_init(void);
void ui_render_main(void);
void ui_render_tutanak_modal(void);
void ui_render_pin_modal(void);
void ui_render_settings_modal(void);
void ui_render_shutdown_screen(void);

#endif
