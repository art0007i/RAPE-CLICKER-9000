#include "config.h"

#include "app_state.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE "clicker.conf"

typedef void (*cfg_write_fn)(FILE *);
typedef void (*cfg_read_fn)(char *);

typedef struct {
    const char *name;
    
    cfg_write_fn write;
    cfg_read_fn read;
} ConfigEntry;


// I LOVE MACRO SPAM!!!!! C LANGUAGE BEST LANGUAGE
#define CFG_CTYPE_U64    uint64_t
#define CFG_CTYPE_INT    int
#define CFG_CTYPE_BOOL   bool
#define CFG_CTYPE_FLOAT  float
#define CFG_CTYPE_HEX    uint32_t
#define CFG_CTYPE_STRING char *

// DEFINE SERIALIZERS AND DERSIALIZERS
#define PRINT_TYPE(selector, value) fprintf(file, selector, value)
#define CONFIG_TYPES \
    Y(U64, ((uint64_t) atoll(value)), PRINT_TYPE("%lu", value)) \
    Y(INT, atoi(value), PRINT_TYPE("%d", value)) \
    Y(BOOL, (strcmp(value, "true") == 0), PRINT_TYPE("%s", value ? "true" : "false")) \
    Y(FLOAT, atof(value), PRINT_TYPE("%f", value)) \
    Y(HEX, ((uint32_t) strtoul(value, NULL, 0)), PRINT_TYPE("0x%x", value)) \
    Y(STRING, value, PRINT_TYPE("%s", value))

#define Y(name, read, write) \
static CFG_CTYPE_##name type_##name##_read(char *value) { \
    return read; \
} \
static void type_##name##_write(FILE *file, CFG_CTYPE_##name value) { \
    write; \
}

CONFIG_TYPES

#undef Y
// END SERIALIZERS

#define CONFIG_ITEMS           \
    X_FN(U64, click_delay)      \
    X_FN(INT, click_button)      \
    X_FN(INT, click_limit)      \
    X_FN(BOOL, keybind_hold)      \
    X_FN(STRING, keybind_device)  \
    X_FN(INT, keybind_event)      \
    X_PTR(BOOL, splash_enable) \
    X_PTR(FLOAT, splash_angle) \
    X_PTR(FLOAT, splash_xpos) \
    X_PTR(FLOAT, splash_ypos) \
    X_PTR(FLOAT, splash_size) \
    X_PTR(FLOAT, splash_bounce_speed) \
    X_PTR(FLOAT, splash_bounce_size) \
    X(STRING, splash_text, get_splash_params()->splash_text, strncpy(get_splash_params()->splash_text, value, sizeof(get_splash_params()->splash_text))) \
    X_PTR(HEX, splash_color) \
    X_PTR(HEX, splash_color_bg) \

#define X(cfgtype, cfgname, cfgget, cfgset) \
static void write_##cfgname(FILE *file) { \
    fputs(#cfgname "=", file); \
    type_##cfgtype##_write(file, cfgget); \
    fputs("\n", file); \
} \
static void read_##cfgname(char *val) { \
    CFG_CTYPE_##cfgtype value = type_##cfgtype##_read(val); \
    cfgset; \
}

#define X_PTR(type, name) \
    X(type, name, get_splash_params()->name, get_splash_params()->name = value)

#define X_FN(type, name) \
    X(type, name, get_##name(), set_##name(value))

CONFIG_ITEMS

#undef X

#define X(cfgtype, cfgname, cfgget, cfgset) \
    { \
        .name = #cfgname, \
        .write = write_##cfgname, \
        .read = read_##cfgname, \
    },

static ConfigEntry config_entries[] = {
    CONFIG_ITEMS
};
#define CONFIG_COUNT (sizeof(config_entries) / sizeof(config_entries[0]))

#undef X_FN
#undef X_PTR

void save_config(void)
{
    printf("Saving config file!\n");
    fflush(stdout);
    FILE *f = fopen(CONFIG_FILE, "w");
    
    if (!f)
        return;
    
    for (int i = 0; i < CONFIG_COUNT; ++i) {
        config_entries[i].write(f);
    }
    
    fclose(f);
    
    set_want_save_config(false);
}


void load_config(void)
{
    FILE *f = fopen(CONFIG_FILE, "r");
    
    if (!f)
        return;
    
    char line[256];
    
    splash *splash_params = get_splash_params();
    while (fgets(line, sizeof(line), f)) {
        
        char key[128];
        char value[256];
        
        if (sscanf(line, "%127[^=]=%127[^\n]", key, value) != 2)
            continue;
        
        printf("k = %s v = %s\n", key, value);
        
        for (int i = 0; i < CONFIG_COUNT; ++i) {
            if (strcmp(key, config_entries[i].name) != 0) continue;
            config_entries[i].read(value);
            break;
        }
    }
    
    fclose(f);
}