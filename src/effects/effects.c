/*
 * Effects Manager Implementation
 * 
 * Coordinates all visual effects systems
 */

#include "effects.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EffectsManager* effects_create(int canvas_width, int canvas_height) {
    EffectsManager *fx = calloc(1, sizeof(EffectsManager));
    if (!fx) return NULL;
    
    fx->canvas_width = canvas_width;
    fx->canvas_height = canvas_height;
    fx->enabled = true;
    
    /* Create sub-systems */
    fx->particles = particles_create(canvas_width, canvas_height);
    fx->trails = trails_create();
    
    /* Initialize enhancements */
    fx->enhancements.breathing_enabled = true;
    fx->enhancements.breath_phase = 0;
    fx->enhancements.breath_rate = 0.5f;    /* Half cycle per second */
    fx->enhancements.breath_amplitude = 1.5f;
    
    fx->enhancements.glow_enabled = true;
    fx->enhancements.glow_intensity = 0;
    fx->enhancements.glow_offset = 1;
    
    fx->enhancements.floor_vibe_enabled = true;
    fx->enhancements.floor_vibe_amount = 0;
    fx->enhancements.floor_vibe_decay = 0.85f;
    fx->enhancements.floor_y = canvas_height - 4;
    
    fx->enhancements.shake_enabled = true;
    fx->enhancements.shake_amount = 0;
    fx->enhancements.shake_decay = 0.8f;
    
    return fx;
}

void effects_destroy(EffectsManager *fx) {
    if (!fx) return;
    
    if (fx->particles) particles_destroy(fx->particles);
    if (fx->trails) trails_destroy(fx->trails);
    free(fx);
}

void effects_update(EffectsManager *fx, float dt, float bass, float treble, float energy) {
    if (!fx || !fx->enabled) return;
    
    /* Update particles */
    if (fx->particles) {
        particles_update(fx->particles, dt);
    }
    
    /* Update breathing animation */
    if (fx->enhancements.breathing_enabled) {
        fx->enhancements.breath_phase += dt * fx->enhancements.breath_rate * 2 * M_PI;
        if (fx->enhancements.breath_phase > 2 * M_PI) {
            fx->enhancements.breath_phase -= 2 * M_PI;
        }
    }
    
    /* Update glow intensity based on energy */
    if (fx->enhancements.glow_enabled) {
        float target_glow = (energy > 0.6f) ? (energy - 0.6f) * 2.5f : 0;
        fx->enhancements.glow_intensity = fx->enhancements.glow_intensity * 0.9f + target_glow * 0.1f;
    }
    
    /* Update floor vibration - decays over time */
    if (fx->enhancements.floor_vibe_enabled) {
        fx->enhancements.floor_vibe_amount *= fx->enhancements.floor_vibe_decay;
        
        /* Add bass energy */
        if (bass > 0.5f) {
            fx->enhancements.floor_vibe_amount += (bass - 0.5f) * 4.0f;
        }
        
        /* Clamp */
        if (fx->enhancements.floor_vibe_amount > 3.0f) {
            fx->enhancements.floor_vibe_amount = 3.0f;
        }
    }
    
    /* Update screen shake */
    if (fx->enhancements.shake_enabled) {
        fx->enhancements.shake_amount *= fx->enhancements.shake_decay;
        
        if (fx->enhancements.shake_amount > 0.1f) {
            fx->enhancements.shake_offset_x = (int)((((float)rand() / RAND_MAX) - 0.5f) * fx->enhancements.shake_amount * 2);
            fx->enhancements.shake_offset_y = (int)((((float)rand() / RAND_MAX) - 0.5f) * fx->enhancements.shake_amount * 2);
        } else {
            fx->enhancements.shake_offset_x = 0;
            fx->enhancements.shake_offset_y = 0;
        }
    }
}

void effects_on_bass_hit(EffectsManager *fx, float intensity, float x, float y) {
    if (!fx || !fx->enabled) return;
    
    /* Emit bass particles - no threshold, let particle system decide */
    if (fx->particles) {
        particles_emit_bass_hit(fx->particles, x, y, intensity);
    }
    
    /* Add floor vibration */
    if (fx->enhancements.floor_vibe_enabled && intensity > 0.3f) {
        fx->enhancements.floor_vibe_amount += intensity * 2.0f;
    }
    
    /* Screen shake on heavy bass */
    if (fx->enhancements.shake_enabled && intensity > 0.5f) {
        fx->enhancements.shake_amount += (intensity - 0.5f) * 3.0f;
    }
}

