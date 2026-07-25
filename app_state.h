#ifndef APP_STATE_H
#define APP_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool splash_enable;
    float splash_angle;
    float splash_xpos;
    float splash_ypos;
    float splash_size;
    float splash_bounce_speed;
    float splash_bounce_size;
    char splash_text[256];
    uint32_t splash_color;
    uint32_t splash_color_bg;
} splash;

// SAVED
int get_keybind_event();
void set_keybind_event(int);

uint64_t get_click_delay();
void set_click_delay(uint64_t);

int get_click_limit();
void set_click_limit(int);

bool get_keybind_hold();
void set_keybind_hold(bool);

int get_click_button();
void set_click_button(int);

char *get_keybind_device();
void set_keybind_device(char *);

splash *get_splash_params();
void reset_splash_params();

// TEMPORARY
bool get_running();
void set_running(bool);

bool get_clicking();
void set_clicking(bool);

bool get_want_read_key();
void set_want_read_key(bool);

bool get_want_save_config();
void set_want_save_config(bool);

#ifdef __cplusplus
}
#endif
#endif // APP_STATE_H