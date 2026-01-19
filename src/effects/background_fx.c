/*
 * Background Particle Effects Implementation - ASCII Dancer v3.0
 */

#include "background_fx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============ Private Helpers ============ */

/* Create emitter config for ambient particles */
static EmitterConfig create_ambient_config(float x, float y, float intensity) {
    EmitterConfig config = {0};
    config.x = x;
    config.y = y;
    config.pattern = SPAWN_BURST;
    config.type = PARTICLE_DOT;
    config.min_speed = 0.5f * intensity;
    config.max_speed = 2.0f * intensity;
    config.spread_angle = 360.0f;
    config.min_life = 3.0f;
    config.max_life = 6.0f;
    config.size_min = 1.0f;
    config.size_max = 1.0f;
    config.gravity = 0.0f;
    config.drag = 0.98f;
    config.color_base = 38;  /* Cyan in 256-color palette */
    config.fade_out = true;
    return config;
}

/* Create emitter config for wave particles */
static EmitterConfig create_wave_config(float x, float y, float band_idx, float energy) {
    EmitterConfig config = {0};
    config.x = x;
    config.y = y;
    config.pattern = SPAWN_FOUNTAIN;
    config.type = PARTICLE_SPARK;
    config.min_speed = 10.0f + energy * 20.0f;
    config.max_speed = 15.0f + energy * 30.0f;
    config.spread_angle = 45.0f;
    config.base_angle = 270.0f;  /* Upward */
    config.min_life = 0.5f;
    config.max_life = 1.2f;
    config.size_min = 1.0f + energy;
    config.size_max = 2.0f + energy;
    config.gravity = 5.0f;
    config.drag = 0.95f;
    config.fade_out = true;
    
    /* Color based on frequency band */
    if (band_idx < 2.0f) {
        /* Bass - red/orange (196-202 range) */
        config.color_base = 196;
    } else if (band_idx < 4.0f) {
        /* Mid - yellow/green (226-228 range) */
        config.color_base = 226;
    } else {
        /* Treble - cyan/blue (51-81 range) */
        config.color_base = 51;
    }
    
    return config;
}

/* Create emitter config for aura particles */
static EmitterConfig create_aura_config(float x, float y, float energy) {
    EmitterConfig config = {0};
    config.x = x;
    config.y = y;
    config.pattern = SPAWN_BURST;
    config.type = PARTICLE_STAR;
    config.min_speed = 2.0f;
    config.max_speed = 5.0f;
    config.spread_angle = 360.0f;
    config.min_life = 0.8f;
    config.max_life = 1.5f;
    config.size_min = 2.0f;
    config.size_max = 2.0f;
    config.gravity = 0.0f;
    config.drag = 0.92f;
    config.fade_out = true;
    
    /* Energy-based color (blue to orange, 21-208 gradient) */
    int color_index = 21 + (int)(energy * 40);  /* Blue to cyan to yellow range */
    config.color_base = color_index;
    
    return config;
}

/* ============ Public API ============ */

BackgroundFX* background_fx_create(ParticleSystem *particles) {
    if (!particles) return NULL;
    
    BackgroundFX *fx = calloc(1, sizeof(BackgroundFX));
    if (!fx) return NULL;
    
    fx->particles = particles;
    fx->type = BG_AMBIENT_FIELD;
    fx->enabled = true;
    fx->intensity = 0.5f;
    fx->speed = 1.0f;
    
    /* Initialize defaults */
    fx->ambient.particle_count = 20;
    fx->ambient.drift_speed = 1.0f;
    fx->ambient.twinkle_rate = 2.0f;
    
    fx->spectral.wave_speed = 0.5f;
    fx->spectral.active_waves = 0;
    
    fx->aura.radius = 20.0f;
    fx->aura.ring_particles = 24;
    
    fx->burst.burst_cooldown = 0.25f;
    fx->burst.burst_particles = 30;
    fx->burst.burst_radius = 15.0f;
    
    fx->ribbons.particles_per_band = 5;
    
    fx->rain.drop_count = 15;
    fx->rain.fall_speed = 10.0f;
    fx->rain.spawn_rate = 0.1f;
    
    fx->vortex.rotation_speed = 0.5f;
    fx->vortex.spiral_arms = 3;
    fx->vortex.arm_length = 30.0f;
    
    return fx;
}

