/*
 * UI Reactivity Implementation - ASCII Dancer v2.4
 *
 * Terminal-safe reactive UI rendering using ncurses.
 * All elements use glyph density only for accessibility.
 */

#include "ui_reactive.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncurses.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============ Border Characters ============ */

/* Border styles from thin to thick */
static const char *border_h[] = {"─", "━", "▬", "█"};
static const char *border_v[] = {"│", "┃", "▌", "█"};
static const char *corner_tl[] = {"┌", "┏", "▛", "█"};
static const char *corner_tr[] = {"┐", "┓", "▜", "█"};
static const char *corner_bl[] = {"└", "┗", "▙", "█"};
static const char *corner_br[] = {"┘", "┛", "▟", "█"};

/* Bar characters for meter (8 levels of fill) */
static const char *bar_chars[] = {
    " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
};

/* Beat indicator frames */
static const char *beat_frames[] = {
    "○", "◔", "◑", "◕", "●", "◕", "◑", "◔"
};
#define BEAT_FRAME_COUNT 8

/* ============ Creation / Destruction ============ */

UIReactive* ui_reactive_create(void) {
    UIReactive *ui = calloc(1, sizeof(UIReactive));
    if (!ui) return NULL;
    
    /* Default visibility */
    ui->visible.show_border = true;
    ui->visible.show_energy_meter = true;
    ui->visible.show_beat_indicator = true;
    ui->visible.show_bpm = true;
    ui->visible.show_debug = false;
    
    /* Default smoothing (slower than dancer) */
    ui->smooth_coef = 0.15f;
    
    /* Default layout */
    ui->screen_width = 80;
    ui->screen_height = 24;
    ui->content_x = 1;
    ui->content_y = 1;
    ui->content_width = 78;
    ui->content_height = 22;
    
    /* Initial BPM */
    ui->bpm_display = 120.0f;
    
    return ui;
}

void ui_reactive_destroy(UIReactive *ui) {
    if (ui) free(ui);
}

/* ============ Internal Helpers ============ */

static float smooth_towards(float current, float target, float coef) {
    return current + (target - current) * coef;
}

/* ============ Update ============ */

void ui_reactive_update(UIReactive *ui,
                        float energy, float bass, float mid, float treble,
                        float beat_phase, float beat_hit, float bpm,
                        float dt) {
    if (!ui) return;
    
    /* Slow smoothing for stable display */
    float coef = ui->smooth_coef;
    
    ui->energy_display = smooth_towards(ui->energy_display, energy, coef);
    ui->bass_display = smooth_towards(ui->bass_display, bass, coef);
    ui->mid_display = smooth_towards(ui->mid_display, mid, coef);
    ui->treble_display = smooth_towards(ui->treble_display, treble, coef);
    ui->beat_phase_display = beat_phase;  /* Phase doesn't need smoothing */
    
    /* BPM smoothing (very slow) */
    if (bpm > 30.0f && bpm < 300.0f) {
        ui->bpm_display = smooth_towards(ui->bpm_display, bpm, coef * 0.3f);
    }
    
    /* Beat hit triggers border pulse */
    if (beat_hit > ui->beat_hit_display) {
        ui->beat_hit_display = beat_hit;
        ui->border_pulse = beat_hit;
    } else {
        /* Decay beat hit and border pulse */
        ui->beat_hit_display *= 0.9f;
        ui->border_pulse *= 0.85f;
    }
    
    /* Update border style based on pulse */
    ui->border_style = (int)(ui->border_pulse * 3.9f);
    if (ui->border_style > 3) ui->border_style = 3;
    
    /* Update energy meter with peak hold */
    ui->meter_value = smooth_towards(ui->meter_value, energy, coef * 1.5f);
    if (energy > ui->meter_peak) {
        ui->meter_peak = energy;
        ui->peak_hold_time = 0;
    } else {
        ui->peak_hold_time += dt;
        if (ui->peak_hold_time > 0.5f) {
            /* Peak decay after hold */
            ui->meter_peak *= 0.95f;
        }
    }
    
    /* Update beat indicator animation frame */
    ui->beat_frame = (int)(beat_phase * BEAT_FRAME_COUNT) % BEAT_FRAME_COUNT;
}

/* ============ Rendering ============ */

static void ui_render_border(const UIReactive *ui) {
    if (!ui || !ui->visible.show_border) return;
    
    int w = ui->screen_width;
    int h = ui->screen_height;
    int style = ui->border_style;
    
    /* Top border */
    mvprintw(0, 0, "%s", corner_tl[style]);
    for (int x = 1; x < w - 1; x++) {
        mvprintw(0, x, "%s", border_h[style]);
    }
    mvprintw(0, w - 1, "%s", corner_tr[style]);
    
    /* Side borders */
    for (int y = 1; y < h - 1; y++) {
        mvprintw(y, 0, "%s", border_v[style]);
        mvprintw(y, w - 1, "%s", border_v[style]);
    }
    
    /* Bottom border */
    mvprintw(h - 1, 0, "%s", corner_bl[style]);
    for (int x = 1; x < w - 1; x++) {
        mvprintw(h - 1, x, "%s", border_h[style]);
    }
    mvprintw(h - 1, w - 1, "%s", corner_br[style]);
}

