/*
 * Background Particle Effects - ASCII Dancer v3.0
 *
 * Spectacular visual effects using the particle system:
 * - Spectral wave pulses
 * - Ambient particle fields
 * - Energy auras around dancer
 * - Beat-synchronized bursts
 * - Frequency-reactive ribbons
 */

#ifndef BACKGROUND_FX_H
#define BACKGROUND_FX_H

#include <stdbool.h>
#include "particles.h"

typedef enum {
    BG_NONE = 0,
    BG_AMBIENT_FIELD,      /* Floating particles in background */
    BG_SPECTRAL_WAVES,     /* Wave pulses from bottom */
    BG_ENERGY_AURA,        /* Glow around dancer */
    BG_BEAT_BURST,         /* Explosions on beats */
    BG_FREQUENCY_RIBBONS,  /* Vertical frequency bars */
    BG_PARTICLE_RAIN,      /* Falling particles */
    BG_SPIRAL_VORTEX,      /* Rotating spiral effect */
    BG_COUNT
} BackgroundFXType;

typedef struct {
    /* Effect control */
    BackgroundFXType type;
    bool enabled;
    float intensity;        /* 0-1, overall strength */
    float speed;            /* Speed multiplier */
    
    /* Particle system reference */
    ParticleSystem *particles;
    
    /* Ambient field */
    struct {
        int particle_count;
        float drift_speed;
        float twinkle_rate;
    } ambient;
    
    /* Spectral waves */
    struct {
        float wave_phase;
        float wave_speed;
        int active_waves;
        float band_heights[6];  /* Frequency band amplitudes */
    } spectral;
    
    /* Energy aura */
    struct {
        int dancer_x;
        int dancer_y;
        float radius;
        float pulse_phase;
        int ring_particles;
    } aura;
    
    /* Beat burst */
    struct {
        double last_burst_time;
        float burst_cooldown;
        int burst_particles;
        float burst_radius;
    } burst;
    
    /* Frequency ribbons */
    struct {
        float ribbon_x[6];      /* X position for each band */
        float ribbon_height[6]; /* Current height */
        int particles_per_band;
    } ribbons;
    
    /* Rain effect */
    struct {
        int drop_count;
        float fall_speed;
        float spawn_rate;
    } rain;
    
    /* Spiral vortex */
    struct {
        float rotation;
        float rotation_speed;
        int spiral_arms;
        float arm_length;
    } vortex;
    
    /* Timing */
    double current_time;
    float dt;
    
} BackgroundFX;

/* ============ Lifecycle ============ */

/* Create background effects system */
BackgroundFX* background_fx_create(ParticleSystem *particles);

/* Destroy system */
void background_fx_destroy(BackgroundFX *fx);

/* Enable/disable effects */
void background_fx_enable(BackgroundFX *fx, bool enabled);

/* Set effect type */
void background_fx_set_type(BackgroundFX *fx, BackgroundFXType type);

/* Set intensity (0-1) */
void background_fx_set_intensity(BackgroundFX *fx, float intensity);

/* ============ Updates ============ */

/* Update per frame */
void background_fx_update(BackgroundFX *fx, float dt);

/* Update with audio data */
void background_fx_update_audio(BackgroundFX *fx,
                               float energy,
                               float bass,
                               float mid,
                               float treble,
                               bool beat_hit);

/* Update frequency band data (for spectral/ribbon effects) */
void background_fx_update_bands(BackgroundFX *fx,
                               float sub_bass,
                               float bass,
                               float low_mid,
                               float mid,
                               float high_mid,
                               float treble);

/* Update dancer position (for aura effect) */
void background_fx_update_dancer_pos(BackgroundFX *fx, int x, int y);

/* ============ Effect Generators ============ */

/* Generate ambient floating particles */
void background_fx_generate_ambient(BackgroundFX *fx);

/* Generate spectral wave pulse */
void background_fx_generate_wave(BackgroundFX *fx, float energy);

/* Generate energy aura around dancer */
void background_fx_generate_aura(BackgroundFX *fx, float energy);

/* Generate beat burst explosion */
void background_fx_generate_burst(BackgroundFX *fx, float energy);

/* Generate frequency ribbons */
void background_fx_generate_ribbons(BackgroundFX *fx);

/* Generate particle rain */
void background_fx_generate_rain(BackgroundFX *fx);

/* Generate spiral vortex */
void background_fx_generate_vortex(BackgroundFX *fx);

/* ============ Queries ============ */

/* Get current effect type */
BackgroundFXType background_fx_get_type(BackgroundFX *fx);

/* Get effect type name */
const char* background_fx_get_type_name(BackgroundFXType type);

/* Check if enabled */
bool background_fx_is_enabled(BackgroundFX *fx);

#endif /* BACKGROUND_FX_H */