void background_fx_destroy(BackgroundFX *fx) {
    if (fx) {
        free(fx);
    }
}

void background_fx_enable(BackgroundFX *fx, bool enabled) {
    if (fx) fx->enabled = enabled;
}

void background_fx_set_type(BackgroundFX *fx, BackgroundFXType type) {
    if (fx) fx->type = type;
}

void background_fx_set_intensity(BackgroundFX *fx, float intensity) {
    if (fx) {
        fx->intensity = fmaxf(0.0f, fminf(1.0f, intensity));
    }
}

void background_fx_update(BackgroundFX *fx, float dt) {
    if (!fx || !fx->enabled) return;
    
    fx->current_time += dt;
    fx->dt = dt;
    
    /* Update effect-specific state */
    switch (fx->type) {
        case BG_AMBIENT_FIELD:
            /* Ambient particles spawn continuously */
            if (fx->intensity > 0.1f) {
                background_fx_generate_ambient(fx);
            }
            break;
            
        case BG_SPECTRAL_WAVES:
            fx->spectral.wave_phase += fx->spectral.wave_speed * dt * fx->speed;
            background_fx_generate_wave(fx, fx->intensity);
            break;
            
        case BG_ENERGY_AURA:
            fx->aura.pulse_phase += dt * 3.0f * fx->speed;
            background_fx_generate_aura(fx, fx->intensity);
            break;
            
        case BG_FREQUENCY_RIBBONS:
            background_fx_generate_ribbons(fx);
            break;
            
        case BG_PARTICLE_RAIN:
            background_fx_generate_rain(fx);
            break;
            
        case BG_SPIRAL_VORTEX:
            fx->vortex.rotation += fx->vortex.rotation_speed * dt * fx->speed;
            background_fx_generate_vortex(fx);
            break;
            
        case BG_BEAT_BURST:
            /* Only triggers on beats (from update_audio) */
            break;
            
        default:
            break;
    }
}

void background_fx_update_audio(BackgroundFX *fx,
                               float energy,
                               float bass,
                               float mid,
                               float treble,
                               bool beat_hit) {
    if (!fx || !fx->enabled) return;
    
    /* Beat burst triggers on hits */
    if (fx->type == BG_BEAT_BURST && beat_hit) {
        double time_since_burst = fx->current_time - fx->burst.last_burst_time;
        if (time_since_burst >= fx->burst.burst_cooldown) {
            background_fx_generate_burst(fx, energy);
            fx->burst.last_burst_time = fx->current_time;
        }
    }
    
    /* Modulate intensity based on energy */
    float dynamic_intensity = fx->intensity * (0.5f + energy * 0.5f);
    fx->intensity = dynamic_intensity;
}

void background_fx_update_bands(BackgroundFX *fx,
                               float sub_bass,
                               float bass,
                               float low_mid,
                               float mid,
                               float high_mid,
                               float treble) {
    if (!fx) return;
    
    fx->spectral.band_heights[0] = sub_bass;
    fx->spectral.band_heights[1] = bass;
    fx->spectral.band_heights[2] = low_mid;
    fx->spectral.band_heights[3] = mid;
    fx->spectral.band_heights[4] = high_mid;
    fx->spectral.band_heights[5] = treble;
    
    /* Smooth ribbon heights */
    for (int i = 0; i < 6; i++) {
        float target = fx->spectral.band_heights[i];
        fx->ribbons.ribbon_height[i] += (target - fx->ribbons.ribbon_height[i]) * 0.2f;
    }
}

