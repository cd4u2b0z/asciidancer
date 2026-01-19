/*
 * Braille Dancer - High-resolution dancer using braille rendering
 * Integrates skeleton animation with the existing dancer interface
 * v2.2: Added particle system, motion trails, visual enhancements
 * v2.4: Polish pass - body masking, reduced particle density, better proportions
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
#include "../effects/particles.h"  /* For body mask functions */

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
static float bass_threshold = 0.15f;
static float treble_threshold = 0.12f;

/* Continuous particle spawning */
static float particle_spawn_timer = 0.0f;
static float particle_spawn_rate = 0.05f;  /* Seconds between spawns */

/* Rhythm tracking (v2.3) */
static float current_beat_phase = 0.0f;
static float current_bpm = 120.0f;
static bool rhythm_onset = false;
static float rhythm_onset_strength = 0.0f;

/* Pixel dimensions */
static int pixel_width = 0;
static int pixel_height = 0;

/* Ground and shadow (reflection) settings */
static bool show_ground = false;
static bool show_shadow = false;

/* Ground position */
static int ground_y = 0;  /* Pixel y-coordinate of ground line */

/* Convert joint normalized coords (0-1) to pixel coords */
static inline float joint_to_pixel_x(float x) {
    /* Joint x is 0-1 centered at 0.5, convert to pixel coords */
    return (x - 0.5f) * (pixel_width * 0.75f) + (pixel_width / 2.0f);
}

static inline float joint_to_pixel_y(float y) {
    /* Joint y is 0-1 from top, convert to pixel coords with headroom */
    return y * (pixel_height * 0.70f) + (pixel_height * 0.18f);
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
    
    /* Ground line is at the bottom of the canvas */
    ground_y = pixel_height - 3;
    
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
    
    /* Render ground line (before dancer so it's behind) */
    if (show_ground) {
        for (int x = 0; x < pixel_width; x++) {
            braille_set_pixel(canvas, x, ground_y, true);
        }
    }
    
    /* Render shadow/reflection (mirrored silhouette below ground) */
    if (show_shadow && skeleton) {
        const Joint *joints = skeleton_dancer_get_joints(skeleton);
        if (joints) {
            /* Draw connecting lines for shadow silhouette */
            /* Body connections - create a proper shadow shape */
            int shadow_pairs[][2] = {
                {0, 1},   /* Head to neck */
                {1, 2},   /* Neck to hip center */
                {1, 3}, {1, 4},   /* Neck to shoulders */
                {3, 5}, {4, 6},   /* Shoulders to elbows */
                {5, 7}, {6, 8},   /* Elbows to hands */
                {2, 9}, {2, 10},  /* Hips to knees */
                {9, 11}, {10, 12} /* Knees to feet */
            };
            int num_pairs = sizeof(shadow_pairs) / sizeof(shadow_pairs[0]);
            
            for (int p = 0; p < num_pairs; p++) {
                int i1 = shadow_pairs[p][0];
                int i2 = shadow_pairs[p][1];
                
                float px1 = joint_to_pixel_x(joints[i1].x);
                float py1 = joint_to_pixel_y(joints[i1].y);
                float px2 = joint_to_pixel_x(joints[i2].x);
                float py2 = joint_to_pixel_y(joints[i2].y);
                
                /* Mirror y across ground line with perspective squash */
                float dist1 = ground_y - py1;
                float dist2 = ground_y - py2;
                float mirror_y1 = ground_y + dist1 * 0.40f;  /* Squashed reflection */
                float mirror_y2 = ground_y + dist2 * 0.40f;
                
                /* Draw shadow line with thickness */
                if (mirror_y1 > ground_y && mirror_y1 < pixel_height &&
                    mirror_y2 > ground_y && mirror_y2 < pixel_height) {
                    /* Main line */
                    braille_draw_line(canvas, (int)px1, (int)mirror_y1,
                                     (int)px2, (int)mirror_y2);
                    /* Thicker line - offset by 1 */
                    braille_draw_line(canvas, (int)px1 + 1, (int)mirror_y1,
                                     (int)px2 + 1, (int)mirror_y2);
                }
            }
            
            /* Draw shadow head (larger blob for visibility) */
            float head_x = joint_to_pixel_x(joints[0].x);
            float head_y = joint_to_pixel_y(joints[0].y);
            float head_mirror_y = ground_y + (ground_y - head_y) * 0.40f;
            if (head_mirror_y > ground_y && head_mirror_y < pixel_height - 3) {
                /* Small circle for head shadow */
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -2; dx <= 2; dx++) {
                        if (dx*dx + dy*dy <= 4) {  /* Circle of radius ~2 */
                            braille_set_pixel(canvas, (int)head_x + dx, 
                                            (int)head_mirror_y + dy, true);
                        }
                    }
                }
            }
        }
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

