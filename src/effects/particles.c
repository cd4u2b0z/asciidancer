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
    
    /* Body mask defaults (disabled until set) */
    ps->body_mask_enabled = false;
    ps->body_center_x = canvas_width / 2.0f;
    ps->body_center_y = canvas_height / 2.0f;
    ps->body_radius = 8.0f;
    ps->repulsion_strength = 60.0f;  /* Default outward repulsion */
    
    /* Particle cap for visual clarity */
    ps->max_active = 40;  /* Reduced from 256 for cleaner visuals */
    
    /* Normal fade speed */
    ps->fade_multiplier = 1.0f;
    
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

void particles_spawn(ParticleSystem *ps, const EmitterConfig *config, int count) {
    if (!ps || !ps->enabled || !config) return;
    
    /* Cap spawning if we're at max active particles */
    if (ps->active_count >= ps->max_active) {
        count = count / 4;  /* Drastically reduce spawns when full */
        if (count < 1) return;
    }
    
    for (int i = 0; i < count; i++) {
        int idx = find_slot(ps);
        Particle *p = &ps->particles[idx];
        
        /* Position */
        p->x = config->x;
        p->y = config->y;
        
        /* Body mask check - nudge spawn point outward if too close to body */
        if (ps->body_mask_enabled) {
            float dx = p->x - ps->body_center_x;
            float dy = p->y - ps->body_center_y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            /* If within body exclusion zone, push outward */
            if (dist < ps->body_radius && dist > 0.1f) {
                float push = (ps->body_radius - dist) + 3.0f;
                p->x += (dx / dist) * push;
                p->y += (dy / dist) * push;
            } else if (dist < 0.1f) {
                /* Dead center - push in random direction */
                float angle = randf() * 2.0f * M_PI;
                p->x += cosf(angle) * (ps->body_radius + 3.0f);
                p->y += sinf(angle) * (ps->body_radius + 3.0f);
            }
        }
        
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
    
    /* Reduced particle count for cleaner visuals */
    int count = (int)(2 + intensity * 8);  /* Was 5 + intensity * 30 */
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 0.6f,  /* Narrower spread */
        .base_angle = -M_PI / 2,
        .min_speed = 30.0f + intensity * 40.0f,  /* Slower */
        .max_speed = 60.0f + intensity * 60.0f,
        .min_life = 0.3f,
        .max_life = 0.7f,  /* Shorter life */
        .gravity = 180.0f,  /* More gravity = falls faster, clears screen */
        .drag = 0.94f,
        .size_min = 1.0f,
        .size_max = 1.0f,  /* Smaller */
        .pattern = SPAWN_FOUNTAIN,
        .type = PARTICLE_SPARK,
        .color_base = 1,
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}

void particles_emit_treble_sparkle(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled || intensity < 0.2f) return;
    
    /* Reduced particle count for cleaner visuals */
    int count = (int)(3 + intensity * 10);  /* Was 15 + intensity * 40 */
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 2,
        .base_angle = 0,
        .min_speed = 15.0f,  /* Slightly faster to move away */
        .max_speed = 40.0f,
        .min_life = 0.15f,  /* Shorter life */
        .max_life = 0.35f,
        .gravity = 0.0f,
        .drag = 0.85f,  /* More drag = stops faster */
        .size_min = 1.0f,
        .size_max = 1.0f,
        .pattern = SPAWN_SPARKLE,
        .type = PARTICLE_SPARK,
        .color_base = 2,
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}

void particles_emit_beat_burst(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled) return;
    
    /* Reduced particle count for cleaner visuals */
    int count = (int)(4 + intensity * 12);  /* Was 15 + intensity * 40 */
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = M_PI * 2,
        .base_angle = 0,
        .min_speed = 40.0f * intensity,  /* Slightly slower */
        .max_speed = 80.0f * intensity,
        .min_life = 0.2f,  /* Shorter life */
        .max_life = 0.5f,
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

/* NEW: Spawn floating music note particles */
void particles_emit_music_notes(ParticleSystem *ps, float x, float y, float intensity) {
    if (!ps || !ps->enabled || intensity < 0.3f) return;
    
    /* Spawn 1-3 music notes floating upward */
    int count = 1 + (int)(intensity * 2);
    if (count > 3) count = 3;
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = 1.0f,  /* ~60 degree spread */
        .base_angle = -M_PI / 2,  /* Upward */
        .min_speed = 20.0f,
        .max_speed = 40.0f,
        .min_life = 1.0f,   /* Notes last longer */
        .max_life = 2.0f,
        .gravity = -15.0f,  /* Float upward (negative gravity) */
        .drag = 0.95f,
        .size_min = 1.0f,
        .size_max = 1.5f,
        .pattern = SPAWN_POINT,
        .type = PARTICLE_NOTE,
        .color_base = 3,
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}