void background_fx_update_dancer_pos(BackgroundFX *fx, int x, int y) {
    if (!fx) return;
    fx->aura.dancer_x = x;
    fx->aura.dancer_y = y;
}

/* ============ Effect Generators ============ */

void background_fx_generate_ambient(BackgroundFX *fx) {
    if (!fx || !fx->particles) return;
    
    /* Spawn a few ambient particles per frame */
    static float spawn_accumulator = 0.0f;
    spawn_accumulator += fx->dt * fx->ambient.twinkle_rate;
    
    while (spawn_accumulator >= 1.0f) {
        /* Random position across screen */
        float x = (float)(rand() % fx->particles->canvas_width);
        float y = (float)(rand() % fx->particles->canvas_height);
        
        EmitterConfig config = create_ambient_config(x, y, fx->intensity);
        particles_spawn(fx->particles, &config, 1);
        
        spawn_accumulator -= 1.0f;
    }
}

void background_fx_generate_wave(BackgroundFX *fx, float energy) {
    if (!fx || !fx->particles) return;
    
    /* Spawn wave particles from bottom of screen */
    int num_bands = 6;
    int width = fx->particles->canvas_width;
    int spacing = width / (num_bands + 1);
    
    for (int i = 0; i < num_bands; i++) {
        float band_energy = fx->spectral.band_heights[i];
        if (band_energy < 0.1f) continue;
        
        float x = spacing * (i + 1);
        float y = fx->particles->canvas_height - 2;
        
        /* Wave amplitude affects spawn rate */
        float spawn_chance = band_energy * fx->intensity * fx->dt * 10.0f;
        if ((float)rand() / RAND_MAX < spawn_chance) {
            EmitterConfig config = create_wave_config(x, y, (float)i, band_energy);
            particles_spawn(fx->particles, &config, 2);
        }
    }
}

void background_fx_generate_aura(BackgroundFX *fx, float energy) {
    if (!fx || !fx->particles) return;
    
    /* Spawn particles in ring around dancer */
    float pulse = 1.0f + 0.3f * sinf(fx->aura.pulse_phase);
    float radius = fx->aura.radius * pulse * energy;
    
    static float spawn_timer = 0.0f;
    spawn_timer += fx->dt;
    
    if (spawn_timer >= 0.1f) {  /* Spawn every 100ms */
        for (int i = 0; i < fx->aura.ring_particles; i++) {
            float angle = (2.0f * M_PI * i) / fx->aura.ring_particles;
            float x = fx->aura.dancer_x + radius * cosf(angle);
            float y = fx->aura.dancer_y + radius * sinf(angle);
            
            EmitterConfig config = create_aura_config(x, y, energy);
            particles_spawn(fx->particles, &config, 1);
        }
        spawn_timer = 0.0f;
    }
}

void background_fx_generate_burst(BackgroundFX *fx, float energy) {
    if (!fx || !fx->particles) return;
    
    /* Explosion at dancer position */
    int count = (int)(fx->burst.burst_particles * energy * fx->intensity);
    particles_emit_beat_burst(fx->particles, 
                             (float)fx->aura.dancer_x,
                             (float)fx->aura.dancer_y,
                             energy * fx->intensity);
}

void background_fx_generate_ribbons(BackgroundFX *fx) {
    if (!fx || !fx->particles) return;
    
    /* Vertical frequency bars */
    int num_bands = 6;
    int width = fx->particles->canvas_width;
    int spacing = width / (num_bands + 1);
    
    for (int i = 0; i < num_bands; i++) {
        float height = fx->ribbons.ribbon_height[i];
        if (height < 0.1f) continue;
        
        float x = spacing * (i + 1);
        int bar_height = (int)(height * fx->particles->canvas_height * 0.8f);
        
        /* Spawn particles along the bar */
        float spawn_chance = height * fx->intensity * fx->dt * 15.0f;
        if ((float)rand() / RAND_MAX < spawn_chance) {
            float y = fx->particles->canvas_height - (float)(rand() % (bar_height + 1));
            
            EmitterConfig config = create_wave_config(x, y, (float)i, height);
            config.min_speed = 1.0f;
            config.max_speed = 3.0f;
            config.spread_angle = 30.0f;
            config.base_angle = 90.0f;  /* Sideways */
            particles_spawn(fx->particles, &config, 1);
        }
    }
}

