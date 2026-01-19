/*
 * Configuration system for asciidancer v2.4
 * INI file parser and runtime settings
 */
#pragma once

#include <stddef.h>

/* Color theme enumeration */
typedef enum {
    THEME_DEFAULT = 0,
    THEME_FIRE,
    THEME_ICE,
    THEME_NEON,
    THEME_MATRIX,
    THEME_SYNTHWAVE,
    THEME_MONO,
    /* v2.4+ new themes */
    THEME_AURORA,       /* Northern lights: green/purple/blue */
    THEME_SUNSET,       /* Warm sunset: orange/pink/purple */
    THEME_OCEAN,        /* Deep sea: teal/blue/aqua */
    THEME_CANDY,        /* Pastel candy: pink/mint/lavender */
    THEME_VAPOR,        /* Vaporwave: hot pink/cyan/purple */
    THEME_EMBER,        /* Glowing coals: dark red/orange glow */
    THEME_COUNT         /* Always last - count of themes */
} ColorTheme;

/* Configuration structure */
typedef struct {
    /* Audio settings */
    char audio_source[256];
    int sample_rate;
    int use_pipewire;       /* 1 = PipeWire, 0 = PulseAudio */
    
    /* Visual settings */
    ColorTheme theme;
    float sensitivity;
    int show_ground;
    int show_shadow;
    
    /* Terminal settings */
    int target_fps;
    int auto_scale;
    
    /* Animation settings */
    float smoothing;
    float energy_decay;
    
    /* Debug */
    int debug_mode;
} Config;

/* Initialize config with default values */
void config_init(Config *cfg);

/* Load config from INI file. Returns 0 on success, -1 on error */
int config_load(Config *cfg, const char *path);

/* Save config to INI file. Returns 0 on success, -1 on error */
int config_save(const Config *cfg, const char *path);

/* Get default config file path (~/.config/asciidancer/config.ini) */
char *config_get_default_path(void);

/* Create default config file if it doesn't exist */
int config_create_default(const char *path);

/* Parse theme name to enum */
ColorTheme config_theme_from_name(const char *name);

/* Get theme name from enum */
const char *config_theme_name(ColorTheme theme);
