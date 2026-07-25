#include "app_state.h"
#include "global_keybind.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>
#include <string.h>
#include <linux/uinput.h>

// SAVED
static atomic_int listen_key;
int get_keybind_event() {
    return atomic_load(&listen_key);
}
void set_keybind_event(int key) {
    atomic_store(&listen_key, key);
    set_want_save_config(true);
}

static atomic_ullong delay_ns = 10000000;
uint64_t get_click_delay() {
    return atomic_load(&delay_ns);
}
void set_click_delay(uint64_t d) {
    atomic_store(&delay_ns, d);
    set_want_save_config(true);
}

static atomic_int click_limit = 1000000;
int get_click_limit() {
    return atomic_load(&click_limit);
}
void set_click_limit(int d) {
    atomic_store(&click_limit, d);
    set_want_save_config(true);
}

static atomic_bool hold_mode;
bool get_keybind_hold() {
    return atomic_load(&hold_mode);
}
void set_keybind_hold(bool hold) {
    atomic_store(&hold_mode, hold);
    set_want_save_config(true);
}

static atomic_int click_key = BTN_LEFT;
int get_click_button() {
    return atomic_load(&click_key);
}
void set_click_button(int key) {
    atomic_store(&click_key, key);
    set_want_save_config(true);
}

static char wanted_device[MAX_NAME_SIZE] = "";
mtx_t wanted_lock;
char *get_keybind_device() {
    char *new_str = malloc(MAX_NAME_SIZE);
    mtx_lock(&wanted_lock);
    strncpy(new_str, wanted_device, MAX_NAME_SIZE);
    mtx_unlock(&wanted_lock);
    return new_str;
}
void set_keybind_device(char *path) {
    mtx_lock(&wanted_lock);
    if (path == NULL) {
        memset(wanted_device, 0, MAX_NAME_SIZE);
    } else {
        strncpy(wanted_device, path, MAX_NAME_SIZE);
    }
    mtx_unlock(&wanted_lock);
    set_want_save_config(true);
}

static splash splash_params = {};
splash *get_splash_params() {
    return &splash_params;
}
void reset_splash_params() {
    splash_params.splash_enable = true;
    splash_params.splash_angle = -9.5;
    splash_params.splash_xpos = 0.75;
    splash_params.splash_ypos = 0.85;
    splash_params.splash_size = 0.25;
    splash_params.splash_bounce_speed = 2*3.14159265;
    splash_params.splash_bounce_size = 0.07;
    strncpy(splash_params.splash_text, "Made in China!", sizeof(splash_params.splash_text));
    splash_params.splash_color = 0xff00ffff;
    splash_params.splash_color_bg = 0xff004444;
}

// TEMPORARY
static atomic_bool running = true;
bool get_running() {
    return atomic_load(&running);
}
void set_running(bool b) {
    atomic_store(&running, b);
}

static atomic_bool clicking = false;
bool get_clicking() {
    return atomic_load(&clicking);
}
void set_clicking(bool b) {
    atomic_store(&clicking, b);
}

static atomic_bool want_read_key;
bool get_want_read_key() {
    return atomic_load(&want_read_key);
}
void set_want_read_key(bool want) {
    atomic_store(&want_read_key, want);
}

static atomic_bool want_save_config;
bool get_want_save_config() {
    return atomic_load(&want_save_config);
}
void set_want_save_config(bool want) {
    atomic_store(&want_save_config, want);
}
