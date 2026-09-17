#include "ui.h"
#include "fb_graphics.h"
#include "config.h"
#include "logo_data.h"
#include <stdio.h>
#include <string.h>

UI_STATE g_ui = {0};

void ui_init(void) {
    g_ui.days = 46;
    g_ui.hours = 10;
    g_ui.minutes = 9;
    g_ui.seconds = 29;
    g_ui.progress_percent = 0;
    g_ui.auto_shutdown_remaining = g_config.auto_shutdown_seconds > 0 ? g_config.auto_shutdown_seconds : 180;
    g_ui.is_paused = false;
    g_ui.active_modal = MODAL_NONE;
    g_ui.selected_button = 0;
    g_ui.pin_len = 0;
    g_ui.pin_error = false;
    g_ui.settings_field = 0;
    g_ui.settings_saved = false;
}

static void int_to_str2(int val, char *out) {
    if (val < 0) val = 0;
    if (val > 99) {
        out[0] = '0' + (val / 100) % 10;
        out[1] = '0' + (val / 10) % 10;
        out[2] = '0' + (val % 10);
        out[3] = '\0';
    } else {
        out[0] = '0' + (val / 10);
        out[1] = '0' + (val % 10);
        out[2] = '\0';
    }
}

void ui_render_main(void) {
    gfx_clear(COLOR_BG);

    int sw = g_gfx.width;
    int sh = g_gfx.height;
    int cx = sw / 2;

    // 1. Top Header Bar (Height: 60px)
    int top_h = 60;
    gfx_fill_rect(0, 0, sw, top_h, 0x000B111E);
    gfx_draw_rect(0, top_h - 2, sw, 2, COLOR_ACCENT_GOLD, 2);

    // Ayascell Logo from PNG (48x48)
    gfx_draw_image_rgba(24, 6, LOGO_WIDTH, LOGO_HEIGHT, logo_data);

    gfx_draw_text(84, 12, "AYASCELL MUHUROS", COLOR_CYAN, FONT_TITLE);
    gfx_draw_text(84, 36, "RESMI CIHAZ VE SISTEM GUVENLIK PROTOKOLU", COLOR_ACCENT_GOLD, FONT_BODY);

    // 2. Hero Seal Notice
    int content_y = 90;
    gfx_draw_text_centered(cx, content_y, g_config.seal_title, COLOR_TEXT_WHITE, FONT_TITLE);

    // 3. Main Countdown Container Card
    content_y += 46;
    int card_w = sw - 120;
    if (card_w > 960) card_w = 960;
    int card_x = cx - card_w / 2;
    int card_h = 186;

    gfx_draw_rounded_card(card_x, content_y, card_w, card_h, COLOR_CARD_BG, COLOR_CARD_BORDER, 1);

    // Countdown Card Header
    gfx_draw_text(card_x + 20, content_y + 12, "MUHUR VE HEDEF SURE SAYACI", COLOR_ACCENT_GOLD, FONT_BODY);

    char date_range[64];
    snprintf(date_range, sizeof(date_range), "%04d-%02d-%02d -> %04d-%02d-%02d",
             g_config.start_year, g_config.start_month, g_config.start_day,
             g_config.end_year, g_config.end_month, g_config.end_day);
    gfx_draw_text(card_x + card_w - 230, content_y + 12, date_range, COLOR_TEXT_DIM, FONT_BODY);

    // 4 Countdown Digits Boxes
    int box_gap = 14;
    int box_w = (card_w - 40 - 3 * box_gap) / 4;
    int box_h = 92;
    int by = content_y + 36;

    char str_num[16];

    // GÜN (Days)
    int cur_bx = card_x + 20;
    gfx_draw_rounded_card(cur_bx, by, box_w, box_h, 0x00141D2E, 0x00223554, 1);
    int_to_str2((int)g_ui.days, str_num);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + 10, str_num, COLOR_CYAN, FONT_DIGITS);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + box_h - 22, "GUN", COLOR_TEXT_MUTED, FONT_BODY);

    // SAAT (Hours)
    cur_bx += box_w + box_gap;
    gfx_draw_rounded_card(cur_bx, by, box_w, box_h, 0x00141D2E, 0x00223554, 1);
    int_to_str2((int)g_ui.hours, str_num);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + 10, str_num, COLOR_CYAN, FONT_DIGITS);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + box_h - 22, "SAAT", COLOR_TEXT_MUTED, FONT_BODY);

    // DAKİKA (Minutes)
    cur_bx += box_w + box_gap;
    gfx_draw_rounded_card(cur_bx, by, box_w, box_h, 0x00141D2E, 0x00223554, 1);
    int_to_str2((int)g_ui.minutes, str_num);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + 10, str_num, COLOR_CYAN, FONT_DIGITS);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + box_h - 22, "DAKIKA", COLOR_TEXT_MUTED, FONT_BODY);

    // SANİYE (Seconds)
    cur_bx += box_w + box_gap;
    gfx_draw_rounded_card(cur_bx, by, box_w, box_h, 0x00281622, 0x005E233E, 1);
    int_to_str2((int)g_ui.seconds, str_num);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + 10, str_num, COLOR_ROSE, FONT_DIGITS);
    gfx_draw_text_centered(cur_bx + box_w / 2, by + box_h - 22, "SANIYE", 0x00FCA5A5, FONT_BODY);

    // Progress Bar
    int pbar_y = by + box_h + 14;
    gfx_draw_text(card_x + 20, pbar_y, "Safak Ilerlemesi:", COLOR_TEXT_MUTED, FONT_BODY);

    int pbar_x = card_x + 150;
    int pbar_w = card_w - 320;
    int pbar_h = 10;
    gfx_fill_rect(pbar_x, pbar_y + 3, pbar_w, pbar_h, 0x00090E17);
    gfx_draw_rect(pbar_x, pbar_y + 3, pbar_w, pbar_h, 0x001F2C42, 1);

    int fill_w = (pbar_w * g_ui.progress_percent) / 100;
    if (fill_w < 6) fill_w = 6;
    gfx_draw_gradient_h(pbar_x, pbar_y + 3, fill_w, pbar_h, COLOR_BLUE, COLOR_GREEN);

    char full_pct[32];
    snprintf(full_pct, sizeof(full_pct), "%%%d Tamamlandi", g_ui.progress_percent);
    gfx_draw_text(pbar_x + pbar_w + 12, pbar_y, full_pct, COLOR_GREEN, FONT_BODY);

    // 4. Action Buttons Row
    content_y += card_h + 18;
    int btn_h = 44;

    // Button 1: [ MÜHÜR TUTANAĞI ]
    int btn2_w = 240;
    int btn_gap = 16;
    int btn1_w = card_w - btn2_w - btn_gap;
    gfx_draw_rounded_card(card_x, content_y, btn1_w, btn_h,
                          g_ui.selected_button == 0 ? 0x002B3B5C : 0x00162032,
                          COLOR_ACCENT_GOLD,
                          g_ui.selected_button == 0 ? 2 : 1);
    gfx_draw_text_centered(card_x + btn1_w / 2, content_y + 14,
                           "[1] MUHUR TUTANAGI VE GEREKCE (NEDEN - SONUC)",
                           COLOR_ACCENT_GOLD_LT, FONT_BODY);

    // Button 2: [ SİSTEMİ KAPAT ]
    int btn2_x = card_x + btn1_w + btn_gap;
    gfx_draw_rounded_card(btn2_x, content_y, btn2_w, btn_h,
                          g_ui.selected_button == 1 ? 0x00DC2626 : 0x00991B1B,
                          COLOR_RED,
                          g_ui.selected_button == 1 ? 2 : 1);
    gfx_draw_text_centered(btn2_x + btn2_w / 2, content_y + 14,
                           "[2] SISTEMI KAPAT",
                           COLOR_TEXT_WHITE, FONT_BODY);

    // 5. PROMINENT AUTO-SHUTDOWN COUNTDOWN BANNER
    content_y += btn_h + 16;
    int banner_h = 48;
    gfx_draw_rounded_card(card_x, content_y, card_w, banner_h, 0x00111A2B, 0x00E28816, 1);

    if (g_ui.is_paused) {
        gfx_draw_text(card_x + 24, content_y + 16,
                      "[!] Yonetici Modu Acik: Otomatik kapanma sayaci duraklatildi.",
                      COLOR_ACCENT_GOLD_LT, FONT_BODY);
    } else {
        int mins = g_ui.auto_shutdown_remaining / 60;
        int secs = g_ui.auto_shutdown_remaining % 60;
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", mins, secs);

        gfx_draw_text(card_x + 24, content_y + 16, "Guvenlik Proseduru: Cihaz ", COLOR_TEXT_MUTED, FONT_BODY);
        int w_label = gfx_text_width("Guvenlik Proseduru: Cihaz ", FONT_BODY);

        int x_timer = card_x + 24 + w_label + 4;
        gfx_draw_text(x_timer, content_y + 10, time_str, COLOR_ORANGE, FONT_TITLE);
        int w_timer = gfx_text_width(time_str, FONT_TITLE);

        int x_suffix = x_timer + w_timer + 8;
        gfx_draw_text(x_suffix, content_y + 16, "sonra otomatik kapanacaktir. (Durdur: [P])", COLOR_TEXT_MUTED, FONT_BODY);

        // Countdown progress bar in banner
        int cb_w = 140;
        int cb_x = card_x + card_w - cb_w - 24;
        int cb_h = 10;
        int cb_y = content_y + 19;
        gfx_fill_rect(cb_x, cb_y, cb_w, cb_h, 0x00090E17);
        gfx_draw_rect(cb_x, cb_y, cb_w, cb_h, 0x00332210, 1);

        int total_shut = g_config.auto_shutdown_seconds > 0 ? g_config.auto_shutdown_seconds : 180;
        int cb_fill = (cb_w * g_ui.auto_shutdown_remaining) / total_shut;
        if (cb_fill < 0) cb_fill = 0;
        if (cb_fill > cb_w) cb_fill = cb_w;
        gfx_fill_rect(cb_x, cb_y, cb_fill, cb_h, COLOR_ORANGE);
    }

    // 6. Bottom Helper Bar
    int bbar_h = 44;
    int bbar_y = sh - bbar_h;
    gfx_fill_rect(0, bbar_y, sw, bbar_h, 0x000B111E);
    gfx_draw_rect(0, bbar_y, sw, 1, 0x001E293B, 1);

    gfx_draw_text(24, bbar_y + 14, "Iletisim ve Destek: Ayascell Security Lab <https://ayascell.com>", COLOR_TEXT_DIM, FONT_BODY);

    int f2_w = 200;
    int f2_x = sw - f2_w - 24;
    gfx_draw_rounded_card(f2_x, bbar_y + 6, f2_w, 32, 0x00151E2E, COLOR_ACCENT_GOLD, 1);
    gfx_draw_text_centered(f2_x + f2_w / 2, bbar_y + 14, "[F2] Yetkili Girisi (PIN)", COLOR_ACCENT_GOLD_LT, FONT_BODY);

    // Modals if active
    if (g_ui.active_modal == MODAL_TUTANAK) {
        ui_render_tutanak_modal();
    } else if (g_ui.active_modal == MODAL_PIN) {
        ui_render_pin_modal();
    } else if (g_ui.active_modal == MODAL_SETTINGS) {
        ui_render_settings_modal();
    }

    gfx_flip();
}

