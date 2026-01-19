/*
 * 256-color theme implementation
 * Uses ncurses extended color support for smooth gradients
 */

#include "colors.h"
#include <stdlib.h>
#include <string.h>

/* Current theme */
static ThemeColors current_theme;
static int has_256_colors = 0;

/* 
 * 256-color palette indices (xterm colors):
 * 0-7:   Standard colors
 * 8-15:  Bright colors
 * 16-231: 6x6x6 color cube (216 colors)
 * 232-255: Grayscale (24 shades)
 *
 * Color cube formula: 16 + 36*r + 6*g + b (where r,g,b = 0-5)
 */

/* Helper: create color from RGB (0-5 each) */
static short rgb_to_256(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 5) r = 5;
    if (g < 0) g = 0; if (g > 5) g = 5;
    if (b < 0) b = 0; if (b > 5) b = 5;
    return 16 + 36 * r + 6 * g + b;
}

/* Helper: grayscale (0-23, where 0=dark, 23=bright) */
static short gray_to_256(int level) {
    if (level < 0) level = 0;
    if (level > 23) level = 23;
    return 232 + level;
}

/* ============ Theme definitions ============ */

static void setup_theme_default(ThemeColors *t) {
    /* Cyan gradient (cool blue to bright cyan) */
    t->dancer_colors[0] = rgb_to_256(0, 2, 3);  /* Dark cyan */
    t->dancer_colors[1] = rgb_to_256(0, 2, 4);
    t->dancer_colors[2] = rgb_to_256(0, 3, 4);
    t->dancer_colors[3] = rgb_to_256(0, 3, 5);
    t->dancer_colors[4] = rgb_to_256(0, 4, 5);
    t->dancer_colors[5] = rgb_to_256(1, 4, 5);
    t->dancer_colors[6] = rgb_to_256(1, 5, 5);
    t->dancer_colors[7] = rgb_to_256(2, 5, 5);
    t->dancer_colors[8] = rgb_to_256(3, 5, 5);
    t->dancer_colors[9] = rgb_to_256(5, 5, 5);  /* Bright white */
    
    t->shadow_color = gray_to_256(6);
    t->ground_color = gray_to_256(10);
    t->bass_color = rgb_to_256(5, 1, 1);    /* Red */
    t->mid_color = rgb_to_256(1, 5, 1);     /* Green */
    t->treble_color = rgb_to_256(1, 1, 5);  /* Blue */
    t->info_color = gray_to_256(18);
    t->bpm_color = rgb_to_256(5, 5, 0);     /* Yellow */
    t->background = -1;
}

static void setup_theme_fire(ThemeColors *t) {
    /* Fire: Dark red → Orange → Yellow → White */
    t->dancer_colors[0] = rgb_to_256(2, 0, 0);  /* Dark red */
    t->dancer_colors[1] = rgb_to_256(3, 0, 0);
    t->dancer_colors[2] = rgb_to_256(4, 1, 0);
    t->dancer_colors[3] = rgb_to_256(5, 1, 0);  /* Red */
    t->dancer_colors[4] = rgb_to_256(5, 2, 0);
    t->dancer_colors[5] = rgb_to_256(5, 3, 0);  /* Orange */
    t->dancer_colors[6] = rgb_to_256(5, 4, 0);
    t->dancer_colors[7] = rgb_to_256(5, 5, 0);  /* Yellow */
    t->dancer_colors[8] = rgb_to_256(5, 5, 2);
    t->dancer_colors[9] = rgb_to_256(5, 5, 4);  /* Bright yellow/white */
    
    t->shadow_color = rgb_to_256(1, 0, 0);
    t->ground_color = rgb_to_256(3, 1, 0);
    t->bass_color = rgb_to_256(5, 0, 0);
    t->mid_color = rgb_to_256(5, 3, 0);
    t->treble_color = rgb_to_256(5, 5, 0);
    t->info_color = rgb_to_256(5, 4, 2);
    t->bpm_color = rgb_to_256(5, 2, 0);
    t->background = -1;
}

