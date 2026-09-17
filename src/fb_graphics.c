#include "fb_graphics.h"
#include "fonts_atlas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

GFX_CONTEXT g_gfx = {0};

bool gfx_init(const char *fb_dev) {
    if (!fb_dev) fb_dev = "/dev/fb0";

    g_gfx.fb_fd = open(fb_dev, O_RDWR);
    if (g_gfx.fb_fd < 0) {
        return false;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(g_gfx.fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("Error reading fixed screen info");
        close(g_gfx.fb_fd);
        return false;
    }

    if (ioctl(g_gfx.fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("Error reading variable screen info");
        close(g_gfx.fb_fd);
        return false;
    }

    g_gfx.width = vinfo.xres;
    g_gfx.height = vinfo.yres;
    uint32_t bpp = vinfo.bits_per_pixel;
    g_gfx.pixels_per_scanline = finfo.line_length / (bpp / 8);
    g_gfx.screensize = finfo.line_length * vinfo.yres;

    // Check if pixel format is BGR (blue at offset 16, red at offset 0)
    g_gfx.bgr_mode = (vinfo.red.offset == 0 && vinfo.blue.offset == 16);

    g_gfx.framebuffer = (uint32_t *)mmap(0, g_gfx.screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_gfx.fb_fd, 0);
    if (g_gfx.framebuffer == MAP_FAILED) {
        perror("Error mapping framebuffer to memory");
        close(g_gfx.fb_fd);
        return false;
    }

    size_t backbuffer_size = (size_t)g_gfx.pixels_per_scanline * g_gfx.height * sizeof(uint32_t);
    g_gfx.backbuffer = (uint32_t *)malloc(backbuffer_size);
    if (!g_gfx.backbuffer) {
        g_gfx.backbuffer = g_gfx.framebuffer;
    }

    return true;
}

void gfx_cleanup(void) {
    if (g_gfx.backbuffer && g_gfx.backbuffer != g_gfx.framebuffer) {
        free(g_gfx.backbuffer);
        g_gfx.backbuffer = NULL;
    }
    if (g_gfx.framebuffer && g_gfx.framebuffer != MAP_FAILED) {
        munmap(g_gfx.framebuffer, g_gfx.screensize);
        g_gfx.framebuffer = NULL;
    }
    if (g_gfx.fb_fd >= 0) {
        close(g_gfx.fb_fd);
        g_gfx.fb_fd = -1;
    }
}

static inline uint32_t to_fb_color(uint32_t rgb) {
    if (!g_gfx.bgr_mode) return rgb;
    uint32_t r = (rgb >> 16) & 0xFF;
    uint32_t g = (rgb >> 8) & 0xFF;
    uint32_t b = rgb & 0xFF;
    return (b << 16) | (g << 8) | r;
}

void gfx_flip(void) {
    if (g_gfx.backbuffer && g_gfx.backbuffer != g_gfx.framebuffer) {
        if (!g_gfx.bgr_mode) {
            size_t size = (size_t)g_gfx.pixels_per_scanline * g_gfx.height * sizeof(uint32_t);
            memcpy(g_gfx.framebuffer, g_gfx.backbuffer, size);
        } else {
            uint32_t total = g_gfx.pixels_per_scanline * g_gfx.height;
            for (uint32_t i = 0; i < total; i++) {
                g_gfx.framebuffer[i] = to_fb_color(g_gfx.backbuffer[i]);
            }
        }
    }
}

void gfx_clear(uint32_t color) {
    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;
    uint32_t total = g_gfx.pixels_per_scanline * g_gfx.height;
    for (uint32_t i = 0; i < total; i++) {
        fb[i] = color;
    }
}

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int x2 = x + w;
    int y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > (int)g_gfx.width) x2 = (int)g_gfx.width;
    if (y2 > (int)g_gfx.height) y2 = (int)g_gfx.height;

    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;
    for (int cy = y; cy < y2; cy++) {
        uint32_t *row = &fb[cy * g_gfx.pixels_per_scanline + x];
        int count = x2 - x;
        for (int cx = 0; cx < count; cx++) {
            row[cx] = color;
        }
    }
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t border_color, int border_width) {
    if (w <= 0 || h <= 0 || border_width <= 0) return;
    gfx_fill_rect(x, y, w, border_width, border_color);
    gfx_fill_rect(x, y + h - border_width, w, border_width, border_color);
    gfx_fill_rect(x, y, border_width, h, border_color);
    gfx_fill_rect(x + w - border_width, y, border_width, h, border_color);
}

void gfx_draw_rounded_card(int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color, int border_width) {
    gfx_fill_rect(x + 4, y, w - 8, h, bg_color);
    gfx_fill_rect(x, y + 4, 4, h - 8, bg_color);
    gfx_fill_rect(x + w - 4, y + 4, 4, h - 8, bg_color);

    if (border_width > 0) {
        gfx_fill_rect(x + 4, y, w - 8, border_width, border_color);
        gfx_fill_rect(x + 4, y + h - border_width, w - 8, border_width, border_color);
        gfx_fill_rect(x, y + 4, border_width, h - 8, border_color);
        gfx_fill_rect(x + w - border_width, y + 4, border_width, h - 8, border_color);
    }
}