void ui_render_tutanak_modal(void) {
    int sw = g_gfx.width;
    int sh = g_gfx.height;

    gfx_darken_screen();

    int mw = sw - 120;
    if (mw > 860) mw = 860;
    int mh = sh - 120;
    if (mh > 480) mh = 480;

    int mx = (sw - mw) / 2;
    int my = (sh - mh) / 2;

    gfx_draw_rounded_card(mx, my, mw, mh, 0x000F172A, COLOR_ACCENT_GOLD, 2);

    // Modal Header
    gfx_draw_text(mx + 28, my + 18, "RESMI MUHUR PROTOKOLU VE GEREKCESI", COLOR_ACCENT_GOLD, FONT_TITLE);
    gfx_draw_rect(mx + 28, my + 46, mw - 56, 1, 0x00334155, 1);

    // Card 1: Neden (Gerekçe)
    int inner_w = mw - 56;
    int card1_h = (mh - 150) / 2;
    int cy = my + 60;

    gfx_draw_rounded_card(mx + 28, cy, inner_w, card1_h, 0x001E293B, 0x00334155, 1);
    gfx_draw_text(mx + 38, cy + 12, "[+] Muhurleme Gerekcesi (Neden):", COLOR_CYAN, FONT_BODY);
    gfx_draw_text_wrapped(mx + 38, cy + 34, inner_w - 24, g_config.reason_body, COLOR_TEXT_WHITE, FONT_BODY, 4);

    // Card 2: Hüküm (Sonuç)
    cy += card1_h + 14;
    gfx_draw_rounded_card(mx + 28, cy, inner_w, card1_h, 0x001E293B, 0x00334155, 1);
    gfx_draw_text(mx + 38, cy + 12, "[!] Guvenlik Tedbiri ve Hukum (Sonuc):", COLOR_ORANGE, FONT_BODY);
    gfx_draw_text_wrapped(mx + 38, cy + 34, inner_w - 24, g_config.effect_body, COLOR_TEXT_WHITE, FONT_BODY, 4);

    // Close Hint Button
    int by = my + mh - 40;
    gfx_draw_rounded_card(mx + mw / 2 - 100, by, 200, 32, 0x00334155, COLOR_TEXT_MUTED, 1);
    gfx_draw_text_centered(mx + mw / 2, by + 10, "[ESC] veya [1] Kapat", COLOR_TEXT_WHITE, FONT_BODY);
}