void background_fx_generate_rain(BackgroundFX *fx) {
    if (!fx || !fx->particles) return;
    
    /* Spawn falling particles from top */
    static float rain_timer = 0.0f;
    rain_timer += fx->dt;
    
    float spawn_interval = 1.0f / (fx->rain.spawn_rate * fx->intensity);
    
    if (rain_timer >= spawn_interval) {
        float x = (float)(rand() % fx->particles->canvas_width);
        float y = 0.0f;
        
        EmitterConfig config = {0};
        config.x = x;
        config.y = y;
        config.pattern = SPAWN_POINT;
        config.type = PARTICLE_TRAIL;
        config.min_speed = fx->rain.fall_speed;
        config.max_speed = fx->rain.fall_speed * 1.5f;
        config.spread_angle = 0.0f;
        config.base_angle = 90.0f;  /* Downward */
        config.min_life = 3.0f;
        config.max_life = 5.0f;
        config.size_min = 1.0f;
        config.size_max = 1.0f;
        config.gravity = 2.0f;
        config.drag = 0.99f;
        config.color_base = 51;  /* Cyan-blue */
        config.fade_out = true;
        
        particles_spawn(fx->particles, &config, 1);
        rain_timer = 0.0f;
    }
}

void background_fx_generate_vortex(BackgroundFX *fx) {
    if (!fx || !fx->particles) return;
    
    /* Spiral arms rotating around center */
    int center_x = fx->particles->canvas_width / 2;
    int center_y = fx->particles->canvas_height / 2;
    
    static float spawn_timer = 0.0f;
    spawn_timer += fx->dt;
    
    if (spawn_timer >= 0.05f) {  /* Spawn every 50ms */
        for (int arm = 0; arm < fx->vortex.spiral_arms; arm++) {
            float arm_angle = fx->vortex.rotation + (2.0f * M_PI * arm) / fx->vortex.spiral_arms;
            
            /* Spawn along the spiral arm */
            for (float r = 5.0f; r < fx->vortex.arm_length; r += 10.0f) {
                float spiral_offset = r * 0.1f;
                float angle = arm_angle + spiral_offset;
                
                float x = center_x + r * cosf(angle);
                float y = center_y + r * sinf(angle);
                
                if (x >= 0 && x < fx->particles->canvas_width &&
                    y >= 0 && y < fx->particles->canvas_height) {
                    
                    EmitterConfig config = create_ambient_config(x, y, fx->intensity);
                    config.type = PARTICLE_STAR;
                    config.min_life = 0.5f;
                    config.max_life = 1.0f;
                    particles_spawn(fx->particles, &config, 1);
                }
            }
        }
        spawn_timer = 0.0f;
    }
}

/* ============ Queries ============ */

BackgroundFXType background_fx_get_type(BackgroundFX *fx) {
    return fx ? fx->type : BG_NONE;
}

const char* background_fx_get_type_name(BackgroundFXType type) {
    switch (type) {
        case BG_NONE: return "None";
        case BG_AMBIENT_FIELD: return "Ambient Field";
        case BG_SPECTRAL_WAVES: return "Spectral Waves";
        case BG_ENERGY_AURA: return "Energy Aura";
        case BG_BEAT_BURST: return "Beat Burst";
        case BG_FREQUENCY_RIBBONS: return "Frequency Ribbons";
        case BG_PARTICLE_RAIN: return "Particle Rain";
        case BG_SPIRAL_VORTEX: return "Spiral Vortex";
        default: return "Unknown";
    }
}

bool background_fx_is_enabled(BackgroundFX *fx) {
    return fx ? fx->enabled : false;
}
