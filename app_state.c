#include "app_state.h"
#include "global_keybind.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>
#include <string.h>

// SAVED
static atomic_int listen_key;
int get_listen_key() {
    return atomic_load(&listen_key);
}
void set_listen_key(int key) {
    atomic_store(&listen_key, key);
    set_want_save_config(true);
}

static atomic_ullong delay_ns = 10000000;
uint64_t get_delay_ns() {
    return atomic_load(&delay_ns);
}
void set_delay_ns(uint64_t d) {
    atomic_store(&delay_ns, d);
    set_want_save_config(true);
}

static atomic_bool hold_mode;
bool get_hold_mode() {
    return atomic_load(&hold_mode);
}
void set_hold_mode(bool hold) {
    atomic_store(&hold_mode, hold);
    set_want_save_config(true);
}

static char wanted_device[MAX_NAME_SIZE] = "";
mtx_t wanted_lock;
char *get_wanted_device() {
    char *new_str = malloc(MAX_NAME_SIZE);
    mtx_lock(&wanted_lock);
    strncpy(new_str, wanted_device, MAX_NAME_SIZE);
    mtx_unlock(&wanted_lock);
    return new_str;
}
void set_wanted_device(char *path) {
    mtx_lock(&wanted_lock);
    if (path == NULL) {
        memset(wanted_device, 0, MAX_NAME_SIZE);
    } else {
        strncpy(wanted_device, path, MAX_NAME_SIZE);
    }
    mtx_unlock(&wanted_lock);
    set_want_save_config(true);
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