/* Ground and shadow (reflection) controls */
void dancer_set_ground(bool enabled) {
    show_ground = enabled;
}

void dancer_set_shadow(bool enabled) {
    show_shadow = enabled;
}

bool dancer_get_ground(void) {
    return show_ground;
}

bool dancer_get_shadow(void) {
    return show_shadow;
}

/* Visualizer removed - stubs for compatibility */
void dancer_set_visualizer(bool enabled) {
    (void)enabled;
}

bool dancer_get_visualizer(void) {
    return false;
}

void dancer_update_spectrum(float *spectrum, int num_bars) {
    (void)spectrum;
    (void)num_bars;
}

int dancer_get_particle_count(void) {
    return (effects && effects->particles) ? particles_get_active_count(effects->particles) : 0;
}

/* v3.0: Get particle system for background effects */
ParticleSystem* dancer_get_particle_system(void) {
    return effects ? effects_get_particle_system(effects) : NULL;
}

void calculate_bands(const double *cava_out, int num_bars,
                     double *bass, double *mid, double *treble) {
    *bass = *mid = *treble = 0.0;
    if (num_bars < 3) return;
    
    /* Improved frequency band separation (v2.3)
     * With 24 bars at 44.1kHz, each bar covers ~920Hz
     * Bass: 0-250Hz (bars 0-3, ~4 bars, weighted heavily)
     * Low-mid: 250-500Hz (bars 3-5)
     * Mid: 500-2000Hz (bars 5-10)
     * High-mid: 2000-4000Hz (bars 10-14)
     * Treble: 4000Hz+ (bars 14+)
     */
    int sub_bass_end = num_bars / 8;       /* Sub-bass: ~0-150Hz */
    int bass_end = num_bars / 4;            /* Bass: ~150-300Hz */
    int low_mid_end = num_bars * 3 / 8;     /* Low-mid: ~300-600Hz */
    int mid_end = num_bars / 2;             /* Mid: ~600-1200Hz */
    int high_mid_end = num_bars * 5 / 8;    /* High-mid: ~1200-2400Hz */
    
    /* Bass: combine sub-bass and bass with weighting */
    double sub_bass = 0, low_bass = 0;
    for (int i = 0; i < sub_bass_end; i++) sub_bass += cava_out[i];
    for (int i = sub_bass_end; i < bass_end; i++) low_bass += cava_out[i];
    *bass = (sub_bass * 1.2 + low_bass) / bass_end;  /* Weight sub-bass more */
    
    /* Mid: combine low-mid and mid */
    double low_mid = 0, core_mid = 0;
    for (int i = bass_end; i < low_mid_end; i++) low_mid += cava_out[i];
    for (int i = low_mid_end; i < mid_end; i++) core_mid += cava_out[i];
    *mid = (low_mid + core_mid) / (mid_end - bass_end);
    
    /* Treble: combine high-mid and treble */
    double high_mid = 0, high_treble = 0;
    for (int i = mid_end; i < high_mid_end; i++) high_mid += cava_out[i];
    for (int i = high_mid_end; i < num_bars; i++) high_treble += cava_out[i];
    *treble = (high_mid * 0.8 + high_treble * 1.2) / (num_bars - mid_end);  /* Weight highs more */
    
    /* Normalize */
    if (*bass > 1.0) *bass = 1.0;
    if (*mid > 1.0) *mid = 1.0;
    if (*treble > 1.0) *treble = 1.0;
}

/* === Rhythm-aware update (v2.3) === */

