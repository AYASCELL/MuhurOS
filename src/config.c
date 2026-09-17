#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MUHUR_CONFIG g_config = {0};

static void trim_inplace(char *s) {
    int len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

void config_set_defaults(void) {
    strncpy(g_config.owner_name, "Ayascell", sizeof(g_config.owner_name) - 1);
    strncpy(g_config.seal_title, "BU BILGISAYAR GUVENLIK PROTOKOLU GEREGINCE MUHURLUDUR", sizeof(g_config.seal_title) - 1);
    strncpy(g_config.seal_subtitle, "Cihaz sahibi Ayascell tarafindan belirlenen sure boyunca erisime kapatilmistir.", sizeof(g_config.seal_subtitle) - 1);
    strncpy(g_config.category, "Cihaz ve Veri Guvenligi", sizeof(g_config.category) - 1);
    strncpy(g_config.scope, "Genel Sistem Muhurleme", sizeof(g_config.scope) - 1);
    strncpy(g_config.duty_type, "Guvenlik Muhru", sizeof(g_config.duty_type) - 1);

    g_config.start_year = 2026;
    g_config.start_month = 10;
    g_config.start_day = 1;

    g_config.end_year = 2026;
    g_config.end_month = 11;
    g_config.end_day = 1;

    g_config.auto_shutdown_seconds = 180;
    strncpy(g_config.admin_pin, "1923", sizeof(g_config.admin_pin) - 1);

    strncpy(g_config.tutanak_title, "AYASCELL SISTEM GUVENLIK VE MUHUR PROTOKOLU TUTANAGI", sizeof(g_config.tutanak_title) - 1);
    strncpy(g_config.reason_body, "Bilgisayar sahibi Ayascell'in belirledigi sure ve amac dogrultusunda cihazin guvenligini saglamak, yetkisiz mudahaleleri onlemek ve kisisel verileri koruma altina almaktir.", sizeof(g_config.reason_body) - 1);
    strncpy(g_config.effect_body, "Bu cihaz; hedef tarihe kadar her turlu ucuncu sahis kurcalamasi ve veri transferine karsi muhur altindadir. Sistem birkac dakika icinde otomatik kapanacaktir.", sizeof(g_config.effect_body) - 1);
    strncpy(g_config.loaded_filepath, "/boot/config.txt", sizeof(g_config.loaded_filepath) - 1);
}

static const int days_before_month[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

static int is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

uint64_t date_to_epoch(int year, int month, int day, int hour, int minute, int second) {
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) return 0;
    
    long long days = 0;
    for (int y = 1970; y < year; y++) {
        days += is_leap(y) ? 366 : 365;
    }
    
    days += days_before_month[month - 1];
    if (month > 2 && is_leap(year)) {
        days += 1;
    }
    days += (day - 1);

    return (uint64_t)days * 86400ULL + (uint64_t)hour * 3600ULL + (uint64_t)minute * 60ULL + (uint64_t)second;
}

static void parse_line(char *line) {
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '#' || *line == ';' || *line == '\0' || *line == '\r' || *line == '\n') {
        return;
    }

    char *eq = strchr(line, '=');
    if (!eq) return;

    *eq = '\0';
    char *key = line;
    char *val = eq + 1;

    trim_inplace(key);
    while (*val == ' ' || *val == '\t') val++;
    trim_inplace(val);

    if (strcmp(key, "hedef_yil") == 0 || strcmp(key, "end_year") == 0) {
        int v = atoi(val);
        if (v >= 2020 && v <= 2099) g_config.end_year = v;
    } else if (strcmp(key, "hedef_ay") == 0 || strcmp(key, "end_month") == 0) {
        int v = atoi(val);
        if (v >= 1 && v <= 12) g_config.end_month = v;
    } else if (strcmp(key, "hedef_gun") == 0 || strcmp(key, "end_day") == 0) {
        int v = atoi(val);
        if (v >= 1 && v <= 31) g_config.end_day = v;
    } else if (strcmp(key, "kapanma_suresi") == 0 || strcmp(key, "auto_shutdown_seconds") == 0) {
        int v = atoi(val);
        if (v >= 10 && v <= 3600) g_config.auto_shutdown_seconds = v;
    } else if (strcmp(key, "pin") == 0 || strcmp(key, "admin_pin") == 0) {
        strncpy(g_config.admin_pin, val, sizeof(g_config.admin_pin) - 1);
    } else if (strcmp(key, "baslik") == 0 || strcmp(key, "seal_title") == 0) {
        strncpy(g_config.seal_title, val, sizeof(g_config.seal_title) - 1);
    } else if (strcmp(key, "sahip") == 0 || strcmp(key, "owner_name") == 0) {
        strncpy(g_config.owner_name, val, sizeof(g_config.owner_name) - 1);
    } else if (strcmp(key, "tutanak_baslik") == 0 || strcmp(key, "tutanak_title") == 0) {
        strncpy(g_config.tutanak_title, val, sizeof(g_config.tutanak_title) - 1);
    } else if (strcmp(key, "gerekce") == 0 || strcmp(key, "reason_body") == 0) {
        strncpy(g_config.reason_body, val, sizeof(g_config.reason_body) - 1);
    } else if (strcmp(key, "sonuc") == 0 || strcmp(key, "effect_body") == 0) {
        strncpy(g_config.effect_body, val, sizeof(g_config.effect_body) - 1);
    }
}

