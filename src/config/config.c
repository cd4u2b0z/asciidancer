/*
 * Configuration system for braille-boogie v3.2
 * INI file parser and runtime settings
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <errno.h>

#define CONFIG_MAX_LINE 512
#define CONFIG_MAX_PATH 1024

/* Helper to create directory recursively */
static int mkdir_recursive(const char *path) {
    char tmp[CONFIG_MAX_PATH];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

/* Helper to trim whitespace */
static char *trim(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
    return str;
}

/* Initialize config with default values */
void config_init(Config *cfg) {
    if (!cfg) return;
    
    memset(cfg, 0, sizeof(Config));
    
    /* Audio settings */
    strncpy(cfg->audio_source, "auto", sizeof(cfg->audio_source) - 1);
    cfg->sample_rate = 44100;
    cfg->use_pipewire = 1;
    
    /* Visual settings */
    cfg->theme = THEME_DEFAULT;
    cfg->sensitivity = 1.0f;
    cfg->show_ground = 1;
    cfg->show_shadow = 1;
    
    /* Terminal settings */
    cfg->target_fps = 60;
    cfg->auto_scale = 1;
    
    /* Animation settings */
    cfg->smoothing = 0.8f;
    cfg->energy_decay = 0.95f;
    
    /* Debug */
    cfg->debug_mode = 0;
}

/* Get default config file path */
char *config_get_default_path(void) {
    static char path[CONFIG_MAX_PATH];
    const char *home = getenv("HOME");
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    
    if (xdg_config && xdg_config[0]) {
        snprintf(path, sizeof(path), "%s/braille-boogie/config.ini", xdg_config);
    } else if (home && home[0]) {
        snprintf(path, sizeof(path), "%s/.config/braille-boogie/config.ini", home);
    } else {
        return NULL;
    }
    
    return path;
}

/* Parse theme name to enum */
ColorTheme config_theme_from_name(const char *name) {
    if (!name) return THEME_DEFAULT;
    
    if (strcasecmp(name, "fire") == 0) return THEME_FIRE;
    if (strcasecmp(name, "ice") == 0) return THEME_ICE;
    if (strcasecmp(name, "neon") == 0) return THEME_NEON;
    if (strcasecmp(name, "matrix") == 0) return THEME_MATRIX;
    if (strcasecmp(name, "synthwave") == 0) return THEME_SYNTHWAVE;
    if (strcasecmp(name, "mono") == 0) return THEME_MONO;
    if (strcasecmp(name, "aurora") == 0) return THEME_AURORA;
    if (strcasecmp(name, "sunset") == 0) return THEME_SUNSET;
    if (strcasecmp(name, "ocean") == 0) return THEME_OCEAN;
    if (strcasecmp(name, "candy") == 0) return THEME_CANDY;
    if (strcasecmp(name, "vapor") == 0) return THEME_VAPOR;
    if (strcasecmp(name, "ember") == 0) return THEME_EMBER;
    
    return THEME_DEFAULT;
}

/* Get theme name from enum */
static const char *config_theme_name(ColorTheme theme) {
    switch (theme) {
        case THEME_FIRE:      return "fire";
        case THEME_ICE:       return "ice";
        case THEME_NEON:      return "neon";
        case THEME_MATRIX:    return "matrix";
        case THEME_SYNTHWAVE: return "synthwave";
        case THEME_MONO:      return "mono";
        case THEME_AURORA:    return "aurora";
        case THEME_SUNSET:    return "sunset";
        case THEME_OCEAN:     return "ocean";
        case THEME_CANDY:     return "candy";
        case THEME_VAPOR:     return "vapor";
        case THEME_EMBER:     return "ember";
        case THEME_COUNT:     return "default";
        default:              return "default";
    }
}

/* Load config from INI file */
int config_load(Config *cfg, const char *path) {
    if (!cfg || !path) return -1;
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char line[CONFIG_MAX_LINE];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), f)) {
        char *trimmed = trim(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        
        /* Section header */
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
            }
            continue;
        }
        
        /* Key=value pair */
        char *eq = strchr(trimmed, '=');
        if (!eq) continue;
        
        *eq = '\0';
        const char *key = trim(trimmed);
        const char *value = trim(eq + 1);
        
        /* Parse based on section */
        if (strcmp(section, "audio") == 0) {
            if (strcmp(key, "source") == 0) {
                strncpy(cfg->audio_source, value, sizeof(cfg->audio_source) - 1);
            } else if (strcmp(key, "sample_rate") == 0) {
                cfg->sample_rate = atoi(value);
            } else if (strcmp(key, "use_pipewire") == 0) {
                cfg->use_pipewire = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        } else if (strcmp(section, "visual") == 0) {
            if (strcmp(key, "theme") == 0) {
                cfg->theme = config_theme_from_name(value);
            } else if (strcmp(key, "sensitivity") == 0) {
                cfg->sensitivity = (float)atof(value);
            } else if (strcmp(key, "show_ground") == 0) {
                cfg->show_ground = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "show_shadow") == 0) {
                cfg->show_shadow = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        } else if (strcmp(section, "terminal") == 0) {
            if (strcmp(key, "fps") == 0) {
                cfg->target_fps = atoi(value);
            } else if (strcmp(key, "auto_scale") == 0) {
                cfg->auto_scale = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        } else if (strcmp(section, "animation") == 0) {
            if (strcmp(key, "smoothing") == 0) {
                cfg->smoothing = (float)atof(value);
            } else if (strcmp(key, "energy_decay") == 0) {
                cfg->energy_decay = (float)atof(value);
            }
        } else if (strcmp(section, "debug") == 0) {
            if (strcmp(key, "enabled") == 0) {
                cfg->debug_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        }
    }
    
    fclose(f);
    return 0;
}

/* Save config to INI file */
static int config_save(const Config *cfg, const char *path) {
    if (!cfg || !path) return -1;
    
    /* Create directory if needed */
    char dir[CONFIG_MAX_PATH];
    strncpy(dir, path, sizeof(dir) - 1);
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir_recursive(dir);
    }
    
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    
    fprintf(f, "# braille-boogie configuration v3.2\n\n");
    
    fprintf(f, "[audio]\n");
    fprintf(f, "source = %s\n", cfg->audio_source);
    fprintf(f, "sample_rate = %d\n", cfg->sample_rate);
    fprintf(f, "use_pipewire = %s\n\n", cfg->use_pipewire ? "true" : "false");
    
    fprintf(f, "[visual]\n");
    fprintf(f, "theme = %s\n", config_theme_name(cfg->theme));
    fprintf(f, "sensitivity = %.2f\n", cfg->sensitivity);
    fprintf(f, "show_ground = %s\n", cfg->show_ground ? "true" : "false");
    fprintf(f, "show_shadow = %s\n\n", cfg->show_shadow ? "true" : "false");
    
    fprintf(f, "[terminal]\n");
    fprintf(f, "fps = %d\n", cfg->target_fps);
    fprintf(f, "auto_scale = %s\n\n", cfg->auto_scale ? "true" : "false");
    
    fprintf(f, "[animation]\n");
    fprintf(f, "smoothing = %.2f\n", cfg->smoothing);
    fprintf(f, "energy_decay = %.2f\n\n", cfg->energy_decay);
    
    fprintf(f, "[debug]\n");
    fprintf(f, "enabled = %s\n", cfg->debug_mode ? "true" : "false");
    
    fclose(f);
    return 0;
}

/* Create default config file if it doesn't exist */
int config_create_default(const char *path) {
    if (!path) return -1;
    
    /* Check if file already exists */
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 0;  /* Already exists */
    }
    
    /* Create with defaults */
    Config cfg;
    config_init(&cfg);
    return config_save(&cfg, path);
}