void ui_render_pin_modal(void) {
    int sw = g_gfx.width;
    int sh = g_gfx.height;

    gfx_darken_screen();

    int mw = 440;
    int mh = 240;
    int mx = (sw - mw) / 2;
    int my = (sh - mh) / 2;

    gfx_draw_rounded_card(mx, my, mw, mh, 0x000F172A, COLOR_ACCENT_GOLD, 2);

    gfx_draw_text_centered(mx + mw / 2, my + 18, "YETKILI GIRISI", COLOR_ACCENT_GOLD, FONT_TITLE);
    gfx_draw_text_centered(mx + mw / 2, my + 46, "Lutfen 4 haneli guvenlik PIN kodunu girin:", COLOR_TEXT_MUTED, FONT_BODY);

    int box_w = 200;
    int box_h = 44;
    int bx = mx + (mw - box_w) / 2;
    int by = my + 80;

    gfx_draw_rounded_card(bx, by, box_w, box_h, 0x00060913,
                          g_ui.pin_error ? COLOR_RED : COLOR_ACCENT_GOLD,
                          g_ui.pin_error ? 2 : 1);

    char stars[16] = {0};
    for (int i = 0; i < g_ui.pin_len; i++) {
        stars[i] = '*';
    }
    gfx_draw_text_centered(bx + box_w / 2, by + 14, stars, COLOR_ACCENT_GOLD_LT, FONT_TITLE);

    if (g_ui.pin_error) {
        gfx_draw_text_centered(mx + mw / 2, my + 140, "Hatali PIN Kodu! Tekrar Deneyin.", COLOR_RED, FONT_BODY);
    } else {
        gfx_draw_text_centered(mx + mw / 2, my + 140, "(Varsayilan PIN: 1923)", COLOR_TEXT_DIM, FONT_BODY);
    }

    gfx_draw_text_centered(mx + mw / 2, my + 190, "[ENTER] Onayla    [ESC] Iptal", COLOR_TEXT_MUTED, FONT_BODY);
}