void particles_update(ParticleSystem *ps, float dt) {
    if (!ps || !ps->enabled) return;
    
    ps->active_count = 0;
    
    /* Apply fade multiplier to dt for faster clearing */
    float effective_dt = dt * ps->fade_multiplier;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        /* Update lifetime (faster when fade_multiplier > 1) */
        p->lifetime -= effective_dt;
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
        
        /* Body mask check during movement - push particles away from body */
        if (ps->body_mask_enabled) {
            float dx = p->x - ps->body_center_x;
            float dy = p->y - ps->body_center_y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < ps->body_radius * 0.9f && dist > 0.1f) {
                /* Push outward with repulsion force */
                float repel = ps->repulsion_strength > 0 ? ps->repulsion_strength : 50.0f;
                float push_factor = (ps->body_radius - dist) / ps->body_radius;
                p->vx += (dx / dist) * repel * push_factor;
                p->vy += (dy / dist) * repel * push_factor;
            }
        }
        
        /* Apply outward bias even outside body mask (v2.4) */
        if (ps->repulsion_strength > 0 && ps->body_mask_enabled) {
            float dx = p->x - ps->body_center_x;
            float dy = p->y - ps->body_center_y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist > 0.1f && dist < ps->body_radius * 2.0f) {
                /* Gentle outward drift */
                float drift = ps->repulsion_strength * 0.1f * dt;
                p->vx += (dx / dist) * drift;
                p->vy += (dy / dist) * drift;
            }
        }
        
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
        const Particle *p = &ps->particles[i];
        if (!p->active) continue;
        
        int px = (int)(p->x + 0.5f);
        int py = (int)(p->y + 0.5f);
        
        /* Skip if brightness too low */
        if (p->brightness < 0.1f) continue;
        
        /* Body mask check - don't render particles that would obscure the character */
        if (ps->body_mask_enabled) {
            float dx = p->x - ps->body_center_x;
            float dy = p->y - ps->body_center_y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            /* Skip rendering if inside body exclusion zone */
            if (dist < ps->body_radius * 0.8f) continue;
        }
        
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
                
            case PARTICLE_NOTE:
                /* Music note shape - oval head + stem */
                /* Note head (small filled oval) */
                braille_set_pixel(canvas, px, py, true);
                braille_set_pixel(canvas, px + 1, py, true);
                braille_set_pixel(canvas, px, py + 1, true);
                braille_set_pixel(canvas, px + 1, py + 1, true);
                /* Stem going up */
                braille_set_pixel(canvas, px + 1, py - 1, true);
                braille_set_pixel(canvas, px + 1, py - 2, true);
                braille_set_pixel(canvas, px + 1, py - 3, true);
                /* Flag at top */
                if (p->brightness > 0.4f) {
                    braille_set_pixel(canvas, px + 2, py - 2, true);
                    braille_set_pixel(canvas, px + 2, py - 3, true);
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

void particles_set_body_mask(ParticleSystem *ps, float center_x, float center_y __attribute__((unused)),
                             float head_y, float foot_y, float radius) {
    if (!ps) return;
    ps->body_center_x = center_x;
    ps->body_center_y = (head_y + foot_y) / 2.0f;  /* Vertical center */
    ps->body_head_y = head_y;
    ps->body_foot_y = foot_y;
    ps->body_radius = radius;
    ps->body_mask_enabled = true;
}

void particles_set_fade_multiplier(ParticleSystem *ps, float mult) {
    if (!ps) return;
    ps->fade_multiplier = (mult > 0.1f) ? mult : 0.1f;
}

void particles_set_repulsion(ParticleSystem *ps, float strength) {
    if (!ps) return;
    ps->repulsion_strength = strength;
}

/* Control bus driven emission (v2.4) */
void particles_emit_controlled(ParticleSystem *ps, 
                               float x, float y,
                               float energy, float onset, 
                               float bass, float treble) {
    if (!ps || !ps->enabled) return;
    
    /* Don't spawn if at particle cap */
    if (ps->active_count >= ps->max_active) return;
    
    /* Count scales with onset + energy */
    int count = (int)(onset * 6.0f + energy * 4.0f);
    if (count < 1) return;
    if (count > 8) count = 8;  /* Hard cap per frame */
    
    /* Spread radius scales with energy */
    float spread = 0.3f + energy * 0.7f;  /* π * 0.3 to π * 1.0 */
    
    /* Velocity scales with onset */
    float speed_base = 20.0f + onset * 60.0f;
    float speed_max = speed_base * 1.5f;
    
    /* Lifetime inversely scales with energy (fast decay at high energy) */
    float life_base = 0.5f - energy * 0.3f;  /* 0.5 to 0.2 seconds */
    if (life_base < 0.15f) life_base = 0.15f;
    
    /* Choose emission type based on bass/treble balance */
    SpawnPattern pattern;
    int color;
    if (bass > treble * 1.5f) {
        /* Bass dominated - upward fountain */
        pattern = SPAWN_FOUNTAIN;
        color = 1;  /* Warm color */
    } else if (treble > bass * 1.5f) {
        /* Treble dominated - sparkle */
        pattern = SPAWN_SPARKLE;
        color = 2;  /* Cool color */
    } else {
        /* Balanced - burst */
        pattern = SPAWN_BURST;
        color = 0;  /* Neutral */
    }
    
    EmitterConfig config = {
        .x = x,
        .y = y,
        .spread_angle = spread * M_PI,
        .base_angle = -M_PI / 2,  /* Up */
        .min_speed = speed_base,
        .max_speed = speed_max,
        .min_life = life_base * 0.7f,
        .max_life = life_base * 1.3f,
        .gravity = 100.0f + energy * 100.0f,  /* More gravity at high energy */
        .drag = 0.92f,
        .size_min = 1.0f,
        .size_max = 1.0f,
        .pattern = pattern,
        .type = PARTICLE_SPARK,
        .color_base = color,
        .fade_out = true,
        .shrink = false
    };
    
    particles_spawn(ps, &config, count);
}