void dancer_update_with_rhythm(struct dancer_state *state, 
                               double bass, double mid, double treble,
                               float beat_phase, float bpm, 
                               bool onset_detected, float onset_strength) {
    if (!skeleton) return;
    
    current_beat_phase = beat_phase;
    current_bpm = bpm;
    rhythm_onset = onset_detected;
    rhythm_onset_strength = onset_strength;
    
    /* Calculate dt (approximately 60fps = 0.0167s) */
    float dt = 0.0167f;
    
    /* Note: visualizer is now updated separately via dancer_update_spectrum() */
    
    /* Smooth audio input for dancer (separate from visualizer) */
    double smooth = 0.88;
    state->bass_intensity = state->bass_intensity * smooth + bass * (1.0 - smooth);
    state->mid_intensity = state->mid_intensity * smooth + mid * (1.0 - smooth);
    state->treble_intensity = state->treble_intensity * smooth + treble * (1.0 - smooth);
    
    /* Track bass/treble velocity for transient detection */
    bass_velocity = (float)state->bass_intensity - last_bass;
    treble_velocity = (float)state->treble_intensity - last_treble;
    
    /* Calculate overall energy */
    float energy = (state->bass_intensity + state->mid_intensity + state->treble_intensity) / 3.0f;
    
    /* Update particle spawn timer */
    particle_spawn_timer += dt;
    
    /* Continuous particle spawning based on energy level */
    float spawn_interval = particle_spawn_rate / (0.5f + energy * 2.0f);  /* Faster when louder */
    
    if (effects && particle_spawn_timer >= spawn_interval && energy > 0.05f) {
        particle_spawn_timer = 0.0f;
        
        /* Spawn particles based on which band is dominant */
        if (state->bass_intensity > state->treble_intensity && state->bass_intensity > bass_threshold) {
            /* Bass-driven particles from feet */
            float foot_x = (skeleton->current[JOINT_FOOT_L].x + skeleton->current[JOINT_FOOT_R].x) / 2;
            float foot_y = skeleton->current[JOINT_FOOT_L].y;
            effects_on_bass_hit(effects, (float)state->bass_intensity * 0.5f,
                               joint_to_pixel_x(foot_x), joint_to_pixel_y(foot_y));
        } else if (state->treble_intensity > treble_threshold) {
            /* Treble-driven sparkles from hands */
            float hand_x = skeleton->current[JOINT_HAND_R].x;
            float hand_y = skeleton->current[JOINT_HAND_R].y;
            effects_on_treble_spike(effects, (float)state->treble_intensity * 0.5f,
                                   joint_to_pixel_x(hand_x), joint_to_pixel_y(hand_y));
        }
    }
    
    /* Strong transient detection - burst on velocity spikes */
    if (effects && bass_velocity > 0.08f && state->bass_intensity > bass_threshold) {
        float foot_x = (skeleton->current[JOINT_FOOT_L].x + skeleton->current[JOINT_FOOT_R].x) / 2;
        float foot_y = skeleton->current[JOINT_FOOT_L].y;
        effects_on_bass_hit(effects, (float)state->bass_intensity, 
                           joint_to_pixel_x(foot_x), joint_to_pixel_y(foot_y));
    }
    
    /* Treble spike burst */
    if (effects && treble_velocity > 0.08f && state->treble_intensity > treble_threshold) {
        float hand_x = skeleton->current[JOINT_HAND_R].x;
        float hand_y = skeleton->current[JOINT_HAND_R].y;
        effects_on_treble_spike(effects, (float)state->treble_intensity, 
                               joint_to_pixel_x(hand_x), joint_to_pixel_y(hand_y));
    }
    
    /* Rhythm onset detection - burst particles on detected onsets */
    if (effects && rhythm_onset && rhythm_onset_strength > 0.3f) {
        float center_x = skeleton->current[JOINT_HIP_CENTER].x;
        float center_y = skeleton->current[JOINT_HIP_CENTER].y;
        effects_on_beat(effects, rhythm_onset_strength,
                       joint_to_pixel_x(center_x), joint_to_pixel_y(center_y));
    }
    
    /* Beat phase pulse - small burst near beat (phase close to 0) */
    static float last_phase = 0;
    static float note_timer = 0;
    note_timer += dt;
    
    if (effects && energy > 0.15f && beat_phase < 0.1f && last_phase > 0.9f) {
        float center_x = skeleton->current[JOINT_HIP_CENTER].x;
        float center_y = skeleton->current[JOINT_HIP_CENTER].y;
        effects_on_beat(effects, energy * 0.7f,
                       joint_to_pixel_x(center_x), joint_to_pixel_y(center_y));
        
        /* Spawn music notes on beats! Lower threshold, more frequent */
        if (effects->particles && energy > 0.25f && note_timer > 0.3f) {
            note_timer = 0;
            /* Spawn from head area - randomize position */
            float head_x = skeleton->current[JOINT_HEAD].x;
            float head_y = skeleton->current[JOINT_HEAD].y;
            int offset_x = (rand() % 30) - 15;
            particles_emit_music_notes(effects->particles,
                                       joint_to_pixel_x(head_x) + offset_x,
                                       joint_to_pixel_y(head_y) - 3,
                                       energy * 1.5f);  /* Boost intensity */
        }
    }
    
    /* Also spawn notes on half-beats at high energy */
    if (effects && effects->particles && energy > 0.5f && 
        beat_phase > 0.45f && beat_phase < 0.55f && note_timer > 0.2f) {
        note_timer = 0;
        float hand_x = (rand() % 2 == 0) ? 
            skeleton->current[JOINT_HAND_L].x : skeleton->current[JOINT_HAND_R].x;
        float hand_y = skeleton->current[JOINT_HAND_L].y;
        particles_emit_music_notes(effects->particles,
                                   joint_to_pixel_x(hand_x),
                                   joint_to_pixel_y(hand_y),
                                   energy);
    }
    last_phase = beat_phase;
    
    /* Clear particles faster when music stops */
    static float silence_timer = 0;
    if (energy < 0.02f) {
        silence_timer += dt;
        /* Fast fade when silent - accelerate particle death */
        if (effects && effects->particles) {
            particles_set_fade_multiplier(effects->particles, 3.0f);
        }
    } else {
        silence_timer = 0;
        /* Normal fade speed when playing */
        if (effects && effects->particles) {
            particles_set_fade_multiplier(effects->particles, 1.0f);
        }
    }
    
    /* Update body mask for particles - keep them away from character center */
    if (effects && effects->particles && skeleton) {
        float head_px = joint_to_pixel_x(skeleton->current[JOINT_HEAD].x);
        float head_py = joint_to_pixel_y(skeleton->current[JOINT_HEAD].y);
        float hip_py = joint_to_pixel_y(skeleton->current[JOINT_HIP_CENTER].y);
        float foot_py = joint_to_pixel_y(skeleton->current[JOINT_FOOT_L].y);
        
        /* Body exclusion radius based on shoulder width */
        float shoulder_l = joint_to_pixel_x(skeleton->current[JOINT_SHOULDER_L].x);
        float shoulder_r = joint_to_pixel_x(skeleton->current[JOINT_SHOULDER_R].x);
        float body_radius = (shoulder_r - shoulder_l) * 0.8f + 4.0f;
        
        particles_set_body_mask(effects->particles, head_px, 
                               (head_py + hip_py) / 2.0f, head_py, foot_py, body_radius);
    }
    
    /* Update effects */
    if (effects) {
        effects_update(effects, dt, (float)state->bass_intensity, 
                      (float)state->treble_intensity, energy);
        
        /* Update trails with joint positions converted to pixels */
        if (effects->trails && pixel_width > 0 && pixel_height > 0) {
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
    
    /* Update skeleton with rhythm-locked animation (only call once!) */
    skeleton_dancer_update_with_phase(skeleton, 
                                      (float)state->bass_intensity,
                                      (float)state->mid_intensity,
                                      (float)state->treble_intensity,
                                      dt, beat_phase, bpm);
    state->phase = skeleton->phase;
}

float dancer_get_beat_phase(void) {
    return current_beat_phase;
}

float dancer_get_bpm(void) {
    return current_bpm;
}

/* ============ v3.1: Energy Override System ============ */

void dancer_adjust_energy(float amount) {
    if (skeleton) {
        skeleton_dancer_adjust_energy(skeleton, amount);
    }
}

void dancer_toggle_energy_lock(void) {
    if (skeleton) {
        skeleton_dancer_toggle_energy_lock(skeleton);
    }
}

float dancer_get_effective_energy(void) {
    if (skeleton) {
        return skeleton_dancer_get_effective_energy(skeleton);
    }
    return 0.5f;
}

bool dancer_is_energy_locked(void) {
    if (skeleton) {
        return skeleton_dancer_is_energy_locked(skeleton);
    }
    return false;
}

float dancer_get_energy_override(void) {
    if (skeleton) {
        return skeleton_dancer_get_energy_override(skeleton);
    }
    return 0.0f;
}

/* ============ v3.1: Spin Control ============ */

void dancer_trigger_spin(int direction) {
    if (skeleton) {
        skeleton_dancer_trigger_spin(skeleton, direction);
    }
}

float dancer_get_facing(void) {
    if (skeleton) {
        return skeleton_dancer_get_facing(skeleton);
    }
    return 0.0f;
}