void ui_render_settings_modal(void) {
    int sw = g_gfx.width;
    int sh = g_gfx.height;

    gfx_darken_screen();

    int mw = 720;
    int mh = 400;
    int mx = (sw - mw) / 2;
    int my = (sh - mh) / 2;

    gfx_draw_rounded_card(mx, my, mw, mh, 0x000F172A, COLOR_GREEN, 2);

    gfx_draw_text_centered(mx + mw / 2, my + 18, "YONETICI AYARLARI (CANLI DUZENLEME)", COLOR_GREEN, FONT_TITLE);
    gfx_draw_rect(mx + 28, my + 44, mw - 56, 1, 0x00334155, 1);

    const char *month_names[12] = {
        "01 (Ocak)", "02 (Subat)", "03 (Mart)", "04 (Nisan)",
        "05 (Mayis)", "06 (Haziran)", "07 (Temmuz)", "08 (Agustos)",
        "09 (Eylul)", "10 (Ekim)", "11 (Kasim)", "12 (Aralik)"
    };

    int cy = my + 56;
    int row_h = 36;
    int row_w = mw - 56;

    for (int i = 0; i < 4; i++) {
        bool is_sel = (g_ui.settings_field == i);
        int ry = cy + i * (row_h + 8);

        if (is_sel) {
            gfx_draw_rounded_card(mx + 28, ry, row_w, row_h, 0x001B2E4B, COLOR_ACCENT_GOLD, 1);
        } else {
            gfx_draw_rounded_card(mx + 28, ry, row_w, row_h, 0x00111A2B, 0x001E293B, 1);
        }

        uint32_t label_color = is_sel ? COLOR_ACCENT_GOLD_LT : COLOR_CYAN;
        uint32_t val_color = is_sel ? COLOR_TEXT_WHITE : 0x00CBD5E1;

        if (i == 0) {
            gfx_draw_text(mx + 44, ry + 10, "1. Hedef Bitis Yili", label_color, FONT_BODY);
            char y_str[32];
            snprintf(y_str, sizeof(y_str), is_sel ? "<  %d  >" : "%d", g_config.end_year);
            gfx_draw_text(mx + 300, ry + 10, y_str, val_color, FONT_BODY);
        } else if (i == 1) {
            gfx_draw_text(mx + 44, ry + 10, "2. Hedef Bitis Ayi", label_color, FONT_BODY);
            int m_idx = g_config.end_month - 1;
            if (m_idx < 0) m_idx = 0;
            if (m_idx > 11) m_idx = 11;
            char m_str[48];
            snprintf(m_str, sizeof(m_str), is_sel ? "<  %s  >" : "%s", month_names[m_idx]);
            gfx_draw_text(mx + 300, ry + 10, m_str, val_color, FONT_BODY);
        } else if (i == 2) {
            gfx_draw_text(mx + 44, ry + 10, "3. Hedef Bitis Gunu", label_color, FONT_BODY);
            char d_str[32];
            snprintf(d_str, sizeof(d_str), is_sel ? "<  %02d  >" : "%02d", g_config.end_day);
            gfx_draw_text(mx + 300, ry + 10, d_str, val_color, FONT_BODY);
        } else if (i == 3) {
            gfx_draw_text(mx + 44, ry + 10, "4. Kapanma Sayaci", label_color, FONT_BODY);
            char s_str[48];
            snprintf(s_str, sizeof(s_str), is_sel ? "<  %d Saniye  >" : "%d Saniye", g_config.auto_shutdown_seconds);
            gfx_draw_text(mx + 300, ry + 10, s_str, val_color, FONT_BODY);
        }
    }

    int sy = cy + 4 * (row_h + 8) + 10;
    if (g_ui.settings_saved) {
        gfx_draw_rounded_card(mx + 28, sy, row_w, 42, 0x00143823, COLOR_GREEN, 1);
        gfx_draw_text_centered(mx + mw / 2, sy + 13, "[V] Ayarlar basariyla kaydedildi ve uygulandi!", COLOR_GREEN, FONT_BODY);
    } else {
        gfx_draw_rounded_card(mx + 28, sy, row_w, 42, 0x001E293B, 0x00334155, 1);
        gfx_draw_text_centered(mx + mw / 2, sy + 13, "Degisiklikleri kalici olarak kaydetmek icin [ENTER] tusuna basin.", COLOR_TEXT_MUTED, FONT_BODY);
    }

    int by = my + mh - 42;
    gfx_draw_text_centered(mx + mw / 2, by + 10,
                           "[YUKARI/ASAGI] Ayar Sec   [SOL/SAG] Degistir   [ENTER] Kaydet   [ESC] Cikis",
                           COLOR_ACCENT_GOLD_LT, FONT_BODY);
}

void ui_render_shutdown_screen(void) {
    gfx_clear(0x00060913);

    int sw = g_gfx.width;
    int sh = g_gfx.height;
    int cx = sw / 2;
    int cy = sh / 2;

    // Centered Logo
    gfx_draw_image_rgba(cx - LOGO_WIDTH / 2, cy - 110, LOGO_WIDTH, LOGO_HEIGHT, logo_data);

    // Title
    gfx_draw_text_centered(cx, cy - 30, "AYASCELL MUHUROS", COLOR_CYAN, FONT_TITLE);

    // Message Box
    int box_w = 640;
    int box_h = 90;
    int bx = cx - box_w / 2;
    int by = cy + 10;
    gfx_draw_rounded_card(bx, by, box_w, box_h, 0x000E1726, COLOR_RED, 2);

    gfx_draw_text_centered(cx, by + 20, "SISTEM VE DONANIM GUVENLE KAPATILIYOR...", COLOR_RED, FONT_TITLE);
    gfx_draw_text_centered(cx, by + 54, "Lutfen bekleyin, anakart gucu ACPI S5 seviyesinde tamamen kesiliyor.", COLOR_TEXT_MUTED, FONT_BODY);

    gfx_flip();
}
