/*
 * Braille Dancer - High-resolution dancer using braille rendering
 * Integrates skeleton animation with the existing dancer interface
 * v2.2: Added particle system, motion trails, visual enhancements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <math.h>
#include "../dancer/dancer.h"
#include "braille_canvas.h"
#include "skeleton_dancer.h"
#include "../effects/effects.h"

/* Canvas size in terminal cells */
#define CANVAS_CELLS_W 25
#define CANVAS_CELLS_H 13

static BrailleCanvas *canvas = NULL;
static SkeletonDancer *skeleton = NULL;
static EffectsManager *effects = NULL;
static int initialized = 0;

/* Track audio for effects */
static float last_bass = 0;
static float last_treble = 0;
static float bass_velocity = 0;
static float treble_velocity = 0;

/* Beat detection for effects */
static float bass_threshold = 0.3f;
static float treble_threshold = 0.25f;

/* Pixel dimensions */
static int pixel_width = 0;
static int pixel_height = 0;

/* Convert joint normalized coords (0-1) to pixel coords */
static inline float joint_to_pixel_x(float x) {
    /* Joint x is 0-1 centered at 0.5, convert to pixel coords */
    return (x - 0.5f) * (pixel_width * 0.8f) + (pixel_width / 2.0f);
}

static inline float joint_to_pixel_y(float y) {
    /* Joint y is 0-1 from top, convert to pixel coords */
    return y * (pixel_height * 0.8f) + (pixel_height * 0.1f);
}

int dancer_load_frames(void) {
    if (initialized) return 1;
    
    setlocale(LC_ALL, "");
    
    /* Create braille canvas */
    canvas = braille_canvas_create(CANVAS_CELLS_W, CANVAS_CELLS_H);
    if (!canvas) return -1;
    
    /* Create skeleton dancer */
    skeleton = skeleton_dancer_create(CANVAS_CELLS_W, CANVAS_CELLS_H);
    if (!skeleton) {
        braille_canvas_destroy(canvas);
        return -1;
    }
    
    /* Create effects system */
    pixel_width = CANVAS_CELLS_W * 2;   /* 2 pixels per cell width */
    pixel_height = CANVAS_CELLS_H * 4;  /* 4 pixels per cell height */
    effects = effects_create(pixel_width, pixel_height);
    
    initialized = 1;
    return 1;
}

void dancer_init(struct dancer_state *state) {
    memset(state, 0, sizeof(*state));
    dancer_load_frames();
}

void dancer_cleanup(void) {
    if (effects) {
        effects_destroy(effects);
        effects = NULL;
    }
    if (skeleton) {
        skeleton_dancer_destroy(skeleton);
        skeleton = NULL;
    }
    if (canvas) {
        braille_canvas_destroy(canvas);
        canvas = NULL;
    }
    initialized = 0;
}