static void ui_render_energy_meter(const UIReactive *ui, int x, int y, int width) {
    if (!ui || !ui->visible.show_energy_meter || width < 3) return;
    
    /* Calculate fill */
    int fill = (int)(ui->meter_value * (width - 2) * 8);  /* 8 sub-levels per char */
    int full_chars = fill / 8;
    int partial = fill % 8;
    
    /* Peak marker position */
    int peak_pos = (int)(ui->meter_peak * (width - 2));
    if (peak_pos >= width - 2) peak_pos = width - 3;
    
    /* Draw meter frame */
    mvprintw(y, x, "[");
    
    /* Draw filled portion */
    for (int i = 0; i < width - 2; i++) {
        int cx = x + 1 + i;
        if (i < full_chars) {
            mvprintw(y, cx, "%s", bar_chars[8]);  /* Full */
        } else if (i == full_chars) {
            mvprintw(y, cx, "%s", bar_chars[partial]);  /* Partial */
        } else if (i == peak_pos) {
            mvprintw(y, cx, "▏");  /* Peak marker */
        } else {
            mvprintw(y, cx, " ");
        }
    }
    
    mvprintw(y, x + width - 1, "]");
}

static void ui_render_beat_indicator(UIReactive *ui, int x, int y) {
    if (!ui || !ui->visible.show_beat_indicator) return;
    
    mvprintw(y, x, "%s", beat_frames[ui->beat_frame]);
}

static void ui_render_bpm_display(const UIReactive *ui, int x, int y) {
    if (!ui || !ui->visible.show_bpm) return;
    
    mvprintw(y, x, "%.0f BPM", ui->bpm_display);
}

static void ui_render_spectrum_mini(const UIReactive *ui, int x, int y, int width) {
    if (!ui || width < 6) return;
    
    /* 3-band mini spectrum */
    int band_width = width / 3;
    
    /* Bass */
    int bass_fill = (int)(ui->bass_display * 8);
    if (bass_fill > 8) bass_fill = 8;
    for (int i = 0; i < band_width; i++) {
        mvprintw(y, x + i, "%s", bar_chars[bass_fill]);
    }
    
    /* Mid */
    int mid_fill = (int)(ui->mid_display * 8);
    if (mid_fill > 8) mid_fill = 8;
    for (int i = 0; i < band_width; i++) {
        mvprintw(y, x + band_width + i, "%s", bar_chars[mid_fill]);
    }
    
    /* Treble */
    int treble_fill = (int)(ui->treble_display * 8);
    if (treble_fill > 8) treble_fill = 8;
    for (int i = 0; i < band_width; i++) {
        mvprintw(y, x + band_width * 2 + i, "%s", bar_chars[treble_fill]);
    }
}

void ui_reactive_render(UIReactive *ui) {
    if (!ui) return;
    
    /* Border */
    ui_render_border(ui);
    
    /* Status line at bottom */
    int status_y = ui->screen_height - 2;
    
    /* Energy meter on left */
    ui_render_energy_meter(ui, 2, status_y, 12);
    
    /* Beat indicator */
    ui_render_beat_indicator(ui, 16, status_y);
    
    /* BPM display */
    ui_render_bpm_display(ui, 19, status_y);
    
    /* Mini spectrum on right */
    ui_render_spectrum_mini(ui, ui->screen_width - 12, status_y, 9);
    
    /* Debug info if enabled */
    if (ui->visible.show_debug) {
        mvprintw(1, 2, "E:%.2f B:%.2f M:%.2f T:%.2f Ph:%.2f",
                 ui->energy_display, ui->bass_display, 
                 ui->mid_display, ui->treble_display,
                 ui->beat_phase_display);
    }
}

/* ============ Configuration ============ */

void ui_reactive_set_layout(UIReactive *ui,
                            int screen_width, int screen_height,
                            int content_x, int content_y,
                            int content_width, int content_height) {
    if (!ui) return;
    ui->screen_width = screen_width;
    ui->screen_height = screen_height;
    ui->content_x = content_x;
    ui->content_y = content_y;
    ui->content_width = content_width;
    ui->content_height = content_height;
}

void ui_reactive_set_visible(UIReactive *ui, UIVisibility vis) {
    if (!ui) return;
    ui->visible = vis;
}

void ui_toggle_border(UIReactive *ui) {
    if (ui) ui->visible.show_border = !ui->visible.show_border;
}

void ui_toggle_energy_meter(UIReactive *ui) {
    if (ui) ui->visible.show_energy_meter = !ui->visible.show_energy_meter;
}

void ui_toggle_beat_indicator(UIReactive *ui) {
    if (ui) ui->visible.show_beat_indicator = !ui->visible.show_beat_indicator;
}

void ui_toggle_debug(UIReactive *ui) {
    if (ui) ui->visible.show_debug = !ui->visible.show_debug;
}

void ui_reactive_set_smoothing(UIReactive *ui, float speed) {
    if (!ui) return;
    ui->smooth_coef = 0.05f + speed * 0.45f;  /* Range: 0.05 to 0.5 */
}

/* ============ Utility ============ */

const char* ui_get_border_char(int style, bool is_corner, int corner_type) {
    if (style < 0) style = 0;
    if (style > 3) style = 3;
    
    if (!is_corner) {
        return border_h[style];
    }
    
    switch (corner_type) {
        case 0: return corner_tl[style];
        case 1: return corner_tr[style];
        case 2: return corner_bl[style];
        case 3: return corner_br[style];
        default: return corner_tl[style];
    }
}

const char* ui_get_bar_char(int level) {
    if (level < 0) level = 0;
    if (level > 8) level = 8;
    return bar_chars[level];
}
