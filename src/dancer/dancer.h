// Dancer animation system
// Rhythm-based animation using custom Braille art frames
// v2.2: Effects control API

#pragma once

#include <stdbool.h>

// Frame dimensions
#define FRAME_WIDTH 25
#define FRAME_HEIGHT 13

// Dancer state
struct dancer_state {
    int current_frame;
    double bass_intensity;
    double mid_intensity;
    double treble_intensity;
    double phase;  // Dance rhythm phase
};

// Core dancer API
void dancer_init(struct dancer_state *state);
void dancer_update(struct dancer_state *state, double bass, double mid, double treble);
void dancer_compose_frame(struct dancer_state *state, char *output);
void calculate_bands(const double *cava_out, int num_bars, double *bass, double *mid, double *treble);
void dancer_cleanup(void);

// Effects control (v2.2)
void dancer_set_particles(bool enabled);
void dancer_set_trails(bool enabled);
void dancer_set_breathing(bool enabled);

bool dancer_get_particles(void);
bool dancer_get_trails(void);
bool dancer_get_breathing(void);

int dancer_get_particle_count(void);

// Rhythm-aware update (v2.3) - pass beat phase and BPM for tighter sync
void dancer_update_with_rhythm(struct dancer_state *state,
                               double bass, double mid, double treble,
                               float beat_phase, float bpm,
                               bool onset_detected, float onset_strength);

// Get current rhythm info
float dancer_get_beat_phase(void);
float dancer_get_bpm(void);

// v3.0: Get particle system for background effects
#include "effects/particles.h"
ParticleSystem* dancer_get_particle_system(void);

// Ground and shadow control (v3.0)
void dancer_set_ground(bool enabled);
void dancer_set_shadow(bool enabled);
bool dancer_get_ground(void);
bool dancer_get_shadow(void);

// v3.1: Energy Override System
void dancer_adjust_energy(float amount);
void dancer_toggle_energy_lock(void);
float dancer_get_effective_energy(void);
bool dancer_is_energy_locked(void);
float dancer_get_energy_override(void);

// v3.1: Spin and Facing Control
void dancer_trigger_spin(int direction);  // 1=clockwise, -1=counter-clockwise
float dancer_get_facing(void);
void dancer_set_visualizer(bool enabled);
bool dancer_get_visualizer(void);
void dancer_update_spectrum(float *spectrum, int num_bars);