static void setup_theme_ice(ThemeColors *t) {
    /* Ice: Deep blue → Cyan → White */
    t->dancer_colors[0] = rgb_to_256(0, 0, 2);  /* Dark blue */
    t->dancer_colors[1] = rgb_to_256(0, 0, 3);
    t->dancer_colors[2] = rgb_to_256(0, 1, 4);
    t->dancer_colors[3] = rgb_to_256(0, 2, 5);
    t->dancer_colors[4] = rgb_to_256(0, 3, 5);  /* Cyan-blue */
    t->dancer_colors[5] = rgb_to_256(1, 4, 5);
    t->dancer_colors[6] = rgb_to_256(2, 4, 5);
    t->dancer_colors[7] = rgb_to_256(3, 5, 5);  /* Bright cyan */
    t->dancer_colors[8] = rgb_to_256(4, 5, 5);
    t->dancer_colors[9] = rgb_to_256(5, 5, 5);  /* White */
    
    t->shadow_color = rgb_to_256(0, 0, 1);
    t->ground_color = rgb_to_256(1, 2, 3);
    t->bass_color = rgb_to_256(0, 2, 5);
    t->mid_color = rgb_to_256(0, 4, 5);
    t->treble_color = rgb_to_256(3, 5, 5);
    t->info_color = rgb_to_256(3, 4, 5);
    t->bpm_color = rgb_to_256(0, 5, 5);
    t->background = -1;
}

static void setup_theme_neon(ThemeColors *t) {
    /* Neon: Magenta → Cyan → Green (vaporwave) */
    t->dancer_colors[0] = rgb_to_256(3, 0, 3);  /* Dark magenta */
    t->dancer_colors[1] = rgb_to_256(4, 0, 4);
    t->dancer_colors[2] = rgb_to_256(5, 0, 5);  /* Bright magenta */
    t->dancer_colors[3] = rgb_to_256(4, 0, 5);
    t->dancer_colors[4] = rgb_to_256(2, 1, 5);
    t->dancer_colors[5] = rgb_to_256(0, 3, 5);  /* Cyan */
    t->dancer_colors[6] = rgb_to_256(0, 4, 4);
    t->dancer_colors[7] = rgb_to_256(0, 5, 3);
    t->dancer_colors[8] = rgb_to_256(0, 5, 1);  /* Green */
    t->dancer_colors[9] = rgb_to_256(2, 5, 0);  /* Bright green */
    
    t->shadow_color = rgb_to_256(1, 0, 2);
    t->ground_color = rgb_to_256(2, 0, 3);
    t->bass_color = rgb_to_256(5, 0, 3);
    t->mid_color = rgb_to_256(0, 5, 5);
    t->treble_color = rgb_to_256(0, 5, 0);
    t->info_color = rgb_to_256(5, 0, 5);
    t->bpm_color = rgb_to_256(0, 5, 5);
    t->background = -1;
}

static void setup_theme_matrix(ThemeColors *t) {
    /* Matrix: All green shades */
    t->dancer_colors[0] = rgb_to_256(0, 1, 0);  /* Very dark green */
    t->dancer_colors[1] = rgb_to_256(0, 1, 0);
    t->dancer_colors[2] = rgb_to_256(0, 2, 0);
    t->dancer_colors[3] = rgb_to_256(0, 2, 0);
    t->dancer_colors[4] = rgb_to_256(0, 3, 0);
    t->dancer_colors[5] = rgb_to_256(0, 4, 0);
    t->dancer_colors[6] = rgb_to_256(0, 4, 0);
    t->dancer_colors[7] = rgb_to_256(0, 5, 0);  /* Bright green */
    t->dancer_colors[8] = rgb_to_256(1, 5, 1);
    t->dancer_colors[9] = rgb_to_256(3, 5, 3);  /* Light green */
    
    t->shadow_color = rgb_to_256(0, 1, 0);
    t->ground_color = rgb_to_256(0, 2, 0);
    t->bass_color = rgb_to_256(0, 3, 0);
    t->mid_color = rgb_to_256(0, 4, 0);
    t->treble_color = rgb_to_256(0, 5, 0);
    t->info_color = rgb_to_256(0, 3, 0);
    t->bpm_color = rgb_to_256(0, 5, 0);
    t->background = -1;
}

