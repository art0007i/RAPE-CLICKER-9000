#include "config.h"

#include "app_state.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE "clicker.conf"

void save_config(void)
{
    printf("Saving config file!\n");
    fflush(stdout);
    FILE *f = fopen(CONFIG_FILE, "w");
    
    if (!f)
        return;
    
    fprintf(f, "delay=%lu\n", get_delay_ns());
    fprintf(f, "keybind_device=%s\n", get_wanted_device());
    fprintf(f, "keybind_event=%d\n", get_listen_key());
    fprintf(f, "keybind_hold=%s\n", get_hold_mode() ? "true" : "false");
    
    fclose(f);
    
    set_want_save_config(false);
}


void load_config(void)
{
    FILE *f = fopen(CONFIG_FILE, "r");
    
    if (!f)
        return;
    
    char line[256];
    
    while (fgets(line, sizeof(line), f)) {
        
        char key[128];
        char value[256];
        
        if (sscanf(line, "%127[^=]=%127[^\n]", key, value) != 2)
            continue;
        
        printf("k = %s v = %s\n", key, value);
        if (strcmp(key, "delay") == 0) {
            set_delay_ns(atoll(value));
        } else if (strcmp(key, "keybind_device") == 0) {
            set_wanted_device(value);
        } else if (strcmp(key, "keybind_event") == 0) {
            set_listen_key(atoi(value));
        } else if (strcmp(key, "keybind_hold") == 0) {
            set_hold_mode(strcmp(value, "true") == 0);
        }
    }
    
    fclose(f);
}