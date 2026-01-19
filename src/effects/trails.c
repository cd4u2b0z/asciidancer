/*
 * Motion Trails Implementation
 */

#include "trails.h"
#include "../braille/skeleton_dancer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Forward declarations for static functions */
static void trails_track_all_limbs(MotionTrails *trails);
static void trails_clear(MotionTrails *trails);

MotionTrails* trails_create(void) {
    MotionTrails *trails = calloc(1, sizeof(MotionTrails));
    if (!trails) return NULL;
    
    trails->fade_rate = 0.85f;       /* Each step fades to 70% */
    trails->min_velocity = 0.3f;    /* Minimum pixels/frame to show - lowered for sensitivity */
    trails->trail_length = 6;       /* Default trail length */
    trails->adaptive_length = true;
    trails->enabled = true;
    trails->update_interval = 2;    /* Update every 2 frames */
    
    /* Default: track hands and feet */
    trails_track_all_limbs(trails);
    
    return trails;
}

void trails_destroy(MotionTrails *trails) {
    if (trails) free(trails);
}

void trails_set_tracked_joints(MotionTrails *trails, const int *joint_ids, int count) {
    if (!trails) return;
    
    trails->num_tracked = (count < TRAIL_JOINTS) ? count : TRAIL_JOINTS;
    for (int i = 0; i < trails->num_tracked; i++) {
        trails->tracked_joints[i] = joint_ids[i];
    }
}

static void trails_track_all_limbs(MotionTrails *trails) {
    if (!trails) return;
    
    /* Track: hands, feet, elbows, knees */
    const int limb_joints[] = {
        JOINT_HAND_L,
        JOINT_HAND_R,
        JOINT_FOOT_L,
        JOINT_FOOT_R,
        JOINT_ELBOW_L,
        JOINT_ELBOW_R,
        JOINT_KNEE_L,
        JOINT_KNEE_R
    };
    
    trails->num_tracked = 8;
    for (int i = 0; i < 8; i++) {
        trails->tracked_joints[i] = limb_joints[i];
    }
}

void trails_update(MotionTrails *trails, Joint *joints, int num_joints, float dt __attribute__((unused))) {
    if (!trails || !trails->enabled || !joints) return;
    
    trails->frame_count++;
    if (trails->frame_count < trails->update_interval) return;
    trails->frame_count = 0;
    
    for (int t = 0; t < trails->num_tracked; t++) {
        int joint_id = trails->tracked_joints[t];
        if (joint_id < 0 || joint_id >= num_joints) continue;
        
        JointTrail *trail = &trails->joints[t];
        float new_x = joints[joint_id].x;
        float new_y = joints[joint_id].y;
        
        /* Calculate velocity */
        if (trail->history[0].valid) {
            float dx = new_x - trail->last_x;
            float dy = new_y - trail->last_y;
            trail->velocity = sqrtf(dx * dx + dy * dy);
        }
        
        /* Fade existing trail points */
        for (int i = 0; i < TRAIL_HISTORY_SIZE; i++) {
            if (trail->history[i].valid) {
                trail->history[i].alpha *= trails->fade_rate;
                if (trail->history[i].alpha < 0.05f) {
                    trail->history[i].valid = false;
                }
            }
        }
        
        /* Only add new point if moving fast enough */
        if (trail->velocity >= trails->min_velocity) {
            TrailPoint *point = &trail->history[trail->write_pos];
            point->x = new_x;
            point->y = new_y;
            point->alpha = 1.0f;
            point->valid = true;
            
            trail->write_pos = (trail->write_pos + 1) % TRAIL_HISTORY_SIZE;
        }
        
        trail->last_x = new_x;
        trail->last_y = new_y;
    }
}

void trails_render(MotionTrails *trails, BrailleCanvas *canvas) {
    if (!trails || !trails->enabled || !canvas) return;
    
    for (int t = 0; t < trails->num_tracked; t++) {
        JointTrail *trail = &trails->joints[t];
        
        /* Find valid points and sort by age */
        const TrailPoint *prev = NULL;
        
        for (int i = 0; i < TRAIL_HISTORY_SIZE; i++) {
            /* Read from oldest to newest */
            int idx = (trail->write_pos + i) % TRAIL_HISTORY_SIZE;
            TrailPoint *point = &trail->history[idx];
            
            if (!point->valid || point->alpha < 0.1f) continue;
            
            int px = (int)(point->x + 0.5f);
            int py = (int)(point->y + 0.5f);
            
            /* Draw point if bright enough */
            if (point->alpha > 0.3f) {
                braille_set_pixel(canvas, px, py, true);
            }
            
            /* Connect to previous point with line for smoother trail */
            if (prev && prev->valid && point->alpha > 0.2f && prev->alpha > 0.2f) {
                int prev_x = (int)(prev->x + 0.5f);
                int prev_y = (int)(prev->y + 0.5f);
                
                /* Only draw if points are close enough */
                float dist = sqrtf((px - prev_x) * (px - prev_x) + 
                                   (py - prev_y) * (py - prev_y));
                if (dist < 15) {
                    braille_draw_line(canvas, prev_x, prev_y, px, py);
                }
            }
            
            prev = point;
        }
    }
}

static void trails_clear(MotionTrails *trails) {
    if (!trails) return;
    
    for (int t = 0; t < TRAIL_JOINTS; t++) {
        for (int i = 0; i < TRAIL_HISTORY_SIZE; i++) {
            trails->joints[t].history[i].valid = false;
        }
        trails->joints[t].write_pos = 0;
        trails->joints[t].velocity = 0;
    }
}

void trails_set_enabled(MotionTrails *trails, bool enabled) {
    if (trails) {
        trails->enabled = enabled;
        if (!enabled) trails_clear(trails);
    }
}

bool trails_is_enabled(MotionTrails *trails) {
    return trails ? trails->enabled : false;
}

void trails_set_length(MotionTrails *trails, int length) {
    if (trails) {
        trails->trail_length = (length < TRAIL_HISTORY_SIZE) ? length : TRAIL_HISTORY_SIZE;
    }
}

void trails_set_fade_rate(MotionTrails *trails, float rate) {
    if (trails) {
        trails->fade_rate = (rate > 0 && rate < 1) ? rate : 0.7f;
    }
}