static void setup_theme_synthwave(ThemeColors *t) {
    /* Synthwave: Pink → Purple → Blue with orange accents */
    t->dancer_colors[0] = rgb_to_256(2, 0, 2);  /* Dark purple */
    t->dancer_colors[1] = rgb_to_256(3, 0, 3);
    t->dancer_colors[2] = rgb_to_256(4, 0, 4);
    t->dancer_colors[3] = rgb_to_256(5, 0, 4);  /* Pink-purple */
    t->dancer_colors[4] = rgb_to_256(5, 0, 3);
    t->dancer_colors[5] = rgb_to_256(5, 1, 3);  /* Hot pink */
    t->dancer_colors[6] = rgb_to_256(5, 2, 3);
    t->dancer_colors[7] = rgb_to_256(5, 3, 4);
    t->dancer_colors[8] = rgb_to_256(5, 4, 5);
    t->dancer_colors[9] = rgb_to_256(5, 5, 5);  /* White-pink */
    
    t->shadow_color = rgb_to_256(1, 0, 2);
    t->ground_color = rgb_to_256(5, 2, 0);  /* Orange ground line */
    t->bass_color = rgb_to_256(5, 0, 3);
    t->mid_color = rgb_to_256(0, 3, 5);
    t->treble_color = rgb_to_256(5, 3, 0);
    t->info_color = rgb_to_256(5, 0, 5);
    t->bpm_color = rgb_to_256(5, 3, 0);
    t->background = -1;
}

static void setup_theme_mono(ThemeColors *t) {
    /* Monochrome: Pure grayscale */
    t->dancer_colors[0] = gray_to_256(4);
    t->dancer_colors[1] = gray_to_256(6);
    t->dancer_colors[2] = gray_to_256(8);
    t->dancer_colors[3] = gray_to_256(10);
    t->dancer_colors[4] = gray_to_256(12);
    t->dancer_colors[5] = gray_to_256(14);
    t->dancer_colors[6] = gray_to_256(16);
    t->dancer_colors[7] = gray_to_256(18);
    t->dancer_colors[8] = gray_to_256(20);
    t->dancer_colors[9] = gray_to_256(23);
    
    t->shadow_color = gray_to_256(3);
    t->ground_color = gray_to_256(8);
    t->bass_color = gray_to_256(20);
    t->mid_color = gray_to_256(16);
    t->treble_color = gray_to_256(12);
    t->info_color = gray_to_256(15);
    t->bpm_color = gray_to_256(20);
    t->background = -1;
}
static void setup_theme_aurora(ThemeColors *t) {
    /* Aurora: Green -> Teal -> Blue -> Purple (Northern Lights) */
    t->dancer_colors[0] = rgb_to_256(0, 2, 1);
    t->dancer_colors[1] = rgb_to_256(0, 3, 2);
    t->dancer_colors[2] = rgb_to_256(0, 4, 3);
    t->dancer_colors[3] = rgb_to_256(0, 4, 4);
    t->dancer_colors[4] = rgb_to_256(1, 3, 5);
    t->dancer_colors[5] = rgb_to_256(2, 2, 5);
    t->dancer_colors[6] = rgb_to_256(3, 1, 5);
    t->dancer_colors[7] = rgb_to_256(4, 1, 5);
    t->dancer_colors[8] = rgb_to_256(4, 2, 5);
    t->dancer_colors[9] = rgb_to_256(5, 3, 5);
    t->shadow_color = rgb_to_256(0, 1, 1);
    t->ground_color = rgb_to_256(1, 2, 3);
    t->bass_color = rgb_to_256(0, 4, 2);
    t->mid_color = rgb_to_256(0, 3, 5);
    t->treble_color = rgb_to_256(4, 1, 5);
    t->info_color = rgb_to_256(2, 4, 4);
    t->bpm_color = rgb_to_256(0, 5, 3);
    t->background = -1;
}

