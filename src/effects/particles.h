/*
 * Particle System for ASCII Dancer
 * 
 * Features:
 * - Spark particles on bass hits
 * - Physics simulation (velocity, gravity, drag)
 * - Lifetime and fade out
 * - Configurable spawn patterns
 */

#ifndef PARTICLES_H
#define PARTICLES_H

#include <stdbool.h>
#include "../braille/braille_canvas.h"

/* Maximum particles in system */
#define MAX_PARTICLES 256

/* Particle spawn patterns */
typedef enum {
    SPAWN_POINT,        /* Single point emission */
    SPAWN_BURST,        /* Radial burst from point */
    SPAWN_FOUNTAIN,     /* Upward fountain */
    SPAWN_EXPLOSION,    /* 360-degree explosion */
    SPAWN_RAIN,         /* Falling from top */
    SPAWN_SPARKLE       /* Random sparkles around point */
} SpawnPattern;

/* Particle types affect rendering */
typedef enum {
    PARTICLE_SPARK,     /* Single bright pixel */
    PARTICLE_DOT,       /* Small dot */
    PARTICLE_TRAIL,     /* Leaves trail behind */
    PARTICLE_STAR       /* Star shape (5 pixels) */
} ParticleType;

/* Individual particle */
typedef struct {
    float x, y;         /* Position */
    float vx, vy;       /* Velocity */
    float ax, ay;       /* Acceleration (for custom forces) */
    float lifetime;     /* Remaining life (0-1) */
    float max_life;     /* Initial lifetime */
    float size;         /* Size multiplier */
    float brightness;   /* 0-1, affects color intensity */
    ParticleType type;
    int color_index;    /* Index into color gradient */
    bool active;
} Particle;

/* Emitter configuration */
typedef struct {
    float x, y;             /* Emission point */
    float spread_angle;     /* Emission cone angle (radians) */
    float base_angle;       /* Base emission direction */
    float min_speed;        /* Minimum particle speed */
    float max_speed;        /* Maximum particle speed */
    float min_life;         /* Minimum lifetime (seconds) */
    float max_life;         /* Maximum lifetime (seconds) */
    float gravity;          /* Downward acceleration */
    float drag;             /* Velocity damping (0-1) */
    float size_min;         /* Minimum particle size */
    float size_max;         /* Maximum particle size */
    SpawnPattern pattern;
    ParticleType type;
    int color_base;         /* Base color for particles */
    bool fade_out;          /* Fade brightness over lifetime */
    bool shrink;            /* Shrink size over lifetime */
} EmitterConfig;

/* Particle system state */
typedef struct {
    Particle particles[MAX_PARTICLES];
    int active_count;
    int next_slot;          /* Round-robin allocation */
    
    /* Global settings */
    float world_gravity;
    float world_drag;
    int canvas_width;
    int canvas_height;
    
    /* Body exclusion zone (particles avoid this area) */
    float body_center_x;
    float body_center_y;
    float body_head_y;      /* Top of head */
    float body_foot_y;      /* Bottom of feet */
    float body_radius;      /* Horizontal exclusion radius */
    bool body_mask_enabled;
    
    /* Max particle cap for visual clarity */
    int max_active;
    
    /* Silence fade multiplier */
    float fade_multiplier;
    
    /* Statistics */
    int total_spawned;
    int total_died;
    
    /* Enable/disable */
    bool enabled;
} ParticleSystem;

/* Create/destroy */
ParticleSystem* particles_create(int canvas_width, int canvas_height);
void particles_destroy(ParticleSystem *ps);

/* Spawn particles */
void particles_spawn(ParticleSystem *ps, EmitterConfig *config, int count);
void particles_spawn_at(ParticleSystem *ps, float x, float y, 
                        SpawnPattern pattern, int count, float energy);

/* Preset emitters for common effects */
void particles_emit_bass_hit(ParticleSystem *ps, float x, float y, float intensity);
void particles_emit_treble_sparkle(ParticleSystem *ps, float x, float y, float intensity);
void particles_emit_beat_burst(ParticleSystem *ps, float x, float y, float intensity);
void particles_emit_foot_stomp(ParticleSystem *ps, float x, float y, float intensity);
void particles_emit_hand_flourish(ParticleSystem *ps, float x, float y, float vx, float vy);

/* Update physics */
void particles_update(ParticleSystem *ps, float dt);

/* Render to canvas */
void particles_render(ParticleSystem *ps, BrailleCanvas *canvas);

/* Control */
void particles_clear(ParticleSystem *ps);
void particles_set_enabled(ParticleSystem *ps, bool enabled);
bool particles_is_enabled(ParticleSystem *ps);

/* Body masking - prevents particles from obscuring the character */
void particles_set_body_mask(ParticleSystem *ps, float center_x, float center_y,
                             float head_y, float foot_y, float radius);
void particles_set_fade_multiplier(ParticleSystem *ps, float mult);

/* Statistics */
int particles_get_active_count(ParticleSystem *ps);

#endif /* PARTICLES_H */
