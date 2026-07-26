#ifndef CONFIG_H
#define CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

void save_config(void);
void load_config(void);

const char *get_config_dir(void);

#ifdef __cplusplus
}
#endif
#endif