static void setup_theme_sunset(ThemeColors *t) {
    /* Sunset: Deep orange -> Pink -> Purple */
    t->dancer_colors[0] = rgb_to_256(2, 0, 1);
    t->dancer_colors[1] = rgb_to_256(3, 0, 1);
    t->dancer_colors[2] = rgb_to_256(4, 1, 2);
    t->dancer_colors[3] = rgb_to_256(5, 1, 2);
    t->dancer_colors[4] = rgb_to_256(5, 2, 1);
    t->dancer_colors[5] = rgb_to_256(5, 3, 0);
    t->dancer_colors[6] = rgb_to_256(5, 4, 1);
    t->dancer_colors[7] = rgb_to_256(5, 4, 2);
    t->dancer_colors[8] = rgb_to_256(5, 5, 3);
    t->dancer_colors[9] = rgb_to_256(5, 5, 4);
    t->shadow_color = rgb_to_256(1, 0, 1);
    t->ground_color = rgb_to_256(3, 1, 2);
    t->bass_color = rgb_to_256(5, 2, 0);
    t->mid_color = rgb_to_256(5, 1, 3);
    t->treble_color = rgb_to_256(3, 0, 4);
    t->info_color = rgb_to_256(5, 4, 3);
    t->bpm_color = rgb_to_256(5, 3, 0);
    t->background = -1;
}

static void setup_theme_ocean(ThemeColors *t) {
    /* Ocean: Deep navy -> Teal -> Aqua -> Seafoam */
    t->dancer_colors[0] = rgb_to_256(0, 1, 2);
    t->dancer_colors[1] = rgb_to_256(0, 1, 3);
    t->dancer_colors[2] = rgb_to_256(0, 2, 3);
    t->dancer_colors[3] = rgb_to_256(0, 2, 4);
    t->dancer_colors[4] = rgb_to_256(0, 3, 4);
    t->dancer_colors[5] = rgb_to_256(1, 4, 4);
    t->dancer_colors[6] = rgb_to_256(2, 4, 4);
    t->dancer_colors[7] = rgb_to_256(2, 5, 4);
    t->dancer_colors[8] = rgb_to_256(3, 5, 4);
    t->dancer_colors[9] = rgb_to_256(4, 5, 5);
    t->shadow_color = rgb_to_256(0, 0, 1);
    t->ground_color = rgb_to_256(0, 2, 3);
    t->bass_color = rgb_to_256(0, 2, 4);
    t->mid_color = rgb_to_256(0, 4, 4);
    t->treble_color = rgb_to_256(3, 5, 5);
    t->info_color = rgb_to_256(2, 4, 4);
    t->bpm_color = rgb_to_256(0, 5, 4);
    t->background = -1;
}

static void setup_theme_candy(ThemeColors *t) {
    /* Candy: Soft pastel pink -> mint -> lavender */
    t->dancer_colors[0] = rgb_to_256(4, 2, 3);
    t->dancer_colors[1] = rgb_to_256(5, 2, 3);
    t->dancer_colors[2] = rgb_to_256(5, 3, 4);
    t->dancer_colors[3] = rgb_to_256(5, 4, 5);
    t->dancer_colors[4] = rgb_to_256(4, 4, 5);
    t->dancer_colors[5] = rgb_to_256(3, 5, 4);
    t->dancer_colors[6] = rgb_to_256(4, 5, 4);
    t->dancer_colors[7] = rgb_to_256(4, 5, 5);
    t->dancer_colors[8] = rgb_to_256(5, 5, 4);
    t->dancer_colors[9] = rgb_to_256(5, 5, 5);
    t->shadow_color = rgb_to_256(3, 2, 3);
    t->ground_color = rgb_to_256(4, 3, 4);
    t->bass_color = rgb_to_256(5, 2, 3);
    t->mid_color = rgb_to_256(3, 5, 4);
    t->treble_color = rgb_to_256(4, 3, 5);
    t->info_color = rgb_to_256(5, 4, 5);
    t->bpm_color = rgb_to_256(5, 3, 4);
    t->background = -1;
}