void gfx_draw_gradient_h(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
    if (w <= 0 || h <= 0) return;
    int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    int r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;

    for (int col = 0; col < w; col++) {
        int cur_r = r1 + ((r2 - r1) * col) / w;
        int cur_g = g1 + ((g2 - g1) * col) / w;
        int cur_b = b1 + ((b2 - b1) * col) / w;
        uint32_t color = (cur_r << 16) | (cur_g << 8) | cur_b;
        gfx_fill_rect(x + col, y, 1, h, color);
    }
}

void gfx_darken_screen(void) {
    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;
    uint32_t total = g_gfx.pixels_per_scanline * g_gfx.height;
    for (uint32_t i = 0; i < total; i++) {
        uint32_t c = fb[i];
        uint32_t r = ((c >> 16) & 0xFF) / 4;
        uint32_t g = ((c >> 8) & 0xFF) / 4;
        uint32_t b = (c & 0xFF) / 4;
        fb[i] = (r << 16) | (g << 8) | b;
    }
}

void gfx_draw_image_rgba(int x, int y, int w, int h, const uint32_t *rgba_data) {
    if (!rgba_data) return;
    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;

    for (int cy = 0; cy < h; cy++) {
        int py = y + cy;
        if (py < 0 || py >= (int)g_gfx.height) continue;
        for (int cx = 0; cx < w; cx++) {
            int px = x + cx;
            if (px < 0 || px >= (int)g_gfx.width) continue;

            uint32_t pixel = rgba_data[cy * w + cx];
            uint32_t a = (pixel >> 24) & 0xFF;
            if (a == 0) continue;

            if (a >= 250) {
                fb[py * g_gfx.pixels_per_scanline + px] = pixel & 0x00FFFFFF;
            } else {
                uint32_t dst = fb[py * g_gfx.pixels_per_scanline + px];
                uint32_t sr = (pixel >> 16) & 0xFF;
                uint32_t sg = (pixel >> 8) & 0xFF;
                uint32_t sb = pixel & 0xFF;

                uint32_t dr = (dst >> 16) & 0xFF;
                uint32_t dg = (dst >> 8) & 0xFF;
                uint32_t db = dst & 0xFF;

                uint32_t out_r = (sr * a + dr * (255 - a)) / 255;
                uint32_t out_g = (sg * a + dg * (255 - a)) / 255;
                uint32_t out_b = (sb * a + db * (255 - a)) / 255;

                fb[py * g_gfx.pixels_per_scanline + px] = (out_r << 16) | (out_g << 8) | out_b;
            }
        }
    }
}

static const GLYPH_INFO *find_glyph(unsigned int code, FONT_TYPE font_type) {
    const GLYPH_INFO *glyphs = font_body_glyphs;
    int count = FONT_BODY_COUNT;

    if (font_type == FONT_TITLE) {
        glyphs = font_title_glyphs;
        count = FONT_TITLE_COUNT;
    } else if (font_type == FONT_DIGITS) {
        glyphs = font_digits_glyphs;
        count = FONT_DIGITS_COUNT;
    }

    for (int i = 0; i < count; i++) {
        if (glyphs[i].code == code) {
            return &glyphs[i];
        }
    }
    return &glyphs[0];
}

static const char *utf8_next(const char *p, unsigned int *out_code) {
    if (!p || !*p) {
        *out_code = 0;
        return NULL;
    }
    unsigned char c = (unsigned char)*p;
    if (c < 0x80) {
        *out_code = c;
        return p + 1;
    } else if ((c & 0xE0) == 0xC0) {
        unsigned char c2 = (unsigned char)p[1];
        if (!c2) { *out_code = c; return p + 1; }
        *out_code = ((c & 0x1F) << 6) | (c2 & 0x3F);
        return p + 2;
    } else if ((c & 0xF0) == 0xE0) {
        unsigned char c2 = (unsigned char)p[1];
        unsigned char c3 = (unsigned char)p[2];
        if (!c2 || !c3) { *out_code = c; return p + 1; }
        *out_code = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return p + 3;
    }
    *out_code = c;
    return p + 1;
}

int gfx_text_width(const char *utf8_text, FONT_TYPE font_type) {
    if (!utf8_text) return 0;
    int total_w = 0;
    unsigned int ch;
    const char *p = utf8_text;
    while (p && *p) {
        p = utf8_next(p, &ch);
        if (ch) {
            const GLYPH_INFO *g = find_glyph(ch, font_type);
            if (g) total_w += g->adv;
        }
    }
    return total_w;
}

