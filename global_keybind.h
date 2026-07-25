#ifndef GLOBAL_KEYBIND_H
#define GLOBAL_KEYBIND_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#define MAX_DEVICES 64
#define MAX_NAME_SIZE 256

typedef struct {
    char path[MAX_NAME_SIZE];
    char name[MAX_NAME_SIZE];
} device;

typedef struct {
    size_t count;
    device *devices;
} device_array;

typedef struct {
    int code;
    const char *name;
} event;

typedef struct {
    char path[MAX_NAME_SIZE];
    size_t count;
    event *events;
} event_array;

int keybind_thread();
void refresh_devices(void);
device_array *get_devices(void);
event_array *get_events(void);

#ifdef __cplusplus
}
#endif
#endif // GLOBAL_KEYBIND_H
