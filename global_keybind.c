#include "global_keybind.h"
#include "app_state.h"
#include "evtest_key.h"
#include "config.h"

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>
#include <unistd.h>

static atomic_bool want_refresh = true;

static char current_device_path[MAX_NAME_SIZE] = "";

static int current_device = -1;

static _Atomic(device_array *) global_devices = NULL;
static _Atomic(event_array *) global_events = NULL;

static bool is_holding;

static time_t last_config_save = 0;

int resolve_by_id(device *devices, size_t device_count) {
    DIR *dir = opendir("/dev/input/by-id");
    if (!dir)
        return -1;
    
    int found = 0;
    struct dirent *de;
    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.')
            continue;
        
        char by_id_path[MAX_NAME_SIZE];
        snprintf(by_id_path, sizeof(by_id_path),
                 "/dev/input/by-id/%s", de->d_name);
        
        char resolved_by_id[MAX_NAME_SIZE];
        if (!realpath(by_id_path, resolved_by_id))
            continue;
        
        // Only care about event devices
        const char *event_name = strrchr(resolved_by_id, '/');
        if (!event_name || strncmp(event_name + 1, "event", 5) != 0)
            continue;
        
        for (size_t i = 0; i < device_count; i++) {
            char resolved_device[MAX_NAME_SIZE];
            
            if (!realpath(devices[i].path, resolved_device))
                continue;
            
            if (strcmp(resolved_device, resolved_by_id) == 0) {
                snprintf(devices[i].path,
                         sizeof(devices[i].path),
                         "%s",
                         by_id_path);
                ++found;
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

void rebuild_device_array() {
    
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        perror("opendir");
        return;
    }
    
    device devices[MAX_DEVICES] = {};
    size_t device_count = 0;
    
    struct dirent *de;
    while ((de = readdir(dir))) {
        if (strncmp(de->d_name, "event", 5) != 0)
            continue;
        
        if (device_count >= MAX_DEVICES) {
            fprintf(stderr, "You have more than %d devices! How???", MAX_DEVICES);
            break;
        }
        snprintf(devices[device_count].path, sizeof(devices[device_count].path),
                 "/dev/input/%s", de->d_name);
        
        int fd = open(devices[device_count].path, O_RDONLY);
        
        if (fd < 0) {
            continue;
        }
        
        if (ioctl(fd, EVIOCGNAME(sizeof(devices[device_count].name)),
                  devices[device_count].name) < 0) {
            strcpy(devices[device_count].name, "(unknown)");
        }
        
        close(fd);
        device_count++;
    }
    closedir(dir);
    
    int id_count = resolve_by_id(devices, device_count);
    printf("Device count: %zu, by id %d\n", device_count, id_count);
    fflush(stdout);
    
    
    if (device_count == 0) {
        puts("No input devices found.");
        return;
    }
    
    device_array *dev_arr = malloc(sizeof(device_array));
    dev_arr->count = device_count;
    size_t array_bytes = device_count * sizeof(device);
    dev_arr->devices = malloc(array_bytes);
    memcpy(dev_arr->devices, devices, array_bytes);
            
    atomic_store(&global_devices, dev_arr);
}

int open_current_device() {
    if (current_device >= 0) {
        close(current_device);
    }
    
    if (strlen(current_device_path) == 0) {
        printf("Selecting null device\n");
        current_device = -1;
        return 1;
    }
    
    int fd = open(current_device_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s\n", current_device_path);
        return 1;
    }
    current_device = fd;
    
    printf("Open fd: %d\n", fd);
    
    unsigned long key_bits[NBITS(KEY_MAX + 1)];
    memset(key_bits, 0, sizeof(key_bits));
    
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0){
        fprintf(stderr, "ioctl error\n");
        return 1;
    }
    
    event events[KEY_MAX] = {};
    size_t event_count = 0;
    int listen_key = get_listen_key();
    bool listen_invalid = true;
    
    for (int key = 0; key <= KEY_MAX; key++) {
        if (test_bit(key, key_bits)) {
            if (key == listen_key) {
                listen_invalid = false;
            }
            const char *key_name = keys[key] ? keys[key] : "?"; 
            printf("Supports key code %d (%s)\n", key, key_name);
            events[event_count].code = key;
            events[event_count].name = key_name;
            ++event_count;
        }
    }
    fflush(stdout);
    
    event_array *evt_arr = malloc(sizeof(event_array));
    evt_arr->count = event_count;
    memcpy(evt_arr->path, current_device_path, MAX_NAME_SIZE);
    size_t array_bytes = event_count * sizeof(event);
    evt_arr->events = malloc(array_bytes);
    memcpy(evt_arr->events, events, array_bytes);
    
    if(listen_invalid) {
        set_listen_key(-1);
    }
    atomic_store(&global_events, evt_arr);
    
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    return 0;
}

bool should_change(char wanted[], char current[]) {
    return strcmp(wanted, current) != 0;
}

int keybind_thread() {
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 100000000 // 100 ms
    };
    struct input_event ev;
    
    while(get_running()) {
        if (want_refresh) {
            rebuild_device_array();
            atomic_store(&want_refresh, false);
        }
        
        // TODO: maybe add get_wanted_device_unsafe or something, that would avoid copying the string here, since we only need it to compare
        char *wanted_device = get_wanted_device();
        if (should_change(wanted_device, current_device_path)) {
            strncpy(current_device_path, wanted_device, MAX_NAME_SIZE);
            
            set_clicking(false); // just in case...
            printf("Changing device to %s\n", current_device_path);
            if (open_current_device() != 0) {
                set_wanted_device(NULL);
            }
        }
        free(wanted_device);
        
        if (current_device >= 0) {
            int key = get_listen_key();
            while (get_running()) {
                // Poll all events.
                ssize_t n = read(current_device, &ev, sizeof(ev));
                if (n == sizeof(ev)) {
                    if (ev.type == EV_KEY) {
                        if (ev.value == 1) {
                            if (get_want_read_key()) {
                                set_want_read_key(false);
                                set_listen_key(ev.code);
                                key = -1; // Don't listen until next loop...
                            }
                            if (ev.code == key) {
                                if (get_hold_mode()) {
                                    set_clicking(true);
                                } else {
                                    bool curr = get_clicking();
                                    set_clicking(!curr);
                                }
                            }
                        }
                        if (get_hold_mode() && ev.value == 0 && ev.code == key) {
                            set_clicking(false);
                        }
                    }
                    
                    fflush(stdout);
                } else {
                    break;
                }
            }
        }
        
        time_t now = time(NULL);
        if (last_config_save == 0)
            last_config_save = now;
        
        if (get_want_save_config() && (now - last_config_save) >= 10) {
            save_config();
            last_config_save = now;
        }
        
        thrd_sleep(&ts, NULL);
    }
    
    printf("Keybind thread dying\n");
    fflush(stdout);
    return 0;
}

void refresh_devices() {
    atomic_store(&want_refresh, true);
}

device_array *get_devices() {
    return atomic_load(&global_devices);
}

event_array *get_events() {
    return atomic_load(&global_events);
}