void gfx_draw_text(int x, int y, const char *utf8_text, uint32_t color, FONT_TYPE font_type) {
    if (!utf8_text) return;
    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;

    uint32_t sr = (color >> 16) & 0xFF;
    uint32_t sg = (color >> 8) & 0xFF;
    uint32_t sb = color & 0xFF;

    int cur_x = x;
    unsigned int ch;
    const char *p = utf8_text;

    while (p && *p) {
        p = utf8_next(p, &ch);
        if (ch == '\n') {
            y += (font_type == FONT_TITLE ? 26 : 18);
            cur_x = x;
            continue;
        }
        if (!ch) continue;

        const GLYPH_INFO *g = find_glyph(ch, font_type);
        if (!g) continue;

        if (g->w > 0 && g->h > 0 && g->data) {
            for (int cy = 0; cy < g->h; cy++) {
                int py = y + g->bearing_y + cy;
                if (py < 0 || py >= (int)g_gfx.height) continue;

                for (int cx = 0; cx < g->w; cx++) {
                    int px = cur_x + g->bearing_x + cx;
                    if (px < 0 || px >= (int)g_gfx.width) continue;

                    uint8_t alpha = g->data[cy * g->w + cx];
                    if (alpha == 0) continue;

                    if (alpha >= 250) {
                        fb[py * g_gfx.pixels_per_scanline + px] = color & 0x00FFFFFF;
                    } else {
                        uint32_t dst = fb[py * g_gfx.pixels_per_scanline + px];
                        uint32_t dr = (dst >> 16) & 0xFF;
                        uint32_t dg = (dst >> 8) & 0xFF;
                        uint32_t db = dst & 0xFF;

                        uint32_t out_r = (sr * alpha + dr * (255 - alpha)) / 255;
                        uint32_t out_g = (sg * alpha + dg * (255 - alpha)) / 255;
                        uint32_t out_b = (sb * alpha + db * (255 - alpha)) / 255;

                        fb[py * g_gfx.pixels_per_scanline + px] = (out_r << 16) | (out_g << 8) | out_b;
                    }
                }
            }
        }
        cur_x += g->adv;
    }
}

void gfx_draw_text_centered(int cx, int y, const char *utf8_text, uint32_t color, FONT_TYPE font_type) {
    int w = gfx_text_width(utf8_text, font_type);
    gfx_draw_text(cx - w / 2, y, utf8_text, color, font_type);
}

void gfx_draw_text_wrapped(int x, int y, int max_w, const char *utf8_text, uint32_t color, FONT_TYPE font_type, int line_spacing) {
    if (!utf8_text || max_w <= 0) return;

    int line_h = (font_type == FONT_TITLE ? 24 : 16) + line_spacing;
    int cur_x = x;
    int cur_y = y;

    const char *p = utf8_text;
    while (p && *p) {
        const char *word_start = p;
        int word_w = 0;
        int word_chars = 0;

        while (p && *p && *p != ' ' && *p != '\n') {
            unsigned int ch;
            p = utf8_next(p, &ch);
            if (ch) {
                const GLYPH_INFO *g = find_glyph(ch, font_type);
                if (g) word_w += g->adv;
                word_chars++;
            }
        }

        if (cur_x + word_w > x + max_w && cur_x > x) {
            cur_x = x;
            cur_y += line_h;
        }

        const char *wp = word_start;
        for (int i = 0; i < word_chars; i++) {
            unsigned int ch;
            wp = utf8_next(wp, &ch);
            if (ch) {
                const GLYPH_INFO *g = find_glyph(ch, font_type);
                if (g && g->w > 0 && g->h > 0 && g->data) {
                    uint32_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
                    uint32_t *fb = g_gfx.backbuffer ? g_gfx.backbuffer : g_gfx.framebuffer;

                    for (int cy = 0; cy < g->h; cy++) {
                        int py = cur_y + g->bearing_y + cy;
                        if (py < 0 || py >= (int)g_gfx.height) continue;
                        for (int cx = 0; cx < g->w; cx++) {
                            int px = cur_x + g->bearing_x + cx;
                            if (px < 0 || px >= (int)g_gfx.width) continue;
                            uint8_t a = g->data[cy * g->w + cx];
                            if (a == 0) continue;
                            uint32_t dst = fb[py * g_gfx.pixels_per_scanline + px];
                            uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                            uint32_t out_r = (sr * a + dr * (255 - a)) / 255;
                            uint32_t out_g = (sg * a + dg * (255 - a)) / 255;
                            uint32_t out_b = (sb * a + db * (255 - a)) / 255;
                            fb[py * g_gfx.pixels_per_scanline + px] = (out_r << 16) | (out_g << 8) | out_b;
                        }
                    }
                }
                if (g) cur_x += g->adv;
            }
        }

        if (p && *p == ' ') {
            const GLYPH_INFO *sp = find_glyph(' ', font_type);
            int sp_w = sp ? sp->adv : 6;
            if (cur_x + sp_w <= x + max_w) {
                cur_x += sp_w;
            } else {
                cur_x = x;
                cur_y += line_h;
            }
            p++;
        } else if (p && *p == '\n') {
            cur_x = x;
            cur_y += line_h;
            p++;
        }
    }
}