void dancer_update(struct dancer_state *state, double bass, double mid, double treble) {
    if (!skeleton) return;
    
    /* Smooth audio input */
    double smooth = 0.88;
    state->bass_intensity = state->bass_intensity * smooth + bass * (1.0 - smooth);
    state->mid_intensity = state->mid_intensity * smooth + mid * (1.0 - smooth);
    state->treble_intensity = state->treble_intensity * smooth + treble * (1.0 - smooth);
    
    /* Calculate dt (approximately 60fps = 0.0167s) */
    float dt = 0.0167f;
    
    /* Track bass/treble velocity for transient detection */
    bass_velocity = (float)state->bass_intensity - last_bass;
    treble_velocity = (float)state->treble_intensity - last_treble;
    
    /* Detect bass hit (rising edge above threshold) */
    if (effects && bass_velocity > 0.05f && state->bass_intensity > bass_threshold) {
        /* Use actual foot joint positions - convert to pixel coords */
        float foot_x = (skeleton->current[JOINT_FOOT_L].x + skeleton->current[JOINT_FOOT_R].x) / 2;
        float foot_y = skeleton->current[JOINT_FOOT_L].y;
        
        effects_on_bass_hit(effects, (float)state->bass_intensity, 
                           joint_to_pixel_x(foot_x), joint_to_pixel_y(foot_y));
    }
    
    /* Detect treble spike */
    if (effects && treble_velocity > 0.05f && state->treble_intensity > treble_threshold) {
        /* Use actual hand joint positions - convert to pixel coords */
        float hand_x = skeleton->current[JOINT_HAND_R].x;
        float hand_y = skeleton->current[JOINT_HAND_R].y;
        
        effects_on_treble_spike(effects, (float)state->treble_intensity, 
                               joint_to_pixel_x(hand_x), joint_to_pixel_y(hand_y));
    }
    
    /* Detect beat (overall energy spike) */
    float energy = (state->bass_intensity + state->mid_intensity + state->treble_intensity) / 3.0f;
    static float last_energy = 0;
    if (effects && energy - last_energy > 0.1f && energy > 0.3f) {
        /* Burst from center of dancer - convert to pixel coords */
        float center_x = skeleton->current[JOINT_HIP_CENTER].x;
        float center_y = skeleton->current[JOINT_HIP_CENTER].y;
        effects_on_beat(effects, energy, joint_to_pixel_x(center_x), joint_to_pixel_y(center_y));
    }
    last_energy = energy;
    /* Update effects */
    if (effects && skeleton) {
        effects_update(effects, dt, (float)state->bass_intensity, 
                      (float)state->treble_intensity, energy);
        
        /* Update trails with joint positions converted to pixels */
        if (effects->trails && pixel_width > 0 && pixel_height > 0) {
            /* Create converted joint positions in pixel space */
            Joint pixel_joints[MAX_JOINTS];
            for (int i = 0; i < MAX_JOINTS; i++) {
                pixel_joints[i].x = joint_to_pixel_x(skeleton->current[i].x);
                pixel_joints[i].y = joint_to_pixel_y(skeleton->current[i].y);
            }
            trails_update(effects->trails, pixel_joints, MAX_JOINTS, dt);
        }
    }
    
    last_bass = (float)state->bass_intensity;
    last_treble = (float)state->treble_intensity;
    
    /* Update skeleton animation */
    skeleton_dancer_update(skeleton, 
                          (float)state->bass_intensity,
                          (float)state->mid_intensity,
                          (float)state->treble_intensity,
                          dt);
    
    /* Store phase for any external use */
    state->phase = skeleton->phase;
}

void dancer_compose_frame(struct dancer_state *state, char *output) {
    (void)state;
    
    if (!skeleton || !canvas) {
        strcpy(output, "No dancer loaded\n");
        return;
    }
    
    /* Clear canvas */
    braille_canvas_clear(canvas);
    
    /* Render trails first (behind dancer) */
    if (effects && effects->trails && effects->trails->enabled) {
        trails_render(effects->trails, canvas);
    }
    
    /* Render skeleton to braille canvas */
    skeleton_dancer_render(skeleton, canvas);
    
    /* Render particles on top */
    if (effects && effects->particles && effects->particles->enabled) {
        particles_render(effects->particles, canvas);
    }
    
    /* Convert pixels to braille characters */
    braille_canvas_render(canvas);
    
    /* Convert to UTF-8 output */
    char *ptr = output;
    for (int row = 0; row < canvas->cell_height; row++) {
        int len = braille_canvas_to_utf8(canvas, row, ptr, 256);
        ptr += len;
        *ptr++ = '\n';
    }
    *ptr = '\0';
}

/* === Effects control functions === */

void dancer_set_particles(bool enabled) {
    if (effects) effects_set_particles(effects, enabled);
}

void dancer_set_trails(bool enabled) {
    if (effects) effects_set_trails(effects, enabled);
}

void dancer_set_breathing(bool enabled) {
    if (effects) effects_set_breathing(effects, enabled);
}

bool dancer_get_particles(void) {
    return effects ? effects_particles_enabled(effects) : false;
}

bool dancer_get_trails(void) {
    return effects ? effects_trails_enabled(effects) : false;
}

bool dancer_get_breathing(void) {
    return effects ? effects_breathing_enabled(effects) : false;
}

int dancer_get_particle_count(void) {
    return (effects && effects->particles) ? particles_get_active_count(effects->particles) : 0;
}

void calculate_bands(double *cava_out, int num_bars,
                     double *bass, double *mid, double *treble) {
    *bass = *mid = *treble = 0.0;
    if (num_bars < 3) return;
    
    int bass_end = num_bars / 3;
    int mid_end = (num_bars * 2) / 3;
    
    for (int i = 0; i < bass_end; i++) *bass += cava_out[i];
    for (int i = bass_end; i < mid_end; i++) *mid += cava_out[i];
    for (int i = mid_end; i < num_bars; i++) *treble += cava_out[i];
    
    *bass /= bass_end;
    *mid /= (mid_end - bass_end);
    *treble /= (num_bars - mid_end);
    
    /* Normalize */
    if (*bass > 1.0) *bass = 1.0;
    if (*mid > 1.0) *mid = 1.0;
    if (*treble > 1.0) *treble = 1.0;
}
