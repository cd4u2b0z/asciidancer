/*
 * Advanced Skeleton Dancer Implementation
 * Rich audio-reactive animation with 30+ poses
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "skeleton_dancer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============ Easing Functions ============ */

float ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float ease_in_out_elastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    float c5 = (2.0f * M_PI) / 4.5f;
    if (t < 0.5f) {
        return -(powf(2.0f, 20.0f * t - 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f;
    }
    return (powf(2.0f, -20.0f * t + 10.0f) * sinf((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}

static float ease_out_back(float t) {
    float c1 = 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

static float ease_out_bounce(float t) {
    float n1 = 7.5625f;
    float d1 = 2.75f;
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

/* ============ Random Number Generator ============ */

static float random_float(SkeletonDancer *d) {
    d->random_state = d->random_state * 1103515245 + 12345;
    return (float)((d->random_state >> 16) & 0x7FFF) / 32767.0f;
}

static int random_int(SkeletonDancer *d, int max) {
    return (int)(random_float(d) * max);
}

/* ============ Joint Interpolation ============ */

Joint joint_lerp(Joint a, Joint b, float t) {
    return (Joint){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

static Joint joint_lerp_eased(Joint a, Joint b, float t) {
    float et = ease_in_out_cubic(t);
    return joint_lerp(a, b, et);
}

/* ============ Skeleton Setup ============ */

static void setup_humanoid_skeleton(SkeletonDef *skel) {
    int i = 0;
    skel->bones[i++] = (Bone){JOINT_HEAD, JOINT_NECK, 1, false, 0};
    skel->bones[i++] = (Bone){JOINT_NECK, JOINT_SHOULDER_L, 2, false, 0};
    skel->bones[i++] = (Bone){JOINT_NECK, JOINT_SHOULDER_R, 2, false, 0};
    skel->bones[i++] = (Bone){JOINT_SHOULDER_L, JOINT_ELBOW_L, 2, true, 0.15f};
    skel->bones[i++] = (Bone){JOINT_ELBOW_L, JOINT_HAND_L, 1, true, -0.1f};
    skel->bones[i++] = (Bone){JOINT_SHOULDER_R, JOINT_ELBOW_R, 2, true, -0.15f};
    skel->bones[i++] = (Bone){JOINT_ELBOW_R, JOINT_HAND_R, 1, true, 0.1f};
    skel->bones[i++] = (Bone){JOINT_NECK, JOINT_HIP_CENTER, 2, false, 0};
    skel->bones[i++] = (Bone){JOINT_HIP_CENTER, JOINT_HIP_L, 2, false, 0};
    skel->bones[i++] = (Bone){JOINT_HIP_CENTER, JOINT_HIP_R, 2, false, 0};
    skel->bones[i++] = (Bone){JOINT_HIP_L, JOINT_KNEE_L, 2, true, 0.12f};
    skel->bones[i++] = (Bone){JOINT_KNEE_L, JOINT_FOOT_L, 1, true, -0.08f};
    skel->bones[i++] = (Bone){JOINT_HIP_R, JOINT_KNEE_R, 2, true, -0.12f};
    skel->bones[i++] = (Bone){JOINT_KNEE_R, JOINT_FOOT_R, 1, true, 0.08f};
    skel->num_bones = i;
    skel->head_radius = 4;
}

/* ============ Pose Generation ============ */

#define DEG2RAD(d) ((d) * M_PI / 180.0f)

static Pose make_pose(const char *name, PoseCategory cat,
                      float head_x, float head_y,
                      float shoulder_angle, float lean,
                      float l_arm_upper, float l_arm_lower,
                      float r_arm_upper, float r_arm_lower,
                      float l_leg_upper, float l_leg_lower,
                      float r_leg_upper, float r_leg_lower,
                      float energy_min, float energy_max,
                      float bass_aff, float treble_aff) {
    Pose pose = {0};
    strncpy(pose.name, name, sizeof(pose.name) - 1);
    pose.category = cat;
    pose.energy_min = energy_min;
    pose.energy_max = energy_max;
    pose.bass_affinity = bass_aff;
    pose.treble_affinity = treble_aff;
    pose.num_joints = JOINT_COUNT;
    
    float lean_rad = DEG2RAD(lean);
    float shoulder_rad = DEG2RAD(shoulder_angle);
    
    /* Body dimensions */
    float head_size = 0.08f;
    float neck_len = 0.05f;
    float shoulder_width = 0.15f;
    float upper_arm = 0.12f;
    float lower_arm = 0.10f;
    float spine_len = 0.20f;
    float hip_width = 0.10f;
    float upper_leg = 0.15f;
    float lower_leg = 0.13f;
    
    pose.joints[JOINT_HEAD].x = head_x;
    pose.joints[JOINT_HEAD].y = head_y;
    
    pose.joints[JOINT_NECK].x = head_x + sinf(lean_rad) * neck_len;
    pose.joints[JOINT_NECK].y = head_y + head_size + neck_len;
    
    float neck_x = pose.joints[JOINT_NECK].x;
    float neck_y = pose.joints[JOINT_NECK].y;
    
    float sh_offset_x = cosf(lean_rad + shoulder_rad) * shoulder_width;
    float sh_offset_y = sinf(lean_rad + shoulder_rad) * shoulder_width;
    
    pose.joints[JOINT_SHOULDER_L].x = neck_x - sh_offset_x;
    pose.joints[JOINT_SHOULDER_L].y = neck_y + sh_offset_y * 0.3f;
    pose.joints[JOINT_SHOULDER_R].x = neck_x + sh_offset_x;
    pose.joints[JOINT_SHOULDER_R].y = neck_y - sh_offset_y * 0.3f;
    
    /* Arms */
    float l_up = DEG2RAD(l_arm_upper);
    pose.joints[JOINT_ELBOW_L].x = pose.joints[JOINT_SHOULDER_L].x + sinf(l_up) * upper_arm;
    pose.joints[JOINT_ELBOW_L].y = pose.joints[JOINT_SHOULDER_L].y + cosf(l_up) * upper_arm;
    float l_lo = DEG2RAD(l_arm_lower);
    pose.joints[JOINT_HAND_L].x = pose.joints[JOINT_ELBOW_L].x + sinf(l_lo) * lower_arm;
    pose.joints[JOINT_HAND_L].y = pose.joints[JOINT_ELBOW_L].y + cosf(l_lo) * lower_arm;
    
    float r_up = DEG2RAD(r_arm_upper);
    pose.joints[JOINT_ELBOW_R].x = pose.joints[JOINT_SHOULDER_R].x + sinf(r_up) * upper_arm;
    pose.joints[JOINT_ELBOW_R].y = pose.joints[JOINT_SHOULDER_R].y + cosf(r_up) * upper_arm;
    float r_lo = DEG2RAD(r_arm_lower);
    pose.joints[JOINT_HAND_R].x = pose.joints[JOINT_ELBOW_R].x + sinf(r_lo) * lower_arm;
    pose.joints[JOINT_HAND_R].y = pose.joints[JOINT_ELBOW_R].y + cosf(r_lo) * lower_arm;
    
    /* Hips */
    pose.joints[JOINT_HIP_CENTER].x = neck_x + sinf(lean_rad) * spine_len;
    pose.joints[JOINT_HIP_CENTER].y = neck_y + cosf(lean_rad) * spine_len;
    float hip_x = pose.joints[JOINT_HIP_CENTER].x;
    float hip_y = pose.joints[JOINT_HIP_CENTER].y;
    
    pose.joints[JOINT_HIP_L].x = hip_x - hip_width;
    pose.joints[JOINT_HIP_L].y = hip_y;
    pose.joints[JOINT_HIP_R].x = hip_x + hip_width;
    pose.joints[JOINT_HIP_R].y = hip_y;
    
    /* Legs */
    float ll_up = DEG2RAD(l_leg_upper);
    pose.joints[JOINT_KNEE_L].x = pose.joints[JOINT_HIP_L].x + sinf(ll_up) * upper_leg;
    pose.joints[JOINT_KNEE_L].y = pose.joints[JOINT_HIP_L].y + cosf(ll_up) * upper_leg;
    float ll_lo = DEG2RAD(l_leg_lower);
    pose.joints[JOINT_FOOT_L].x = pose.joints[JOINT_KNEE_L].x + sinf(ll_lo) * lower_leg;
    pose.joints[JOINT_FOOT_L].y = pose.joints[JOINT_KNEE_L].y + cosf(ll_lo) * lower_leg;
    
    float rl_up = DEG2RAD(r_leg_upper);
    pose.joints[JOINT_KNEE_R].x = pose.joints[JOINT_HIP_R].x + sinf(rl_up) * upper_leg;
    pose.joints[JOINT_KNEE_R].y = pose.joints[JOINT_HIP_R].y + cosf(rl_up) * upper_leg;
    float rl_lo = DEG2RAD(r_leg_lower);
    pose.joints[JOINT_FOOT_R].x = pose.joints[JOINT_KNEE_R].x + sinf(rl_lo) * lower_leg;
    pose.joints[JOINT_FOOT_R].y = pose.joints[JOINT_KNEE_R].y + cosf(rl_lo) * lower_leg;
    
    return pose;
}

static void add_pose(SkeletonDancer *d, Pose pose) {
    if (d->num_poses >= MAX_POSES) return;
    int idx = d->num_poses++;
    d->poses[idx] = pose;
    
    /* Add to category index */
    int cat = pose.category;
    if (d->category_counts[cat] < MAX_POSES) {
        d->poses_by_category[cat][d->category_counts[cat]++] = idx;
    }
}

static void add_all_poses(SkeletonDancer *d) {
    /* ========== IDLE POSES (very low energy) ========== */
    add_pose(d, make_pose("idle_stand", POSE_CAT_IDLE,
        0.5f, 0.1f, 0, 0,
        10, 5, -10, -5,     /* arms relaxed down */
        3, 0, -3, 0,        /* legs neutral */
        0.0f, 0.15f, 0.3f, 0.3f));
    
    add_pose(d, make_pose("idle_breathe", POSE_CAT_IDLE,
        0.5f, 0.11f, 0, 0,
        12, 8, -12, -8,
        2, 0, -2, 0,
        0.0f, 0.15f, 0.3f, 0.3f));
    
    add_pose(d, make_pose("idle_shift_l", POSE_CAT_IDLE,
        0.48f, 0.1f, -3, -5,
        15, 10, -8, -3,
        -5, 5, 8, -5,
        0.0f, 0.15f, 0.3f, 0.3f));
    
    add_pose(d, make_pose("idle_shift_r", POSE_CAT_IDLE,
        0.52f, 0.1f, 3, 5,
        8, 3, -15, -10,
        8, -5, -5, 5,
        0.0f, 0.15f, 0.3f, 0.3f));
    
    /* ========== CALM POSES (gentle swaying) ========== */
    add_pose(d, make_pose("calm_sway_l", POSE_CAT_CALM,
        0.47f, 0.1f, -5, -8,
        20, 15, -5, 0,
        -8, 8, 12, -8,
        0.1f, 0.3f, 0.4f, 0.4f));
    
    add_pose(d, make_pose("calm_sway_r", POSE_CAT_CALM,
        0.53f, 0.1f, 5, 8,
        5, 0, -20, -15,
        12, -8, -8, 8,
        0.1f, 0.3f, 0.4f, 0.4f));
    
    add_pose(d, make_pose("calm_nod", POSE_CAT_CALM,
        0.5f, 0.12f, 0, 3,
        15, 10, -15, -10,
        5, 0, -5, 0,
        0.1f, 0.3f, 0.5f, 0.3f));
    
    add_pose(d, make_pose("calm_arms_soft", POSE_CAT_CALM,
        0.5f, 0.1f, 0, 0,
        -20, 30, 20, -30,
        3, 0, -3, 0,
        0.1f, 0.3f, 0.3f, 0.5f));
    
    add_pose(d, make_pose("calm_lean_back", POSE_CAT_CALM,
        0.5f, 0.09f, 0, -5,
        25, 20, -25, -20,
        -5, 10, 5, -10,
        0.1f, 0.3f, 0.4f, 0.4f));
    
    /* ========== GROOVE POSES (medium energy, rhythmic) ========== */
    add_pose(d, make_pose("groove_bounce", POSE_CAT_GROOVE,
        0.5f, 0.08f, 0, 0,
        -30, 45, 30, -45,
        10, -15, -10, 15,
        0.25f, 0.55f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("groove_step_l", POSE_CAT_GROOVE,
        0.45f, 0.1f, -8, -12,
        -45, 60, 20, -10,
        -25, 40, 15, -10,
        0.25f, 0.55f, 0.7f, 0.3f));
    
    add_pose(d, make_pose("groove_step_r", POSE_CAT_GROOVE,
        0.55f, 0.1f, 8, 12,
        -20, 10, 45, -60,
        15, -10, -25, 40,
        0.25f, 0.55f, 0.7f, 0.3f));
    
    add_pose(d, make_pose("groove_arms_out", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, 0,
        -60, 30, 60, -30,
        8, -5, -8, 5,
        0.25f, 0.55f, 0.4f, 0.7f));
    
    add_pose(d, make_pose("groove_hip_l", POSE_CAT_GROOVE,
        0.48f, 0.1f, 10, -15,
        -40, 50, 15, 0,
        -20, 30, 25, -20,
        0.25f, 0.55f, 0.8f, 0.3f));
    
    add_pose(d, make_pose("groove_hip_r", POSE_CAT_GROOVE,
        0.52f, 0.1f, -10, 15,
        -15, 0, 40, -50,
        25, -20, -20, 30,
        0.25f, 0.55f, 0.8f, 0.3f));
    
    add_pose(d, make_pose("groove_clap_up", POSE_CAT_GROOVE,
        0.5f, 0.09f, 0, 0,
        -80, -60, 80, 60,
        5, 0, -5, 0,
        0.25f, 0.55f, 0.3f, 0.9f));
    
    add_pose(d, make_pose("groove_clap_down", POSE_CAT_GROOVE,
        0.5f, 0.11f, 0, 2,
        -30, 70, 30, -70,
        5, 0, -5, 0,
        0.25f, 0.55f, 0.3f, 0.8f));
    
    /* ========== ENERGETIC POSES (high energy) ========== */
    add_pose(d, make_pose("energy_arms_up", POSE_CAT_ENERGETIC,
        0.5f, 0.07f, 0, 0,
        -90, -45, 90, 45,
        15, -20, -15, 20,
        0.5f, 0.8f, 0.5f, 0.8f));
    
    add_pose(d, make_pose("energy_pump_l", POSE_CAT_ENERGETIC,
        0.48f, 0.08f, -5, -8,
        -120, -90, 30, 0,
        -15, 25, 20, -15,
        0.5f, 0.8f, 0.7f, 0.6f));
    
    add_pose(d, make_pose("energy_pump_r", POSE_CAT_ENERGETIC,
        0.52f, 0.08f, 5, 8,
        -30, 0, 120, 90,
        20, -15, -15, 25,
        0.5f, 0.8f, 0.7f, 0.6f));
    
    add_pose(d, make_pose("energy_wide", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 0, 0,
        -75, 20, 75, -20,
        25, -10, -25, 10,
        0.5f, 0.8f, 0.6f, 0.7f));
    
    add_pose(d, make_pose("energy_lean_l", POSE_CAT_ENERGETIC,
        0.42f, 0.12f, -15, -20,
        -60, 45, 45, -30,
        -35, 50, 30, -25,
        0.5f, 0.8f, 0.8f, 0.4f));
    
    add_pose(d, make_pose("energy_lean_r", POSE_CAT_ENERGETIC,
        0.58f, 0.12f, 15, 20,
        -45, 30, 60, -45,
        30, -25, -35, 50,
        0.5f, 0.8f, 0.8f, 0.4f));
    
    add_pose(d, make_pose("energy_twist", POSE_CAT_ENERGETIC,
        0.5f, 0.1f, 20, 10,
        -90, 30, 45, -60,
        20, -10, -30, 40,
        0.5f, 0.8f, 0.7f, 0.5f));
    
    /* ========== INTENSE POSES (very high energy, jumps) ========== */
    add_pose(d, make_pose("intense_jump", POSE_CAT_INTENSE,
        0.5f, 0.02f, 0, 0,
        -100, -60, 100, 60,
        35, -70, -35, 70,
        0.75f, 1.0f, 0.6f, 0.7f));
    
    add_pose(d, make_pose("intense_star", POSE_CAT_INTENSE,
        0.5f, 0.03f, 0, 0,
        -120, -30, 120, 30,
        45, -20, -45, 20,
        0.75f, 1.0f, 0.5f, 0.8f));
    
    add_pose(d, make_pose("intense_crouch", POSE_CAT_INTENSE,
        0.5f, 0.2f, 0, 5,
        -30, 60, 30, -60,
        45, -90, -45, 90,
        0.75f, 1.0f, 0.9f, 0.3f));
    
    add_pose(d, make_pose("intense_kick_l", POSE_CAT_INTENSE,
        0.55f, 0.1f, 10, 15,
        -60, 30, 75, -45,
        -60, 80, 10, -5,
        0.75f, 1.0f, 0.8f, 0.5f));
    
    add_pose(d, make_pose("intense_kick_r", POSE_CAT_INTENSE,
        0.45f, 0.1f, -10, -15,
        -75, 45, 60, -30,
        10, -5, -60, 80,
        0.75f, 1.0f, 0.8f, 0.5f));
    
    add_pose(d, make_pose("intense_spin", POSE_CAT_INTENSE,
        0.5f, 0.08f, 30, 25,
        -100, 20, 80, -70,
        40, -30, -20, 35,
        0.75f, 1.0f, 0.6f, 0.6f));
    
    /* ========== BASS HIT POSES (reactive to bass) ========== */
    add_pose(d, make_pose("bass_drop", POSE_CAT_BASS_HIT,
        0.5f, 0.15f, 0, 8,
        -20, 50, 20, -50,
        30, -50, -30, 50,
        0.3f, 1.0f, 1.0f, 0.2f));
    
    add_pose(d, make_pose("bass_stomp_l", POSE_CAT_BASS_HIT,
        0.48f, 0.12f, -5, -10,
        -40, 55, 25, -20,
        -40, 60, 20, -15,
        0.3f, 1.0f, 1.0f, 0.2f));
    
    add_pose(d, make_pose("bass_stomp_r", POSE_CAT_BASS_HIT,
        0.52f, 0.12f, 5, 10,
        -25, 20, 40, -55,
        20, -15, -40, 60,
        0.3f, 1.0f, 1.0f, 0.2f));
    
    add_pose(d, make_pose("bass_pulse", POSE_CAT_BASS_HIT,
        0.5f, 0.13f, 0, 5,
        -50, 40, 50, -40,
        20, -25, -20, 25,
        0.3f, 1.0f, 0.9f, 0.3f));
    
    /* ========== TREBLE ACCENT POSES (reactive to hi-hats, etc) ========== */
    add_pose(d, make_pose("treble_flick_l", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.1f, -3, -5,
        -100, -80, 15, 0,
        5, 0, -5, 0,
        0.2f, 1.0f, 0.2f, 1.0f));
    
    add_pose(d, make_pose("treble_flick_r", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.1f, 3, 5,
        -15, 0, 100, 80,
        5, 0, -5, 0,
        0.2f, 1.0f, 0.2f, 1.0f));
    
    add_pose(d, make_pose("treble_snap", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.09f, 0, 0,
        -85, -70, 85, 70,
        8, -3, -8, 3,
        0.2f, 1.0f, 0.3f, 0.9f));
    
    add_pose(d, make_pose("treble_wave", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.1f, 5, 3,
        -70, 50, -50, 80,
        5, 0, -5, 0,
        0.2f, 1.0f, 0.2f, 1.0f));
}

/* ============ Audio Analysis ============ */

static void update_beat_detector(BeatDetector *bd, float energy, float dt) {
    /* Add to history */
    bd->energy_history[bd->history_idx] = energy;
    bd->history_idx = (bd->history_idx + 1) % 64;
    
    /* Calculate average energy */
    float avg = 0;
    for (int i = 0; i < 64; i++) avg += bd->energy_history[i];
    avg /= 64.0f;
    
    /* Calculate variance for dynamic threshold */
    float variance = 0;
    for (int i = 0; i < 64; i++) {
        float diff = bd->energy_history[i] - avg;
        variance += diff * diff;
    }
    variance /= 64.0f;
    
    /* Beat threshold adapts to music dynamics */
    bd->beat_threshold = avg + sqrtf(variance) * 1.5f;
    
    /* Detect beat */
    bd->time_since_beat += dt;
    bd->beat_detected = false;
    
    if (energy > bd->beat_threshold && bd->time_since_beat > 0.15f) {
        bd->beat_detected = true;
        
        /* Estimate BPM */
        if (bd->time_since_beat < 2.0f) {
            float instant_bpm = 60.0f / bd->time_since_beat;
            bd->bpm_estimate = bd->bpm_estimate * 0.9f + instant_bpm * 0.1f;
        }
        
        bd->time_since_beat = 0;
        bd->beat_count++;
    }
}

static void analyze_audio(AudioAnalysis *a, float bass, float mid, float treble, float dt) {
    /* Raw input */
    a->bass = bass;
    a->mid = mid;
    a->treble = treble;
    
    /* Velocity (rate of change) */
    a->bass_velocity = (bass - a->bass_smooth) / (dt + 0.001f);
    a->mid_velocity = (mid - a->mid_smooth) / (dt + 0.001f);
    a->treble_velocity = (treble - a->treble_smooth) / (dt + 0.001f);
    
    /* Smooth values */
    float fast = 0.7f;
    a->bass_smooth = a->bass_smooth * fast + bass * (1.0f - fast);
    a->mid_smooth = a->mid_smooth * fast + mid * (1.0f - fast);
    a->treble_smooth = a->treble_smooth * fast + treble * (1.0f - fast);
    
    /* Peak tracking */
    if (bass > a->bass_peak) a->bass_peak = bass;
    else a->bass_peak *= 0.995f;
    if (mid > a->mid_peak) a->mid_peak = mid;
    else a->mid_peak *= 0.995f;
    if (treble > a->treble_peak) a->treble_peak = treble;
    else a->treble_peak *= 0.995f;
    
    /* Overall energy */
    a->energy = (bass * 1.2f + mid + treble * 0.8f) / 3.0f;
    a->energy_smooth = a->energy_smooth * 0.9f + a->energy * 0.1f;
    a->energy_long = a->energy_long * 0.995f + a->energy * 0.005f;
    
    /* Dynamics (how much energy varies) */
    float diff = fabsf(a->energy - a->energy_long);
    a->dynamics = a->dynamics * 0.98f + diff * 0.02f;
    
    /* Frequency ratios */
    float total = bass + mid + treble + 0.001f;
    a->bass_ratio = bass / total;
    a->treble_ratio = treble / total;
    a->spectral_centroid = (bass * 0.0f + mid * 0.5f + treble * 1.0f) / total;
    
    /* Beat detection */
    update_beat_detector(&a->beat, a->bass, dt);
    
    /* Style detection */
    if (a->bass_ratio > 0.5f && a->dynamics < 0.2f) {
        a->detected_style = STYLE_ELECTRONIC;
    } else if (a->bass_ratio > 0.45f && a->dynamics > 0.15f) {
        a->detected_style = STYLE_HIPHOP;
    } else if (a->energy_long < 0.2f) {
        a->detected_style = STYLE_AMBIENT;
    } else if (a->dynamics > 0.3f) {
        a->detected_style = STYLE_CLASSICAL;
    } else {
        a->detected_style = STYLE_ROCK;
    }
}

/* ============ Pose Selection ============ */

static bool pose_in_history(SkeletonDancer *d, int pose_idx) {
    for (int i = 0; i < POSE_HISTORY; i++) {
        if (d->pose_history[i] == pose_idx) return true;
    }
    return false;
}

static void add_to_history(SkeletonDancer *d, int pose_idx) {
    d->pose_history[d->history_idx] = pose_idx;
    d->history_idx = (d->history_idx + 1) % POSE_HISTORY;
}

static int select_pose_from_category(SkeletonDancer *d, PoseCategory cat) {
    if (d->category_counts[cat] == 0) return 0;
    
    /* Try to find a pose not in recent history */
    int attempts = 10;
    while (attempts-- > 0) {
        int idx = random_int(d, d->category_counts[cat]);
        int pose_idx = d->poses_by_category[cat][idx];
        
        if (!pose_in_history(d, pose_idx)) {
            return pose_idx;
        }
    }
    
    /* Fall back to random */
    int idx = random_int(d, d->category_counts[cat]);
    return d->poses_by_category[cat][idx];
}

static int select_best_pose(SkeletonDancer *d) {
    AudioAnalysis *a = &d->audio;
    float energy = a->energy_smooth;
    
    /* Determine primary category based on energy */
    PoseCategory primary_cat;
    if (energy < 0.15f) {
        primary_cat = POSE_CAT_IDLE;
    } else if (energy < 0.3f) {
        primary_cat = POSE_CAT_CALM;
    } else if (energy < 0.55f) {
        primary_cat = POSE_CAT_GROOVE;
    } else if (energy < 0.8f) {
        primary_cat = POSE_CAT_ENERGETIC;
    } else {
        primary_cat = POSE_CAT_INTENSE;
    }
    
    /* Check for frequency-specific triggers */
    if (a->beat.beat_detected && a->bass > 0.6f) {
        /* Bass hit - occasionally use bass poses */
        if (random_float(d) < 0.7f) {
            return select_pose_from_category(d, POSE_CAT_BASS_HIT);
        }
    }
    
    if (a->treble_velocity > 2.0f && a->treble > 0.5f) {
        /* Treble accent */
        if (random_float(d) < 0.5f) {
            return select_pose_from_category(d, POSE_CAT_TREBLE_ACCENT);
        }
    }
    
    /* Select from primary category */
    int pose_idx = select_pose_from_category(d, primary_cat);
    
    /* Verify energy range */
    Pose *p = &d->poses[pose_idx];
    if (energy < p->energy_min || energy > p->energy_max) {
        /* Try adjacent categories */
        if (primary_cat > 0 && random_float(d) < 0.3f) {
            pose_idx = select_pose_from_category(d, primary_cat - 1);
        } else if (primary_cat < POSE_CAT_INTENSE && random_float(d) < 0.3f) {
            pose_idx = select_pose_from_category(d, primary_cat + 1);
        }
    }
    
    return pose_idx;
}

/* ============ Physics Update ============ */

static void update_joint_physics(JointPhysics *jp, float dt) {
    /* Spring-damper system */
    float dx = jp->target.x - jp->position.x;
    float dy = jp->target.y - jp->position.y;
    
    /* Acceleration from spring */
    float ax = dx * jp->stiffness;
    float ay = dy * jp->stiffness;
    
    /* Apply acceleration */
    jp->velocity.x += ax * dt;
    jp->velocity.y += ay * dt;
    
    /* Apply damping */
    jp->velocity.x *= (1.0f - jp->damping * dt);
    jp->velocity.y *= (1.0f - jp->damping * dt);
    
    /* Update position */
    jp->position.x += jp->velocity.x * dt;
    jp->position.y += jp->velocity.y * dt;
}

/* ============ Main Update ============ */

void skeleton_dancer_update(SkeletonDancer *d, float bass, float mid, float treble, float dt) {
    if (!d) return;
    
    d->time_total += dt;
    d->time_in_pose += dt;
    
    /* Analyze audio */
    analyze_audio(&d->audio, bass, mid, treble, dt);
    AudioAnalysis *a = &d->audio;
    
    /* Determine animation tempo based on energy and detected BPM */
    float base_tempo = 0.3f;
    if (a->beat.bpm_estimate > 60.0f && a->beat.bpm_estimate < 200.0f) {
        base_tempo = a->beat.bpm_estimate / 120.0f;  /* Normalize around 120 BPM */
    }
    d->tempo = base_tempo * (0.5f + a->energy_smooth);
    
    /* Calculate pose duration based on tempo */
    float min_duration = 0.3f / d->tempo;
    float max_duration = 1.5f / d->tempo;
    d->pose_duration = min_duration + (1.0f - a->energy_smooth) * (max_duration - min_duration);
    
    /* Check if we should transition to a new pose */
    bool should_transition = false;
    
    if (d->time_in_pose > d->pose_duration) {
        should_transition = true;
    }
    
    /* Beat can trigger early transition at high energy */
    if (a->beat.beat_detected && a->energy_smooth > 0.5f && d->time_in_pose > 0.2f) {
        if (random_float(d) < 0.4f) {
            should_transition = true;
        }
    }
    
    if (should_transition) {
        d->time_in_pose = 0;
        d->pose_primary = d->pose_secondary;
        d->pose_secondary = select_best_pose(d);
        add_to_history(d, d->pose_secondary);
        d->blend = 0;
    }
    
    /* Update blend */
    float blend_speed = 3.0f + a->energy_smooth * 4.0f;
    d->blend += dt * blend_speed;
    if (d->blend > 1.0f) d->blend = 1.0f;
    
    /* Calculate modifiers based on frequency bands */
    
    /* Head bob - follows mid frequencies */
    float target_bob = sinf(d->time_total * 4.0f * d->tempo) * 0.02f * a->mid_smooth;
    d->head_bob = d->head_bob * 0.9f + target_bob * 0.1f;
    
    /* Arm swing - treble makes arms more active */
    float arm_phase = d->time_total * 3.0f * d->tempo;
    d->arm_swing_l = sinf(arm_phase) * 0.03f * a->treble_smooth;
    d->arm_swing_r = sinf(arm_phase + M_PI) * 0.03f * a->treble_smooth;
    
    /* Hip sway - bass drives hip movement */
    float hip_phase = d->time_total * 2.0f * d->tempo;
    d->hip_sway = sinf(hip_phase) * 0.02f * a->bass_smooth;
    
    /* Bounce - on beats */
    if (a->beat.beat_detected) {
        d->bounce = 0.03f * a->energy_smooth;
    }
    d->bounce *= 0.85f;  /* Decay */
    
    /* Lean - follows spectral centroid */
    d->lean = (a->spectral_centroid - 0.5f) * 0.03f;
    
    /* Interpolate base pose */
    Pose *p1 = &d->poses[d->pose_primary];
    Pose *p2 = &d->poses[d->pose_secondary];
    float eased_blend = ease_in_out_cubic(d->blend);
    
    for (int i = 0; i < JOINT_COUNT; i++) {
        /* Base interpolation */
        Joint target = joint_lerp(p1->joints[i], p2->joints[i], eased_blend);
        
        /* Apply modifiers */
        if (i == JOINT_HEAD) {
            target.y += d->head_bob - d->bounce;
        } else if (i == JOINT_HAND_L || i == JOINT_ELBOW_L) {
            target.x += d->arm_swing_l;
        } else if (i == JOINT_HAND_R || i == JOINT_ELBOW_R) {
            target.x += d->arm_swing_r;
        } else if (i == JOINT_HIP_CENTER || i == JOINT_HIP_L || i == JOINT_HIP_R) {
            target.x += d->hip_sway;
        }
        
        /* Global bounce and lean */
        target.y -= d->bounce * 0.5f;
        target.x += d->lean;
        
        /* Update physics */
        d->physics[i].target = target;
        
        /* Adjust physics parameters based on joint and energy */
        float stiffness = 15.0f + a->energy_smooth * 10.0f;
        float damping = 8.0f;
        
        /* Extremities are looser */
        if (i == JOINT_HAND_L || i == JOINT_HAND_R || 
            i == JOINT_FOOT_L || i == JOINT_FOOT_R) {
            stiffness *= 0.7f;
            damping *= 0.8f;
        }
        
        d->physics[i].stiffness = stiffness;
        d->physics[i].damping = damping;
        
        update_joint_physics(&d->physics[i], dt);
        d->current[i] = d->physics[i].position;
    }
    
    /* Advance phase */
    d->phase += dt * d->tempo;
}

/* ============ Rendering ============ */

static void joint_to_pixel(SkeletonDancer *d, Joint j, int *px, int *py) {
    *px = (int)((j.x - 0.5f) * d->scale + d->offset_x);
    *py = (int)(j.y * d->scale + d->offset_y);
}

void skeleton_dancer_render(SkeletonDancer *d, BrailleCanvas *canvas) {
    if (!d || !canvas) return;
    
    /* NOTE: Canvas should be cleared by caller before this function */
    
    /* Draw bones */
    for (int i = 0; i < d->skeleton.num_bones; i++) {
        Bone *bone = &d->skeleton.bones[i];
        
        int x1, y1, x2, y2;
        joint_to_pixel(d, d->current[bone->from], &x1, &y1);
        joint_to_pixel(d, d->current[bone->to], &x2, &y2);
        
        if (bone->is_curve && fabsf(bone->curve_amount) > 0.01f) {
            int cx = (x1 + x2) / 2;
            int cy = (y1 + y2) / 2;
            float dx = x2 - x1;
            float dy = y2 - y1;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0.001f) {
                float curve_offset = len * bone->curve_amount;
                cx += (int)(-dy / len * curve_offset);
                cy += (int)(dx / len * curve_offset);
            }
            braille_draw_bezier_quad(canvas, x1, y1, cx, cy, x2, y2);
        } else {
            if (bone->thickness > 1) {
                braille_draw_thick_line(canvas, x1, y1, x2, y2, bone->thickness);
            } else {
                braille_draw_line(canvas, x1, y1, x2, y2);
            }
        }
    }
    
    /* Draw head */
    int head_x, head_y;
    joint_to_pixel(d, d->current[JOINT_HEAD], &head_x, &head_y);
    braille_fill_circle(canvas, head_x, head_y, d->skeleton.head_radius);
    
    /* Draw hands */
    int hx, hy;
    joint_to_pixel(d, d->current[JOINT_HAND_L], &hx, &hy);
    braille_fill_circle(canvas, hx, hy, 2);
    joint_to_pixel(d, d->current[JOINT_HAND_R], &hx, &hy);
    braille_fill_circle(canvas, hx, hy, 2);
    
    braille_canvas_render(canvas);
}

/* ============ Creation/Destruction ============ */

SkeletonDancer* skeleton_dancer_create(int canvas_cell_width, int canvas_cell_height) {
    SkeletonDancer *d = calloc(1, sizeof(SkeletonDancer));
    if (!d) return NULL;
    
    d->canvas_width = canvas_cell_width * BRAILLE_CELL_W;
    d->canvas_height = canvas_cell_height * BRAILLE_CELL_H;
    
    float scale_x = d->canvas_width * 0.8f;
    float scale_y = d->canvas_height * 0.8f;
    d->scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    d->offset_x = d->canvas_width / 2.0f;
    d->offset_y = d->canvas_height * 0.1f;
    
    /* Initialize random state */
    d->random_state = 12345;
    
    /* Setup skeleton */
    setup_humanoid_skeleton(&d->skeleton);
    
    /* Add all poses */
    add_all_poses(d);
    
    /* Initialize pose history */
    for (int i = 0; i < POSE_HISTORY; i++) {
        d->pose_history[i] = -1;
    }
    
    /* Start with idle pose */
    d->pose_primary = 0;
    d->pose_secondary = 0;
    d->blend = 1.0f;
    d->pose_duration = 1.0f;
    
    /* Initialize physics */
    for (int i = 0; i < JOINT_COUNT; i++) {
        d->physics[i].position = d->poses[0].joints[i];
        d->physics[i].target = d->poses[0].joints[i];
        d->physics[i].velocity = (Joint){0, 0};
        d->physics[i].stiffness = 15.0f;
        d->physics[i].damping = 8.0f;
        d->current[i] = d->poses[0].joints[i];
    }
    
    /* Initialize beat detector */
    d->audio.beat.beat_threshold = 0.5f;
    d->audio.beat.bpm_estimate = 120.0f;
    
    return d;
}

void skeleton_dancer_destroy(SkeletonDancer *d) {
    free(d);
}
