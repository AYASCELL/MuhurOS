#ifndef FB_GRAPHICS_H
#define FB_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Color definitions (0x00RRGGBB format)
#define COLOR_BG            0x00060913 // Deep navy/black
#define COLOR_CARD_BG       0x000E1726 // Rich dark slate
#define COLOR_CARD_BORDER   0x001E293B // Subtle border
#define COLOR_ACCENT_GOLD   0x00D4AF37 // Imperial gold
#define COLOR_ACCENT_GOLD_LT 0x00F6E05E // Light gold
#define COLOR_TEXT_WHITE    0x00F8FAFC
#define COLOR_TEXT_MUTED    0x0094A3B8
#define COLOR_TEXT_DIM      0x0064748B
#define COLOR_CYAN          0x0038BDF8 // Day/Minute cyan
#define COLOR_ROSE          0x00FB7185 // Second rose
#define COLOR_GREEN         0x0010B981 // Success emerald
#define COLOR_RED           0x00EF4444 // Danger ruby
#define COLOR_BLUE          0x003B82F6 // Navy blue
#define COLOR_ORANGE        0x00F59E0B // Warning orange

typedef enum {
    FONT_TITLE = 0,
    FONT_BODY,
    FONT_DIGITS
} FONT_TYPE;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
    uint32_t *framebuffer;
    uint32_t *backbuffer;
    int fb_fd;
    size_t screensize;
    bool bgr_mode;
} GFX_CONTEXT;

extern GFX_CONTEXT g_gfx;

bool gfx_init(const char *fb_dev);
void gfx_cleanup(void);
void gfx_flip(void);
void gfx_clear(uint32_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t border_color, int border_width);
void gfx_draw_rounded_card(int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color, int border_width);
void gfx_draw_gradient_h(int x, int y, int w, int h, uint32_t c1, uint32_t c2);
void gfx_draw_image_rgba(int x, int y, int w, int h, const uint32_t *rgba_data);
void gfx_darken_screen(void);

// Smooth Typography Functions
void gfx_draw_text(int x, int y, const char *utf8_text, uint32_t color, FONT_TYPE font_type);
int gfx_text_width(const char *utf8_text, FONT_TYPE font_type);
void gfx_draw_text_centered(int cx, int y, const char *utf8_text, uint32_t color, FONT_TYPE font_type);
void gfx_draw_text_wrapped(int x, int y, int max_w, const char *utf8_text, uint32_t color, FONT_TYPE font_type, int line_spacing);

#endif
