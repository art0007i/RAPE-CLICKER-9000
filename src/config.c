#include "config.h"

#include "app_state.h"
#include "ini.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE "clicker.ini"

typedef void (*cfg_write_fn)(FILE *);
typedef void (*cfg_read_fn)(const char *);

typedef struct {
    const char *name;

    cfg_write_fn write;
    cfg_read_fn read;
} ConfigEntry;

typedef struct {
    const char *name;

    size_t count;
    ConfigEntry *items;
} ConfigSection;

// I LOVE MACRO SPAM!!!!! C LANGUAGE BEST LANGUAGE
#define CFG_CTYPE_U64    uint64_t
#define CFG_CTYPE_INT    int
#define CFG_CTYPE_BOOL   bool
#define CFG_CTYPE_FLOAT  float
#define CFG_CTYPE_HEX    uint32_t
#define CFG_CTYPE_STRING const char *

// DEFINE SERIALIZERS AND DESERIALIZERS
#define PRINT_TYPE(selector, value) fprintf(file, selector, value)
#define CONFIG_TYPES \
    Y(U64, ((uint64_t) atoll(value)), PRINT_TYPE("%lu", value)) \
    Y(INT, atoi(value), PRINT_TYPE("%d", value)) \
    Y(BOOL, (strcmp(value, "true") == 0), PRINT_TYPE("%s", value ? "true" : "false")) \
    Y(FLOAT, atof(value), PRINT_TYPE("%f", value)) \
    Y(HEX, ((uint32_t) strtoul(value, NULL, 0)), PRINT_TYPE("0x%x", value)) \
    // Y(STRING, value, PRINT_TYPE("%s", value))

#define Y(name, read, write) \
static CFG_CTYPE_##name type_##name##_read(const char *value) { \
    return read; \
} \
static void type_##name##_write(FILE *file, CFG_CTYPE_##name value) { \
    write; \
}
// Manually unroll Y macro for strings, because the write function needs a char* not a const char*
static const char * type_STRING_read(const char *value) { \
    return value;
}
static void type_STRING_write(FILE *file, char *value) { \
    PRINT_TYPE("%s", value);
    free(value);
}

CONFIG_TYPES

#undef Y
// END SERIALIZERS

#define CLICK_ITEMS(SEC) \
    X_FN(SEC, U64, delay)      \
    X_FN(SEC, U64, length)      \
    X_FN(SEC, INT, button)      \
    X_FN(SEC, INT, limit)

#define KEYBIND_ITEMS(SEC) \
    X_FN(SEC, BOOL, hold)      \
    X_FN(SEC, STRING, device)  \
    X_FN(SEC, INT, event)

#define SPLASH_ITEMS(SEC) \
    X_SPLASH(SEC, BOOL, enable) \
    X_SPLASH(SEC, FLOAT, angle) \
    X_SPLASH(SEC, FLOAT, xpos) \
    X_SPLASH(SEC, FLOAT, ypos) \
    X_SPLASH(SEC, FLOAT, size) \
    X_SPLASH(SEC, FLOAT, bounce_speed) \
    X_SPLASH(SEC, FLOAT, bounce_size) \
    X(SEC, STRING, text, strdup(get_splash_params()->splash_text), strncpy(get_splash_params()->splash_text, value, sizeof(get_splash_params()->splash_text))) \
    X_SPLASH(SEC, HEX, color) \
    X_SPLASH(SEC, HEX, color_bg)

#define CONFIG_ITEMS           \
    SECTION(click, CLICK_ITEMS) \
    SECTION(keybind, KEYBIND_ITEMS) \
    SECTION(splash, SPLASH_ITEMS)

#define X(cfgsec, cfgtype, cfgname, cfgget, cfgset) \
static void write_##cfgsec##_##cfgname(FILE *file) { \
    fputs(#cfgname "=", file); \
    type_##cfgtype##_write(file, cfgget); \
    fputs("\n", file); \
} \
static void read_##cfgsec##_##cfgname(const char *val) { \
    CFG_CTYPE_##cfgtype value = type_##cfgtype##_read(val); \
    cfgset; \
}

#define X_SPLASH(sec, type, name) \
    X(sec, type, name, get_splash_params()->sec##_##name, get_splash_params()->sec##_##name = value)

#define X_FN(sec, type, name) \
    X(sec, type, name, get_##sec##_##name(), set_##sec##_##name(value))

#define SECTION(sec, fields) fields(sec)

CONFIG_ITEMS

#undef X
#undef SECTION

#define X(cfgsec, cfgtype, cfgname, cfgget, cfgset) \
    { \
        .name = #cfgname, \
        .write = write_##cfgsec##_##cfgname, \
        .read = read_##cfgsec##_##cfgname, \
    },


#define SECTION(sec, fields) \
    static ConfigEntry sec##_entries[] = { fields(sec) };

CONFIG_ITEMS

#undef X
#undef SECTION

#define SECTION(sec, fields) \
{ \
    .name = #sec, \
    .items = sec##_entries, \
    .count = sizeof(sec##_entries) / sizeof(sec##_entries[0]) \
},

static ConfigSection config_sections[] = {
    CONFIG_ITEMS
};

#define SECTION_COUNT (sizeof(config_sections) / sizeof(config_sections[0]))

#undef X_FN
#undef X_PTR

void save_config(void)
{
    printf("Saving config file!\n");
    fflush(stdout);
    FILE *f = fopen(CONFIG_FILE, "w");
    
    if (!f)
        return;

    for (int i = 0; i < SECTION_COUNT; ++i) {
        ConfigSection *s = &config_sections[i];
        if (i > 0) {
            fputs("\n", f);
        }
        fprintf(f, "[%s]\n", s->name);

        for (int j = 0; j < s->count; ++j) {
            s->items[j].write(f);
            // config_entries[i].write(f);
        }
    }

    fclose(f);

    set_want_save_config(false);
}

int ini_entry(void* user, const char* section, const char* name, const char* value) {
    for (int i = 0; i < SECTION_COUNT; ++i) {
        ConfigSection *s = &config_sections[i];
        if (strcmp(section, s->name) != 0) continue;
        
        for (int j = 0; j < s->count; ++j) {
            if (strcmp(name, s->items[j].name) != 0) continue;
            s->items[j].read(value);
            return 1; // success!
        }
    }
    return 0;
}

void load_config(void)
{
    if (ini_parse(CONFIG_FILE, ini_entry, NULL) < 0) {
        printf("Can't load '%s'\n", CONFIG_FILE);
    }
}