static void setup_theme_vapor(ThemeColors *t) {
    /* Vapor: Hot pink -> Cyan -> Purple (intense vaporwave) */
    t->dancer_colors[0] = rgb_to_256(5, 0, 2);
    t->dancer_colors[1] = rgb_to_256(5, 0, 3);
    t->dancer_colors[2] = rgb_to_256(5, 0, 4);
    t->dancer_colors[3] = rgb_to_256(4, 0, 5);
    t->dancer_colors[4] = rgb_to_256(2, 0, 5);
    t->dancer_colors[5] = rgb_to_256(0, 2, 5);
    t->dancer_colors[6] = rgb_to_256(0, 4, 5);
    t->dancer_colors[7] = rgb_to_256(0, 5, 5);
    t->dancer_colors[8] = rgb_to_256(0, 5, 4);
    t->dancer_colors[9] = rgb_to_256(2, 5, 5);
    t->shadow_color = rgb_to_256(2, 0, 2);
    t->ground_color = rgb_to_256(5, 0, 3);
    t->bass_color = rgb_to_256(5, 0, 3);
    t->mid_color = rgb_to_256(0, 5, 5);
    t->treble_color = rgb_to_256(3, 0, 5);
    t->info_color = rgb_to_256(5, 2, 4);
    t->bpm_color = rgb_to_256(0, 5, 5);
    t->background = -1;
}

static void setup_theme_ember(ThemeColors *t) {
    /* Ember: Glowing coals - dark red with orange/yellow highlights */
    t->dancer_colors[0] = rgb_to_256(1, 0, 0);
    t->dancer_colors[1] = rgb_to_256(2, 0, 0);
    t->dancer_colors[2] = rgb_to_256(2, 0, 0);
    t->dancer_colors[3] = rgb_to_256(3, 0, 0);
    t->dancer_colors[4] = rgb_to_256(4, 0, 0);
    t->dancer_colors[5] = rgb_to_256(4, 1, 0);
    t->dancer_colors[6] = rgb_to_256(5, 2, 0);
    t->dancer_colors[7] = rgb_to_256(5, 3, 0);
    t->dancer_colors[8] = rgb_to_256(5, 4, 0);
    t->dancer_colors[9] = rgb_to_256(5, 5, 2);
    t->shadow_color = rgb_to_256(1, 0, 0);
    t->ground_color = rgb_to_256(2, 0, 0);
    t->bass_color = rgb_to_256(3, 0, 0);
    t->mid_color = rgb_to_256(5, 2, 0);
    t->treble_color = rgb_to_256(5, 4, 0);
    t->info_color = rgb_to_256(4, 2, 0);
    t->bpm_color = rgb_to_256(5, 3, 0);
    t->background = -1;
}

/* ============ Public functions ============ */

int colors_init(void) {
    if (!has_colors()) {
        return -1;
    }
    
    start_color();
    use_default_colors();
    
    /* Check for 256 color support */
    has_256_colors = (COLORS >= 256);
    
    /* Apply default theme */
    colors_apply_theme(THEME_DEFAULT);
    
    return 0;
}

