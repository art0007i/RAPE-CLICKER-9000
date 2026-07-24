#ifndef APP_STATE_H
#define APP_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

bool get_running();
void set_running(bool);

bool get_clicking();
void set_clicking(bool);

uint64_t get_delay_ns();
void set_delay_ns(uint64_t);

int get_listen_key();
void set_listen_key(int);

bool get_want_read_key();
void set_want_read_key(bool);

bool get_hold_mode();
void set_hold_mode(bool);

char *get_wanted_device();
void set_wanted_device(char *);

bool get_want_save_config();
void set_want_save_config(bool);

#ifdef __cplusplus
}
#endif
#endif // APP_STATE_H