bool config_load(const char *search_paths[]) {
    config_set_defaults();

    if (!search_paths) return false;

    for (int p = 0; search_paths[p] != NULL; p++) {
        FILE *f = fopen(search_paths[p], "r");
        if (f) {
            char line[1024];
            while (fgets(line, sizeof(line), f)) {
                parse_line(line);
            }
            fclose(f);
            strncpy(g_config.loaded_filepath, search_paths[p], sizeof(g_config.loaded_filepath) - 1);
            printf("Loaded config from: %s\n", search_paths[p]);
            return true;
        }
    }

    return false;
}

bool config_save(const char *filepath) {
    if (!filepath || strlen(filepath) == 0) {
        filepath = g_config.loaded_filepath;
    }
    if (!filepath || strlen(filepath) == 0) {
        filepath = "/boot/config.txt";
    }

    FILE *f = fopen(filepath, "w");
    if (!f) return false;

    fprintf(f, "# =========================================================\n");
    fprintf(f, "#   Ayascell MuhurOS Kiosk Yapilandirma Dosyasi\n");
    fprintf(f, "#   Bu dosya uzerinden muhur suresi ve metinleri degistirilebilir.\n");
    fprintf(f, "# =========================================================\n\n");

    fprintf(f, "sahip = %s\n", g_config.owner_name);
    fprintf(f, "baslik = %s\n\n", g_config.seal_title);

    fprintf(f, "# --- HEDEF MUHUR TARIHI ---\n");
    fprintf(f, "hedef_yil = %d\n", g_config.end_year);
    fprintf(f, "hedef_ay = %d\n", g_config.end_month);
    fprintf(f, "hedef_gun = %d\n\n", g_config.end_day);

    fprintf(f, "# --- SISTEM VE GUVENLIK AYARLARI ---\n");
    fprintf(f, "# Acilis ekraninda otomatik kapanis suresi (saniye)\n");
    fprintf(f, "kapanma_suresi = %d\n\n", g_config.auto_shutdown_seconds);

    fprintf(f, "# Yonetici ekrani acis PIN kodu (F2 tusu ile acilir)\n");
    fprintf(f, "pin = %s\n\n", g_config.admin_pin);

    fprintf(f, "# --- MUHUR TUTANAGI METINLERI ---\n");
    fprintf(f, "tutanak_baslik = %s\n", g_config.tutanak_title);
    fprintf(f, "gerekce = %s\n", g_config.reason_body);
    fprintf(f, "sonuc = %s\n", g_config.effect_body);

    fclose(f);
    return true;
}
