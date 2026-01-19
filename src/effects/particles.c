/*
 * Particle System Implementation
 */

#include "particles.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Random float between 0 and 1 */
static float randf(void) {
    return (float)rand() / (float)RAND_MAX;
}

/* Random float in range */
static float randf_range(float min, float max) {
    return min + randf() * (max - min);
}

ParticleSystem* particles_create(int canvas_width, int canvas_height) {
    ParticleSystem *ps = calloc(1, sizeof(ParticleSystem));
    if (!ps) return NULL;
    
    ps->canvas_width = canvas_width;
    ps->canvas_height = canvas_height;
    ps->world_gravity = 120.0f;  /* Pixels per second^2 */
    ps->world_drag = 0.98f;
    ps->enabled = true;
    
    /* Seed random if not already */
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = true;
    }
    
    return ps;
}

void particles_destroy(ParticleSystem *ps) {
    if (ps) free(ps);
}

/* Find next available particle slot */
static int find_slot(ParticleSystem *ps) {
    /* Try round-robin first */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        int idx = (ps->next_slot + i) % MAX_PARTICLES;
        if (!ps->particles[idx].active) {
            ps->next_slot = (idx + 1) % MAX_PARTICLES;
            return idx;
        }
    }
    
    /* All full - overwrite oldest (round-robin position) */
    int idx = ps->next_slot;
    ps->next_slot = (ps->next_slot + 1) % MAX_PARTICLES;
    return idx;
}

void particles_spawn(ParticleSystem *ps, EmitterConfig *config, int count) {
    if (!ps || !ps->enabled || !config) return;
    
    for (int i = 0; i < count; i++) {
        int idx = find_slot(ps);
        Particle *p = &ps->particles[idx];
        
        /* Position */
        p->x = config->x;
        p->y = config->y;
        
        /* Velocity based on pattern */
        float angle, speed;
        speed = randf_range(config->min_speed, config->max_speed);
        
        switch (config->pattern) {
            case SPAWN_BURST:
            case SPAWN_EXPLOSION:
                angle = randf() * 2.0f * M_PI;
                break;
            case SPAWN_FOUNTAIN:
                angle = -M_PI/2 + randf_range(-config->spread_angle/2, config->spread_angle/2);
                break;
            case SPAWN_RAIN:
                angle = M_PI/2 + randf_range(-0.2f, 0.2f);
                p->y = 0;
                p->x = randf() * ps->canvas_width;
                break;
            case SPAWN_SPARKLE:
                angle = randf() * 2.0f * M_PI;
                p->x += randf_range(-10, 10);
                p->y += randf_range(-10, 10);
                speed *= 0.3f;
                break;
            default: /* SPAWN_POINT */
                angle = config->base_angle + randf_range(-config->spread_angle/2, config->spread_angle/2);
                break;
        }
        
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->ax = 0;
        p->ay = config->gravity;
        
        /* Lifetime */
        p->max_life = randf_range(config->min_life, config->max_life);
        p->lifetime = p->max_life;
        
        /* Appearance */
        p->size = randf_range(config->size_min, config->size_max);
        p->brightness = 1.0f;
        p->type = config->type;
        p->color_index = config->color_base;
        p->active = true;
        
        ps->active_count++;
        ps->total_spawned++;
    }
}

void particles_spawn_at(ParticleSystem *ps, float x, float y,
                        SpawnPattern pattern, int count, float energy) {
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 0.5f,
        .base_angle = -M_PI / 2,  /* Up */
        .min_speed = 30.0f * energy,
        .max_speed = 80.0f * energy,
        .min_life = 0.3f,
        .max_life = 0.8f,
        .gravity = 100.0f,
        .drag = 0.95f,
        .size_min = 1.0f,
        .size_max = 2.0f,
        .pattern = pattern,
        .type = PARTICLE_SPARK,
        .color_base = 0,
        .fade_out = true,
        .shrink = true
    };
    particles_spawn(ps, &config, count);
}

/* === Preset emitters === */

void particles_emit_bass_hit(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled) return;
    
    int count = (int)(5 + intensity * 30);
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI,
        .base_angle = -M_PI / 2,
        .min_speed = 40.0f + intensity * 60.0f,
        .max_speed = 80.0f + intensity * 100.0f,
        .min_life = 0.4f,
        .max_life = 1.0f,
        .gravity = 150.0f,
        .drag = 0.96f,
        .size_min = 1.0f,
        .size_max = 2.0f,
        .pattern = SPAWN_FOUNTAIN,
        .type = PARTICLE_SPARK,
        .color_base = 1,  /* Fire colors */
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}

void particles_emit_treble_sparkle(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled || intensity < 0.2f) return;
    
    int count = (int)(15 + intensity * 40);
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 2,
        .base_angle = 0,
        .min_speed = 10.0f,
        .max_speed = 30.0f,
        .min_life = 0.2f,
        .max_life = 0.5f,
        .gravity = 0.0f,  /* No gravity for sparkles */
        .drag = 0.9f,
        .size_min = 1.0f,
        .size_max = 1.0f,
        .pattern = SPAWN_SPARKLE,
        .type = PARTICLE_SPARK,
        .color_base = 2,  /* Bright colors */
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}