void colors_apply_theme(ColorTheme theme) {
    /* Set up theme colors */
    switch (theme) {
        case THEME_FIRE:      setup_theme_fire(&current_theme);      break;
        case THEME_ICE:       setup_theme_ice(&current_theme);       break;
        case THEME_NEON:      setup_theme_neon(&current_theme);      break;
        case THEME_MATRIX:    setup_theme_matrix(&current_theme);    break;
        case THEME_SYNTHWAVE: setup_theme_synthwave(&current_theme); break;
        case THEME_MONO:      setup_theme_mono(&current_theme);      break;
        case THEME_AURORA:    setup_theme_aurora(&current_theme);    break;
        case THEME_SUNSET:    setup_theme_sunset(&current_theme);    break;
        case THEME_OCEAN:     setup_theme_ocean(&current_theme);     break;
        case THEME_CANDY:     setup_theme_candy(&current_theme);     break;
        case THEME_VAPOR:     setup_theme_vapor(&current_theme);     break;
        case THEME_EMBER:     setup_theme_ember(&current_theme);     break;
        default:              setup_theme_default(&current_theme);   break;
    }
    
    /* Initialize color pairs */
    short bg = current_theme.background;
    
    /* Dancer gradient pairs */
    for (int i = 0; i < GRADIENT_STEPS; i++) {
        if (has_256_colors) {
            init_pair(COLOR_PAIR_DANCER_BASE + i, current_theme.dancer_colors[i], bg);
        } else {
            /* Fallback to basic colors */
            init_pair(COLOR_PAIR_DANCER_BASE + i, COLOR_CYAN, bg);
        }
    }
    
    /* Shadow pairs */
    for (int i = 0; i < GRADIENT_STEPS; i++) {
        if (has_256_colors) {
            init_pair(COLOR_PAIR_SHADOW_BASE + i, current_theme.shadow_color, bg);
        } else {
            init_pair(COLOR_PAIR_SHADOW_BASE + i, COLOR_BLACK, bg);
        }
    }
    
    /* Ground */
    if (has_256_colors) {
        init_pair(COLOR_PAIR_GROUND, current_theme.ground_color, bg);
    } else {
        init_pair(COLOR_PAIR_GROUND, COLOR_WHITE, bg);
    }
    
    /* Frequency bars */
    if (has_256_colors) {
        init_pair(COLOR_PAIR_BAR_BASS, current_theme.bass_color, bg);
        init_pair(COLOR_PAIR_BAR_MID, current_theme.mid_color, bg);
        init_pair(COLOR_PAIR_BAR_TREBLE, current_theme.treble_color, bg);
    } else {
        init_pair(COLOR_PAIR_BAR_BASS, COLOR_RED, bg);
        init_pair(COLOR_PAIR_BAR_MID, COLOR_GREEN, bg);
        init_pair(COLOR_PAIR_BAR_TREBLE, COLOR_BLUE, bg);
    }
    
    /* UI */
    if (has_256_colors) {
        init_pair(COLOR_PAIR_INFO, current_theme.info_color, bg);
        init_pair(COLOR_PAIR_BPM, current_theme.bpm_color, bg);
    } else {
        init_pair(COLOR_PAIR_INFO, COLOR_WHITE, bg);
        init_pair(COLOR_PAIR_BPM, COLOR_YELLOW, bg);
    }
}

int colors_get_dancer_pair(float energy) {
    if (energy < 0.0f) energy = 0.0f;
    if (energy > 1.0f) energy = 1.0f;
    int idx = (int)(energy * (GRADIENT_STEPS - 1));
    return COLOR_PAIR_DANCER_BASE + idx;
}

int colors_get_shadow_pair(float energy) {
    (void)energy;  /* Shadow is uniform color */
    return COLOR_PAIR_SHADOW_BASE;
}

int colors_get_ground_pair(void) {
    return COLOR_PAIR_GROUND;
}

int colors_get_bass_pair(float intensity) {
    (void)intensity;
    return COLOR_PAIR_BAR_BASS;
}

int colors_get_mid_pair(float intensity) {
    (void)intensity;
    return COLOR_PAIR_BAR_MID;
}

int colors_get_treble_pair(float intensity) {
    (void)intensity;
    return COLOR_PAIR_BAR_TREBLE;
}

int colors_get_info_pair(void) {
    return COLOR_PAIR_INFO;
}

int colors_get_bpm_pair(void) {
    return COLOR_PAIR_BPM;
}

int colors_get_energy_pair(float energy) {
    return colors_get_dancer_pair(energy);
}

int colors_has_256(void) {
    return has_256_colors;
}

const char* colors_get_theme_preview(ColorTheme theme) {
    switch (theme) {
        case THEME_FIRE:      return "🔥 Fire (red→orange→yellow)";
        case THEME_ICE:       return "❄️  Ice (blue→cyan→white)";
        case THEME_NEON:      return "💜 Neon (magenta→cyan→green)";
        case THEME_MATRIX:    return "💚 Matrix (green shades)";
        case THEME_SYNTHWAVE: return "🌆 Synthwave (pink→purple)";
        case THEME_MONO:      return "⬜ Mono (grayscale)";
        case THEME_AURORA:    return "🌌 Aurora (northern lights)";
        case THEME_SUNSET:    return "🌅 Sunset (orange/pink/purple)";
        case THEME_OCEAN:     return "🌊 Ocean (deep teal/aqua)";
        case THEME_CANDY:     return "🍬 Candy (pastel rainbow)";
        case THEME_VAPOR:     return "📼 Vapor (hot pink/cyan)";
        case THEME_EMBER:     return "🔴 Ember (glowing coals)";
        default:              return "🎨 Default (cyan gradient)";
    }
}