void effects_on_beat(EffectsManager *fx, float intensity, float x, float y) {
    if (!fx || !fx->enabled) return;
    
    /* Burst particles on beat */
    if (fx->particles && intensity > 0.5f) {
        particles_emit_beat_burst(fx->particles, x, y, intensity);
    }
}

void effects_on_treble_spike(EffectsManager *fx, float intensity, float x, float y) {
    if (!fx || !fx->enabled) return;
    
    /* Sparkle effect */
    if (fx->particles && intensity > 0.4f) {
        particles_emit_treble_sparkle(fx->particles, x, y, intensity);
    }
}

void effects_get_breathing_offset(EffectsManager *fx, float *dx, float *dy) {
    if (!fx || !fx->enhancements.breathing_enabled) {
        if (dx) *dx = 0;
        if (dy) *dy = 0;
        return;
    }
    
    /* Gentle vertical breathing motion */
    float offset = sinf(fx->enhancements.breath_phase) * fx->enhancements.breath_amplitude;
    
    if (dx) *dx = 0;
    if (dy) *dy = offset;
}

bool effects_should_render_glow(EffectsManager *fx) {
    return fx && fx->enhancements.glow_enabled && fx->enhancements.glow_intensity > 0.2f;
}

void effects_get_glow_offset(EffectsManager *fx, int *dx, int *dy) {
    if (!fx) {
        if (dx) *dx = 0;
        if (dy) *dy = 0;
        return;
    }
    
    int offset = fx->enhancements.glow_offset;
    if (dx) *dx = offset;
    if (dy) *dy = 0;
}

int effects_get_floor_offset(EffectsManager *fx) {
    if (!fx || !fx->enhancements.floor_vibe_enabled) return 0;
    
    /* Convert vibration amount to pixel offset */
    int offset = (int)(fx->enhancements.floor_vibe_amount + 0.5f);
    
    /* Alternate direction for vibration effect */
    static int vibe_dir = 1;
    vibe_dir = -vibe_dir;
    
    return offset * vibe_dir;
}

void effects_get_shake_offset(EffectsManager *fx, int *dx, int *dy) {
    if (!fx || !fx->enhancements.shake_enabled) {
        if (dx) *dx = 0;
        if (dy) *dy = 0;
        return;
    }
    
    if (dx) *dx = fx->enhancements.shake_offset_x;
    if (dy) *dy = fx->enhancements.shake_offset_y;
}

void effects_render(EffectsManager *fx, BrailleCanvas *canvas) {
    if (!fx || !fx->enabled || !canvas) return;
    
    /* Render trails first (behind dancer) */
    if (fx->trails) {
        trails_render(fx->trails, canvas);
    }
    
    /* Render particles (on top) */
    if (fx->particles) {
        particles_render(fx->particles, canvas);
    }
}

void effects_set_enabled(EffectsManager *fx, bool enabled) {
    if (fx) fx->enabled = enabled;
}

void effects_set_particles(EffectsManager *fx, bool enabled) {
    if (fx && fx->particles) {
        particles_set_enabled(fx->particles, enabled);
    }
}

void effects_set_trails(EffectsManager *fx, bool enabled) {
    if (fx && fx->trails) {
        trails_set_enabled(fx->trails, enabled);
    }
}

void effects_set_breathing(EffectsManager *fx, bool enabled) {
    if (fx) fx->enhancements.breathing_enabled = enabled;
}

void effects_set_glow(EffectsManager *fx, bool enabled) {
    if (fx) fx->enhancements.glow_enabled = enabled;
}

void effects_set_floor_vibe(EffectsManager *fx, bool enabled) {
    if (fx) fx->enhancements.floor_vibe_enabled = enabled;
}

bool effects_particles_enabled(EffectsManager *fx) {
    return fx && fx->particles && particles_is_enabled(fx->particles);
}

bool effects_trails_enabled(EffectsManager *fx) {
    return fx && fx->trails && trails_is_enabled(fx->trails);
}

bool effects_breathing_enabled(EffectsManager *fx) {
    return fx && fx->enhancements.breathing_enabled;
}