void particles_emit_beat_burst(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled) return;
    
    int count = (int)(15 + intensity * 40);
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 2,
        .base_angle = 0,
        .min_speed = 50.0f * intensity,
        .max_speed = 120.0f * intensity,
        .min_life = 0.3f,
        .max_life = 0.7f,
        .gravity = 80.0f,
        .drag = 0.94f,
        .size_min = 1.0f,
        .size_max = 2.0f,
        .pattern = SPAWN_EXPLOSION,
        .type = PARTICLE_SPARK,
        .color_base = 0,
        .fade_out = true,
        .shrink = true
    };
    
    particles_spawn(ps, &config, count);
}

void particles_emit_foot_stomp(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled || intensity < 0.25f) return;
    
    int count = (int)(4 + intensity * 10);
    
    /* Ground dust effect - particles go sideways */
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 0.3f,
        .base_angle = 0,  /* Horizontal */
        .min_speed = 20.0f,
        .max_speed = 50.0f * intensity,
        .min_life = 0.2f,
        .max_life = 0.5f,
        .gravity = 30.0f,
        .drag = 0.85f,
        .size_min = 1.0f,
        .size_max = 1.0f,
        .pattern = SPAWN_POINT,
        .type = PARTICLE_DOT,
        .color_base = 3,  /* Dust color */
        .fade_out = true,
        .shrink = false
    };
    
    /* Emit both left and right */
    config.base_angle = M_PI;  /* Left */
    particles_spawn(ps, &config, count / 2);
    config.base_angle = 0;     /* Right */
    particles_spawn(ps, &config, count / 2);
}

void particles_emit_hand_flourish(ParticleSystem *ps, float x, float y, float vx, float vy) {
    if (!ps || !ps->enabled) return;
    
    /* Trail particles following hand movement */
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = 0.3f,
        .base_angle = atan2f(vy, vx) + M_PI,  /* Opposite to movement */
        .min_speed = 5.0f,
        .max_speed = 15.0f,
        .min_life = 0.15f,
        .max_life = 0.3f,
        .gravity = 20.0f,
        .drag = 0.9f,
        .size_min = 1.0f,
        .size_max = 1.0f,
        .pattern = SPAWN_POINT,
        .type = PARTICLE_TRAIL,
        .color_base = 2,
        .fade_out = true,
        .shrink = false
    };
    
    /* Add some of parent velocity */
    particles_spawn(ps, &config, 2);
}

void particles_update(ParticleSystem *ps, float dt) {
    if (!ps || !ps->enabled) return;
    
    ps->active_count = 0;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        /* Update lifetime */
        p->lifetime -= dt;
        if (p->lifetime <= 0) {
            p->active = false;
            ps->total_died++;
            continue;
        }
        
        /* Fade and shrink based on remaining life */
        float life_ratio = p->lifetime / p->max_life;
        p->brightness = life_ratio;
        
        /* Apply acceleration (gravity) */
        p->vy += p->ay * dt;
        p->vx += p->ax * dt;
        
        /* Apply drag */
        p->vx *= ps->world_drag;
        p->vy *= ps->world_drag;
        
        /* Update position */
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        /* Bounds check - deactivate if off screen */
        if (p->x < -10 || p->x > ps->canvas_width + 10 ||
            p->y < -10 || p->y > ps->canvas_height + 10) {
            p->active = false;
            ps->total_died++;
            continue;
        }
        
        ps->active_count++;
    }
}

void particles_render(ParticleSystem *ps, BrailleCanvas *canvas) {
    if (!ps || !ps->enabled || !canvas) return;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        int px = (int)(p->x + 0.5f);
        int py = (int)(p->y + 0.5f);
        
        /* Skip if brightness too low */
        if (p->brightness < 0.1f) continue;
        
        switch (p->type) {
            case PARTICLE_SPARK:
                /* Single bright pixel */
                braille_set_pixel(canvas, px, py, true);
                break;
                
            case PARTICLE_DOT:
                /* 2x2 dot for larger effect */
                braille_set_pixel(canvas, px, py, true);
                if (p->brightness > 0.5f) {
                    braille_set_pixel(canvas, px + 1, py, true);
                    braille_set_pixel(canvas, px, py + 1, true);
                }
                break;
                
            case PARTICLE_TRAIL:
                /* Single pixel with velocity trail */
                braille_set_pixel(canvas, px, py, true);
                if (p->brightness > 0.3f) {
                    int tx = px - (int)(p->vx * 0.02f);
                    int ty = py - (int)(p->vy * 0.02f);
                    braille_draw_line(canvas, px, py, tx, ty);
                }
                break;
                
            case PARTICLE_STAR:
                /* 5-pixel star pattern */
                braille_set_pixel(canvas, px, py, true);
                if (p->brightness > 0.5f) {
                    braille_set_pixel(canvas, px - 1, py, true);
                    braille_set_pixel(canvas, px + 1, py, true);
                    braille_set_pixel(canvas, px, py - 1, true);
                    braille_set_pixel(canvas, px, py + 1, true);
                }
                break;
        }
    }
}

void particles_clear(ParticleSystem *ps) {
    if (!ps) return;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ps->particles[i].active = false;
    }
    ps->active_count = 0;
    ps->next_slot = 0;
}

void particles_set_enabled(ParticleSystem *ps, bool enabled) {
    if (ps) ps->enabled = enabled;
}

bool particles_is_enabled(ParticleSystem *ps) {
    return ps ? ps->enabled : false;
}

int particles_get_active_count(ParticleSystem *ps) {
    return ps ? ps->active_count : 0;
}
