#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char owner_name[64];
    char seal_title[128];
    char seal_subtitle[256];
    char category[64];
    char scope[64];
    char duty_type[64];
    
    int start_year;
    int start_month;
    int start_day;
    
    int end_year;
    int end_month;
    int end_day;
    
    int auto_shutdown_seconds;
    char admin_pin[16];
    
    char tutanak_title[128];
    char reason_body[512];
    char effect_body[512];

    char loaded_filepath[256];
} MUHUR_CONFIG;

extern MUHUR_CONFIG g_config;

void config_set_defaults(void);
bool config_load(const char *search_paths[]);
bool config_save(const char *filepath);

uint64_t date_to_epoch(int year, int month, int day, int hour, int minute, int second);

#endif
