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

static float ease_in_out_cubic(float t) {
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

static float __attribute__((unused)) ease_out_back(float t) {
    float c1 = 1.70158f;
    float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}

static float __attribute__((unused)) ease_out_bounce(float t) {
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

static Joint joint_lerp(Joint a, Joint b, float t) {
    return (Joint){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

static Joint __attribute__((unused)) joint_lerp_eased(Joint a, Joint b, float t) {
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
    pose.facing = 0.0f;      /* Default: facing forward */
    pose.dip_amount = 0.0f;  /* Default: no dip */
    
    float lean_rad = DEG2RAD(lean);
    float shoulder_rad = DEG2RAD(shoulder_angle);
    
    /* Body dimensions - adjusted for better human proportions (v2.4 polish)
     * Head is smaller relative to body, legs are longer for better silhouette
     */
    float head_size = 0.06f;       /* Was 0.08f - smaller head */
    float neck_len = 0.04f;        /* Was 0.05f - shorter neck */
    float shoulder_width = 0.14f;  /* Was 0.15f - slightly narrower */
    float upper_arm = 0.10f;       /* Was 0.12f - shorter arms */
    float lower_arm = 0.09f;       /* Was 0.10f */
    float spine_len = 0.16f;       /* Was 0.20f - shorter torso */
    float hip_width = 0.08f;       /* Was 0.10f - narrower hips */
    float upper_leg = 0.18f;       /* Was 0.15f - LONGER legs */
    float lower_leg = 0.16f;       /* Was 0.13f - LONGER lower legs */
    
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

/* Forward declaration for procedural pose generation */
static void generate_pose_variations(SkeletonDancer *d);

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
    
    /* ========== ADDITIONAL BASS HITS ========== */
    add_pose(d, make_pose("bass_slam", POSE_CAT_BASS_HIT,
        0.5f, 0.16f, 0, 12,
        -15, 45, 15, -45,
        35, -55, -35, 55,
        0.4f, 1.0f, 1.0f, 0.1f));
    
    add_pose(d, make_pose("bass_bounce_l", POSE_CAT_BASS_HIT,
        0.45f, 0.14f, -8, 6,
        -35, 50, 20, -30,
        -30, 50, 25, -20,
        0.35f, 1.0f, 0.95f, 0.2f));
    
    add_pose(d, make_pose("bass_bounce_r", POSE_CAT_BASS_HIT,
        0.55f, 0.14f, 8, 6,
        -20, 30, 35, -50,
        25, -20, -30, 50,
        0.35f, 1.0f, 0.95f, 0.2f));
    
    add_pose(d, make_pose("bass_chest_pop", POSE_CAT_BASS_HIT,
        0.5f, 0.11f, 0, -3,
        -40, 35, 40, -35,
        15, -10, -15, 10,
        0.4f, 1.0f, 0.9f, 0.3f));
    
    /* ========== ADDITIONAL TREBLE ACCENTS ========== */
    add_pose(d, make_pose("treble_double_flick", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.08f, 0, 0,
        -95, -75, 95, 75,
        6, -2, -6, 2,
        0.25f, 1.0f, 0.2f, 1.0f));
    
    add_pose(d, make_pose("treble_shimmy", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.1f, 10, 0,
        -60, 40, 70, -50,
        8, -5, -8, 5,
        0.2f, 1.0f, 0.25f, 0.95f));
    
    add_pose(d, make_pose("treble_pop", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.09f, -5, -3,
        -80, -50, 60, 30,
        10, -5, -10, 5,
        0.2f, 1.0f, 0.3f, 0.9f));
    
    /* ========== HIP HOP MOVES ========== */
    add_pose(d, make_pose("hiphop_step", POSE_CAT_GROOVE,
        0.48f, 0.1f, -6, 5,
        -45, 55, 30, -25,
        -25, 40, 20, -15,
        0.3f, 0.6f, 0.7f, 0.4f));
    
    add_pose(d, make_pose("hiphop_bounce", POSE_CAT_GROOVE,
        0.5f, 0.12f, 0, 8,
        -35, 50, 35, -50,
        20, -30, -20, 30,
        0.3f, 0.6f, 0.75f, 0.35f));
    
    add_pose(d, make_pose("hiphop_lean", POSE_CAT_GROOVE,
        0.53f, 0.1f, 10, 12,
        -50, 40, 25, -15,
        15, -10, -25, 35,
        0.3f, 0.6f, 0.65f, 0.4f));
    
    add_pose(d, make_pose("hiphop_rock", POSE_CAT_GROOVE,
        0.47f, 0.11f, -10, 8,
        -30, 45, 50, -40,
        -20, 35, 30, -25,
        0.3f, 0.6f, 0.7f, 0.35f));
    
    /* ========== POPPING MOVES ========== */
    add_pose(d, make_pose("pop_arm_l", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, -3, 0,
        -90, 0, 40, -30,
        8, -3, -8, 3,
        0.45f, 0.8f, 0.5f, 0.7f));
    
    add_pose(d, make_pose("pop_arm_r", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 3, 0,
        -40, 30, 90, 0,
        -8, 3, 8, -3,
        0.45f, 0.8f, 0.5f, 0.7f));
    
    add_pose(d, make_pose("pop_chest", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -5,
        -50, 35, 50, -35,
        10, -5, -10, 5,
        0.5f, 0.85f, 0.6f, 0.6f));
    
    add_pose(d, make_pose("pop_neck", POSE_CAT_ENERGETIC,
        0.52f, 0.1f, 8, 0,
        -35, 40, 45, -35,
        5, 0, -5, 0,
        0.45f, 0.8f, 0.4f, 0.7f));
    
    /* ========== LOCKING MOVES ========== */
    add_pose(d, make_pose("lock_point_l", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, -5, -3,
        -130, -100, 25, 10,
        10, -5, -5, 0,
        0.5f, 0.85f, 0.4f, 0.8f));
    
    add_pose(d, make_pose("lock_point_r", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 5, -3,
        -25, -10, 130, 100,
        -5, 0, 10, -5,
        0.5f, 0.85f, 0.4f, 0.8f));
    
    add_pose(d, make_pose("lock_freeze", POSE_CAT_ENERGETIC,
        0.5f, 0.1f, 0, 0,
        -85, -40, 85, 40,
        15, -8, -15, 8,
        0.55f, 0.9f, 0.5f, 0.7f));
    
    add_pose(d, make_pose("lock_wrist", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 3, -2,
        -70, 60, 80, -70,
        8, -3, -8, 3,
        0.5f, 0.85f, 0.45f, 0.75f));
    
    /* ========== HOUSE DANCE MOVES ========== */
    add_pose(d, make_pose("house_jack_up", POSE_CAT_GROOVE,
        0.5f, 0.07f, 0, -5,
        -55, 30, 55, -30,
        20, -25, -20, 25,
        0.3f, 0.6f, 0.6f, 0.5f));
    
    add_pose(d, make_pose("house_jack_down", POSE_CAT_GROOVE,
        0.5f, 0.13f, 0, 8,
        -40, 50, 40, -50,
        30, -40, -30, 40,
        0.3f, 0.6f, 0.65f, 0.45f));
    
    add_pose(d, make_pose("house_stomp_l", POSE_CAT_GROOVE,
        0.45f, 0.11f, -8, 5,
        -50, 45, 30, -20,
        -35, 55, 20, -10,
        0.3f, 0.6f, 0.7f, 0.4f));
    
    add_pose(d, make_pose("house_stomp_r", POSE_CAT_GROOVE,
        0.55f, 0.11f, 8, 5,
        -30, 20, 50, -45,
        20, -10, -35, 55,
        0.3f, 0.6f, 0.7f, 0.4f));
    
    /* ========== VOGUING MOVES ========== */
    add_pose(d, make_pose("vogue_arms_frame", POSE_CAT_GROOVE,
        0.5f, 0.09f, 0, -3,
        -105, -45, 105, 45,
        5, 0, -5, 0,
        0.35f, 0.65f, 0.3f, 0.85f));
    
    add_pose(d, make_pose("vogue_dip", POSE_CAT_GROOVE,
        0.5f, 0.15f, 0, 15,
        -80, 50, 80, -50,
        40, -70, -40, 70,
        0.35f, 0.65f, 0.5f, 0.7f));
    
    add_pose(d, make_pose("vogue_hand_l", POSE_CAT_GROOVE,
        0.48f, 0.1f, -5, 0,
        -95, -70, 30, 20,
        5, 0, -5, 0,
        0.3f, 0.6f, 0.3f, 0.9f));
    
    add_pose(d, make_pose("vogue_hand_r", POSE_CAT_GROOVE,
        0.52f, 0.1f, 5, 0,
        -30, -20, 95, 70,
        -5, 0, 5, 0,
        0.3f, 0.6f, 0.3f, 0.9f));
    
    /* ========== KRUMP MOVES ========== */
    add_pose(d, make_pose("krump_stomp", POSE_CAT_INTENSE,
        0.5f, 0.14f, 0, 10,
        -45, 60, 45, -60,
        35, -55, -35, 55,
        0.7f, 1.0f, 0.9f, 0.3f));
    
    add_pose(d, make_pose("krump_chest_pop", POSE_CAT_INTENSE,
        0.5f, 0.08f, 0, -8,
        -60, 25, 60, -25,
        15, -10, -15, 10,
        0.7f, 1.0f, 0.8f, 0.4f));
    
    add_pose(d, make_pose("krump_arm_swing", POSE_CAT_INTENSE,
        0.48f, 0.1f, -10, 5,
        -110, -50, 70, -40,
        -20, 35, 25, -20,
        0.75f, 1.0f, 0.7f, 0.5f));
    
    add_pose(d, make_pose("krump_buck", POSE_CAT_INTENSE,
        0.5f, 0.12f, 15, 8,
        -55, 45, 75, -55,
        25, -35, -30, 45,
        0.75f, 1.0f, 0.85f, 0.35f));
    
    /* ========== TUTTING MOVES ========== */
    add_pose(d, make_pose("tut_box_l", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, 0,
        -90, -90, 45, 45,
        5, 0, -5, 0,
        0.25f, 0.55f, 0.3f, 0.8f));
    
    add_pose(d, make_pose("tut_box_r", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, 0,
        -45, -45, 90, 90,
        -5, 0, 5, 0,
        0.25f, 0.55f, 0.3f, 0.8f));
    
    add_pose(d, make_pose("tut_king", POSE_CAT_GROOVE,
        0.5f, 0.09f, 0, -2,
        -90, 0, 90, 0,
        8, -3, -8, 3,
        0.3f, 0.6f, 0.35f, 0.85f));
    
    add_pose(d, make_pose("tut_pharaoh", POSE_CAT_GROOVE,
        0.5f, 0.09f, 0, 0,
        -90, 90, 90, -90,
        5, 0, -5, 0,
        0.3f, 0.6f, 0.3f, 0.9f));
    
    /* ========== WAVING MOVES ========== */
    add_pose(d, make_pose("wave_arm_1", POSE_CAT_CALM,
        0.5f, 0.1f, 0, 0,
        -80, -30, 40, 20,
        3, 0, -3, 0,
        0.15f, 0.4f, 0.3f, 0.7f));
    
    add_pose(d, make_pose("wave_arm_2", POSE_CAT_CALM,
        0.5f, 0.1f, 0, 0,
        -60, 10, 60, -10,
        3, 0, -3, 0,
        0.15f, 0.4f, 0.3f, 0.7f));
    
    add_pose(d, make_pose("wave_arm_3", POSE_CAT_CALM,
        0.5f, 0.1f, 0, 0,
        -40, 20, 80, 30,
        3, 0, -3, 0,
        0.15f, 0.4f, 0.3f, 0.7f));
    
    add_pose(d, make_pose("wave_body", POSE_CAT_CALM,
        0.52f, 0.1f, 5, 3,
        -50, 30, 60, -40,
        10, -5, -8, 3,
        0.15f, 0.4f, 0.4f, 0.6f));
    
    /* ========== FLEXING MOVES ========== */
    add_pose(d, make_pose("flex_double", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -3,
        -110, -90, 110, 90,
        10, -5, -10, 5,
        0.5f, 0.8f, 0.6f, 0.6f));
    
    add_pose(d, make_pose("flex_side_l", POSE_CAT_ENERGETIC,
        0.48f, 0.09f, -5, 0,
        -120, -85, 35, 20,
        5, 0, -8, 3,
        0.5f, 0.8f, 0.55f, 0.65f));
    
    add_pose(d, make_pose("flex_side_r", POSE_CAT_ENERGETIC,
        0.52f, 0.09f, 5, 0,
        -35, -20, 120, 85,
        8, -3, -5, 0,
        0.5f, 0.8f, 0.55f, 0.65f));
    
    /* ========== CELEBRATION MOVES ========== */
    add_pose(d, make_pose("celebrate_v", POSE_CAT_ENERGETIC,
        0.5f, 0.07f, 0, -5,
        -120, -60, 120, 60,
        15, -10, -15, 10,
        0.55f, 0.9f, 0.4f, 0.85f));
    
    add_pose(d, make_pose("celebrate_yeah", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -3,
        -100, -70, 45, 20,
        12, -8, -8, 5,
        0.55f, 0.85f, 0.45f, 0.8f));
    
    add_pose(d, make_pose("celebrate_wave", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 5, 0,
        -90, -50, 90, 50,
        10, -5, -10, 5,
        0.5f, 0.85f, 0.4f, 0.8f));
    
    /* ========== NEW: FUNKY MOVES (groove with style) ========== */
    add_pose(d, make_pose("funky_robot_l", POSE_CAT_GROOVE,
        0.5f, 0.1f, -5, 0,
        -90, 0, 45, -90,      /* Robot arm angles */
        10, -5, -10, 5,
        0.25f, 0.6f, 0.5f, 0.6f));
    
    add_pose(d, make_pose("funky_robot_r", POSE_CAT_GROOVE,
        0.5f, 0.1f, 5, 0,
        -45, 90, 90, 0,       /* Mirror robot */
        -10, 5, 10, -5,
        0.25f, 0.6f, 0.5f, 0.6f));
    
    add_pose(d, make_pose("funky_disco_point", POSE_CAT_GROOVE,
        0.5f, 0.08f, 5, -5,
        -130, -100, 30, 0,    /* Classic disco point up */
        15, -10, -5, 0,
        0.3f, 0.7f, 0.4f, 0.8f));
    
    add_pose(d, make_pose("funky_strut", POSE_CAT_GROOVE,
        0.52f, 0.1f, 8, 10,
        -50, 35, 40, -25,
        -20, 35, 30, -20,     /* Strutting walk */
        0.3f, 0.6f, 0.6f, 0.5f));
    
    add_pose(d, make_pose("funky_shoulder_roll", POSE_CAT_GROOVE,
        0.5f, 0.11f, 15, 5,
        -35, 60, -25, 40,     /* Asymmetric shoulders */
        8, -3, -8, 3,
        0.25f, 0.55f, 0.5f, 0.5f));
    
    /* ========== NEW: WAVE ARMS (smooth flowing) ========== */
    add_pose(d, make_pose("wave_left_high", POSE_CAT_CALM,
        0.5f, 0.1f, 0, -3,
        -110, -60, 20, 30,    /* Left arm up in wave */
        3, 0, -3, 0,
        0.15f, 0.4f, 0.3f, 0.7f));
    
    add_pose(d, make_pose("wave_both_up", POSE_CAT_CALM,
        0.5f, 0.09f, 0, 0,
        -100, -50, 100, 50,   /* Both arms waving high */
        5, 0, -5, 0,
        0.15f, 0.45f, 0.3f, 0.8f));
    
    add_pose(d, make_pose("wave_flow_l", POSE_CAT_CALM,
        0.48f, 0.1f, -5, -3,
        -80, 40, -40, 60,     /* Flowing wave motion */
        -5, 8, 10, -8,
        0.1f, 0.35f, 0.4f, 0.6f));
    
    add_pose(d, make_pose("wave_flow_r", POSE_CAT_CALM,
        0.52f, 0.1f, 5, 3,
        40, -60, 80, -40,     /* Mirror flow */
        10, -8, -5, 8,
        0.1f, 0.35f, 0.4f, 0.6f));
    
    /* ========== NEW: HEAD BOB VARIANTS ========== */
    add_pose(d, make_pose("headbang_down", POSE_CAT_ENERGETIC,
        0.5f, 0.13f, 0, 12,   /* Head down (neck bent forward) */
        -25, 40, 25, -40,
        10, -5, -10, 5,
        0.4f, 0.75f, 0.7f, 0.4f));
    
    add_pose(d, make_pose("headbang_back", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -8,   /* Head back */
        -40, 30, 40, -30,
        8, -3, -8, 3,
        0.4f, 0.75f, 0.6f, 0.5f));
    
    add_pose(d, make_pose("head_tilt_l", POSE_CAT_GROOVE,
        0.48f, 0.1f, -8, 0,   /* Tilt head left */
        -30, 45, -20, 30,
        5, 0, -5, 0,
        0.2f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("head_tilt_r", POSE_CAT_GROOVE,
        0.52f, 0.1f, 8, 0,    /* Tilt head right */
        20, -30, 30, -45,
        -5, 0, 5, 0,
        0.2f, 0.5f, 0.5f, 0.5f));
    
    /* ========== NEW: BREAKDANCE INSPIRED ========== */
    add_pose(d, make_pose("break_freeze_l", POSE_CAT_INTENSE,
        0.4f, 0.15f, -20, -25,
        -70, 90, 120, 60,     /* Dramatic freeze pose */
        -50, 70, 25, -15,
        0.7f, 1.0f, 0.8f, 0.5f));
    
    add_pose(d, make_pose("break_freeze_r", POSE_CAT_INTENSE,
        0.6f, 0.15f, 20, 25,
        -120, -60, 70, -90,   /* Mirror freeze */
        25, -15, -50, 70,
        0.7f, 1.0f, 0.8f, 0.5f));
    
    add_pose(d, make_pose("break_toprock", POSE_CAT_ENERGETIC,
        0.5f, 0.1f, 10, 8,
        -55, 40, 70, -50,
        -30, 45, 25, -35,     /* Toprock step */
        0.5f, 0.85f, 0.75f, 0.4f));
    
    add_pose(d, make_pose("break_windmill_prep", POSE_CAT_INTENSE,
        0.45f, 0.18f, -15, 20,
        -30, 90, 60, -70,     /* Getting low */
        -40, 80, 30, -60,
        0.75f, 1.0f, 0.9f, 0.3f));
    
    /* ========== NEW: SMOOTH GROOVES ========== */
    add_pose(d, make_pose("smooth_slide_l", POSE_CAT_GROOVE,
        0.42f, 0.1f, -12, -10,
        -35, 50, 10, 0,
        -30, 50, 20, -15,     /* Smooth slide left */
        0.25f, 0.55f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("smooth_slide_r", POSE_CAT_GROOVE,
        0.58f, 0.1f, 12, 10,
        -10, 0, 35, -50,
        20, -15, -30, 50,     /* Smooth slide right */
        0.25f, 0.55f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("smooth_body_roll", POSE_CAT_GROOVE,
        0.5f, 0.12f, 5, 8,
        -40, 55, 40, -55,     /* Body rolling motion */
        15, -20, -15, 20,
        0.3f, 0.6f, 0.7f, 0.4f));
    
    add_pose(d, make_pose("smooth_isolation", POSE_CAT_GROOVE,
        0.5f, 0.1f, -8, 5,
        -50, 30, 60, -40,     /* Chest isolation feel */
        5, 0, -5, 0,
        0.25f, 0.55f, 0.5f, 0.5f));
    
    /* ========== NEW: PARTY MOVES ========== */
    add_pose(d, make_pose("party_hands_up", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -5,
        -95, -55, 95, 55,     /* Hands up! */
        10, -8, -10, 8,
        0.5f, 0.85f, 0.4f, 0.9f));
    
    add_pose(d, make_pose("party_fist_pump", POSE_CAT_ENERGETIC,
        0.5f, 0.07f, 0, -3,
        -110, -80, 40, 20,    /* Fist pump! */
        12, -10, -8, 5,
        0.55f, 0.9f, 0.6f, 0.7f));
    
    add_pose(d, make_pose("party_double_pump", POSE_CAT_ENERGETIC,
        0.5f, 0.06f, 0, 0,
        -115, -85, 115, 85,   /* Double fist pump */
        15, -12, -15, 12,
        0.6f, 0.95f, 0.5f, 0.8f));
    
    add_pose(d, make_pose("party_jump_prep", POSE_CAT_ENERGETIC,
        0.5f, 0.14f, 0, 5,
        -30, 50, 30, -50,     /* Getting ready to jump */
        25, -40, -25, 40,
        0.5f, 0.8f, 0.8f, 0.4f));
    
    /* ========== NEW: MOONWALK / GLIDE POSES ========== */
    add_pose(d, make_pose("glide_prep", POSE_CAT_GROOVE,
        0.5f, 0.09f, 3, 3,
        -25, 35, 25, -35,
        -5, 20, 15, -25,      /* Weight shifting */
        0.3f, 0.55f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("glide_slide", POSE_CAT_GROOVE,
        0.5f, 0.1f, -3, 0,
        -30, 40, 30, -40,
        15, -30, -20, 45,     /* Sliding motion */
        0.3f, 0.55f, 0.65f, 0.35f));
    
    /* ========== NEW: DRAMATIC POSES ========== */
    add_pose(d, make_pose("dramatic_reach", POSE_CAT_INTENSE,
        0.5f, 0.08f, 0, -10,
        -135, -90, 50, 30,    /* Reaching for the sky */
        10, -5, -10, 5,
        0.6f, 1.0f, 0.4f, 0.9f));
    
    add_pose(d, make_pose("dramatic_pose", POSE_CAT_INTENSE,
        0.55f, 0.1f, 15, 10,
        -80, 20, 100, -50,    /* Dramatic stance */
        -25, 40, 35, -25,
        0.65f, 1.0f, 0.6f, 0.7f));
    
    add_pose(d, make_pose("dramatic_bow", POSE_CAT_CALM,
        0.5f, 0.18f, 0, 25,   /* Taking a bow */
        10, 30, -10, -30,
        10, 0, -10, 0,
        0.0f, 0.3f, 0.5f, 0.5f));
    
    /* ========== NEW: ADDITIONAL IDLE VARIATIONS ========== */
    add_pose(d, make_pose("idle_sway", POSE_CAT_IDLE,
        0.5f, 0.1f, 3, 2,
        15, 10, -10, -5,
        5, -3, -3, 2,
        0.0f, 0.12f, 0.4f, 0.4f));
    
    add_pose(d, make_pose("idle_arms_cross", POSE_CAT_IDLE,
        0.5f, 0.1f, 0, 0,
        30, 75, -30, -75,     /* Arms crossed look */
        2, 0, -2, 0,
        0.0f, 0.15f, 0.3f, 0.3f));
    
    /* ========== NEW: SPIN POSES ========== */
    add_pose(d, make_pose("spin_wind_l", POSE_CAT_INTENSE,
        0.48f, 0.09f, -25, -15,
        -60, 20, 90, -40,     /* Spinning left */
        -35, 55, 30, -40,
        0.7f, 1.0f, 0.6f, 0.6f));
    
    add_pose(d, make_pose("spin_wind_r", POSE_CAT_INTENSE,
        0.52f, 0.09f, 25, 15,
        -90, 40, 60, -20,     /* Spinning right */
        30, -40, -35, 55,
        0.7f, 1.0f, 0.6f, 0.6f));
    
    add_pose(d, make_pose("spin_arms_out", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 15, 10,
        -75, 15, 75, -15,     /* Arms out spinning */
        20, -15, -20, 15,
        0.55f, 0.85f, 0.5f, 0.7f));
    
    /* ========== MASSIVE EXPANSION: GROOVE VARIATIONS ========== */
    /* Subtle head tilts and body shifts for more natural movement */
    add_pose(d, make_pose("groove_tilt_l", POSE_CAT_GROOVE,
        0.48f, 0.09f, -8, 5,
        -20, 25, 15, -20,
        10, -15, -8, 12,
        0.25f, 0.45f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("groove_tilt_r", POSE_CAT_GROOVE,
        0.52f, 0.09f, 8, -5,
        -15, 20, 20, -25,
        -8, 12, 10, -15,
        0.25f, 0.45f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("groove_sink", POSE_CAT_GROOVE,
        0.5f, 0.12f, 0, 8,
        -25, 35, 25, -35,
        12, -18, -12, 18,
        0.35f, 0.55f, 0.55f, 0.45f));
    
    add_pose(d, make_pose("groove_rise", POSE_CAT_GROOVE,
        0.5f, 0.07f, 0, -5,
        -30, 30, 30, -30,
        8, -12, -8, 12,
        0.2f, 0.4f, 0.45f, 0.55f));
    
    /* ========== ROBOT / MECHANICAL MOVES ========== */
    add_pose(d, make_pose("robot_arm_l", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 0, 0,
        -90, 0, 0, 0,
        5, -5, -5, 5,
        0.4f, 0.65f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("robot_arm_r", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 0, 0,
        0, 0, 90, 0,
        5, -5, -5, 5,
        0.4f, 0.65f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("robot_arms_up", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, 0,
        -90, -90, 90, 90,
        0, 0, 0, 0,
        0.35f, 0.6f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("robot_step_l", POSE_CAT_ENERGETIC,
        0.48f, 0.1f, 0, 0,
        -45, 90, 45, -90,
        -30, 45, 15, -20,
        0.45f, 0.7f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("robot_step_r", POSE_CAT_ENERGETIC,
        0.52f, 0.1f, 0, 0,
        -45, -90, 45, 90,
        15, -20, -30, 45,
        0.45f, 0.7f, 0.4f, 0.6f));
    
    /* ========== WAVE DANCE ========== */
    add_pose(d, make_pose("wave_start", POSE_CAT_GROOVE,
        0.5f, 0.09f, -5, 3,
        -100, -60, 30, 20,
        8, -10, -8, 10,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("wave_mid_l", POSE_CAT_GROOVE,
        0.5f, 0.09f, -8, 5,
        -70, -30, 45, 10,
        10, -12, -10, 12,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("wave_mid_r", POSE_CAT_GROOVE,
        0.5f, 0.09f, 8, -5,
        -45, -10, 70, 30,
        10, -12, -10, 12,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("wave_end", POSE_CAT_GROOVE,
        0.5f, 0.09f, 5, -3,
        -30, -20, 100, 60,
        8, -10, -8, 10,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    /* ========== BOUNCE VARIATIONS ========== */
    add_pose(d, make_pose("bounce_low", POSE_CAT_ENERGETIC,
        0.5f, 0.13f, 0, 10,
        -40, 45, 40, -45,
        18, -25, -18, 25,
        0.5f, 0.75f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("bounce_high", POSE_CAT_ENERGETIC,
        0.5f, 0.05f, 0, -8,
        -35, 30, 35, -30,
        5, -8, -5, 8,
        0.4f, 0.6f, 0.45f, 0.55f));
    
    add_pose(d, make_pose("bounce_twist_l", POSE_CAT_ENERGETIC,
        0.48f, 0.1f, -15, 8,
        -50, 40, 30, -25,
        -25, 35, 20, -28,
        0.55f, 0.8f, 0.65f, 0.35f));
    
    add_pose(d, make_pose("bounce_twist_r", POSE_CAT_ENERGETIC,
        0.52f, 0.1f, 15, -8,
        -30, 25, 50, -40,
        20, -28, -25, 35,
        0.55f, 0.8f, 0.35f, 0.65f));
    
    /* ========== ISOLATIONS ========== */
    add_pose(d, make_pose("iso_chest_l", POSE_CAT_GROOVE,
        0.47f, 0.09f, -10, 0,
        -25, 30, 20, -25,
        8, -10, -8, 10,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("iso_chest_r", POSE_CAT_GROOVE,
        0.53f, 0.09f, 10, 0,
        -20, 25, 25, -30,
        8, -10, -8, 10,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("iso_hip_l", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, -5,
        -30, 35, 30, -35,
        -20, 25, 15, -18,
        0.35f, 0.55f, 0.6f, 0.4f));
    
    add_pose(d, make_pose("iso_hip_r", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, 5,
        -30, 35, 30, -35,
        15, -18, -20, 25,
        0.35f, 0.55f, 0.4f, 0.6f));
    
    /* ========== FREESTYLE / WILD MOVES ========== */
    add_pose(d, make_pose("wild_flail1", POSE_CAT_INTENSE,
        0.5f, 0.07f, -12, -8,
        -120, 45, 80, -60,
        -28, 40, 35, -30,
        0.7f, 1.0f, 0.65f, 0.55f));
    
    add_pose(d, make_pose("wild_flail2", POSE_CAT_INTENSE,
        0.5f, 0.07f, 12, 8,
        -80, 60, 120, -45,
        35, -30, -28, 40,
        0.7f, 1.0f, 0.55f, 0.65f));
    
    add_pose(d, make_pose("wild_kick_l", POSE_CAT_INTENSE,
        0.55f, 0.08f, 10, 12,
        -60, 30, 45, -20,
        -60, 10, 30, -35,
        0.75f, 1.0f, 0.8f, 0.3f));
    
    add_pose(d, make_pose("wild_kick_r", POSE_CAT_INTENSE,
        0.45f, 0.08f, -10, -12,
        -45, 20, 60, -30,
        30, -35, -60, 10,
        0.75f, 1.0f, 0.3f, 0.8f));
    
    /* ========== CELEBRATION POSES ========== */
    add_pose(d, make_pose("celebrate_jump", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.04f, 0, -15,
        -130, -45, 130, 45,
        10, -15, -10, 15,
        0.55f, 0.85f, 0.4f, 0.85f));
    
    add_pose(d, make_pose("celebrate_wave", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.08f, -10, 5,
        -140, -30, 80, 20,
        15, -20, -15, 20,
        0.5f, 0.8f, 0.5f, 0.75f));
    
    add_pose(d, make_pose("celebrate_clap", POSE_CAT_TREBLE_ACCENT,
        0.5f, 0.09f, 0, 2,
        -60, -85, 60, 85,
        8, -10, -8, 10,
        0.45f, 0.7f, 0.5f, 0.5f));
    
    /* ========== FOOTWORK EMPHASIS ========== */
    add_pose(d, make_pose("step_cross_l", POSE_CAT_GROOVE,
        0.45f, 0.1f, 5, 3,
        -25, 30, 20, -25,
        20, -25, -5, 8,
        0.3f, 0.5f, 0.65f, 0.35f));
    
    add_pose(d, make_pose("step_cross_r", POSE_CAT_GROOVE,
        0.55f, 0.1f, -5, -3,
        -20, 25, 25, -30,
        -5, 8, 20, -25,
        0.3f, 0.5f, 0.35f, 0.65f));
    
    add_pose(d, make_pose("step_back_l", POSE_CAT_GROOVE,
        0.52f, 0.1f, 3, 5,
        -30, 35, 25, -30,
        5, -8, 25, -35,
        0.35f, 0.55f, 0.4f, 0.6f));
    
    add_pose(d, make_pose("step_back_r", POSE_CAT_GROOVE,
        0.48f, 0.1f, -3, -5,
        -25, 30, 30, -35,
        25, -35, 5, -8,
        0.35f, 0.55f, 0.6f, 0.4f));
    
    /* ========== SHOULDER MOVES ========== */
    add_pose(d, make_pose("shoulder_pop_l", POSE_CAT_GROOVE,
        0.5f, 0.09f, -5, 0,
        -35, 25, 15, -10,
        10, -12, -10, 12,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("shoulder_pop_r", POSE_CAT_GROOVE,
        0.5f, 0.09f, 5, 0,
        -15, 10, 35, -25,
        10, -12, -10, 12,
        0.3f, 0.5f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("shoulder_roll", POSE_CAT_GROOVE,
        0.5f, 0.1f, 0, 3,
        -40, 40, 40, -40,
        12, -15, -12, 15,
        0.35f, 0.55f, 0.5f, 0.5f));
    
    /* ========== ARM CHOREOGRAPHY ========== */
    add_pose(d, make_pose("arms_snake_l", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, -8, 5,
        -110, -40, 70, 30,
        15, -18, -12, 15,
        0.45f, 0.7f, 0.5f, 0.55f));
    
    add_pose(d, make_pose("arms_snake_r", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 8, -5,
        -70, -30, 110, 40,
        12, -15, -15, 18,
        0.45f, 0.7f, 0.55f, 0.5f));
    
    add_pose(d, make_pose("arms_circle_up", POSE_CAT_ENERGETIC,
        0.5f, 0.07f, 0, -5,
        -135, -20, 135, 20,
        8, -10, -8, 10,
        0.4f, 0.65f, 0.5f, 0.6f));
    
    add_pose(d, make_pose("arms_circle_down", POSE_CAT_ENERGETIC,
        0.5f, 0.11f, 0, 8,
        -45, 60, 45, -60,
        15, -20, -15, 20,
        0.5f, 0.75f, 0.55f, 0.45f));
    
    /* ========== BASS HIT REACTIONS ========== */
    add_pose(d, make_pose("bass_stomp_l", POSE_CAT_BASS_HIT,
        0.5f, 0.11f, 5, 12,
        -35, 40, 30, -35,
        -40, 60, 20, -25,
        0.55f, 0.85f, 0.75f, 0.35f));
    
    add_pose(d, make_pose("bass_stomp_r", POSE_CAT_BASS_HIT,
        0.5f, 0.11f, -5, -12,
        -30, 35, 35, -40,
        20, -25, -40, 60,
        0.55f, 0.85f, 0.35f, 0.75f));
    
    add_pose(d, make_pose("bass_crouch_deep", POSE_CAT_BASS_HIT,
        0.5f, 0.14f, 0, 15,
        -25, 40, 25, -40,
        25, -35, -25, 35,
        0.6f, 0.9f, 0.65f, 0.35f));
    
    add_pose(d, make_pose("bass_punch_low", POSE_CAT_BASS_HIT,
        0.5f, 0.1f, 0, 10,
        -50, -70, 50, 70,
        18, -22, -18, 22,
        0.55f, 0.85f, 0.5f, 0.55f));
    
    /* ========== RELAXED / SMOOTH MOVES ========== */
    add_pose(d, make_pose("smooth_sway_l", POSE_CAT_CALM,
        0.48f, 0.1f, -5, 3,
        -20, 20, 15, -15,
        -12, 15, 8, -10,
        0.2f, 0.35f, 0.55f, 0.45f));
    
    add_pose(d, make_pose("smooth_sway_r", POSE_CAT_CALM,
        0.52f, 0.1f, 5, -3,
        -15, 15, 20, -20,
        8, -10, -12, 15,
        0.2f, 0.35f, 0.45f, 0.55f));
    
    add_pose(d, make_pose("smooth_wave", POSE_CAT_CALM,
        0.5f, 0.1f, 0, 2,
        -30, 25, 30, -25,
        10, -12, -10, 12,
        0.2f, 0.35f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("smooth_groove", POSE_CAT_CALM,
        0.5f, 0.11f, 3, 5,
        -25, 30, 25, -30,
        15, -18, -15, 18,
        0.25f, 0.4f, 0.52f, 0.48f));
    
    /* ========== SHARP / PRECISE MOVES (POPPING STYLE) ========== */
    add_pose(d, make_pose("pop_hit1", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 0, -3,
        -60, 45, 60, -45,
        8, -10, -8, 10,
        0.4f, 0.65f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("pop_hit2", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, -8, 0,
        -90, 25, 40, -15,
        12, -15, -12, 15,
        0.42f, 0.68f, 0.52f, 0.48f));
    
    add_pose(d, make_pose("pop_hit3", POSE_CAT_ENERGETIC,
        0.5f, 0.08f, 8, 0,
        -40, 15, 90, -25,
        12, -15, -12, 15,
        0.42f, 0.68f, 0.48f, 0.52f));
    
    add_pose(d, make_pose("pop_freeze", POSE_CAT_ENERGETIC,
        0.5f, 0.09f, 0, 2,
        -75, 60, 75, -60,
        10, -12, -10, 12,
        0.38f, 0.62f, 0.5f, 0.5f));
    
    /* ========== FLUID / CONTINUOUS FLOW ========== */
    add_pose(d, make_pose("flow_a", POSE_CAT_GROOVE,
        0.5f, 0.09f, -5, 3,
        -55, 20, 40, -10,
        8, -10, -8, 10,
        0.3f, 0.52f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("flow_b", POSE_CAT_GROOVE,
        0.5f, 0.09f, 0, 0,
        -40, 30, 55, -25,
        10, -12, -10, 12,
        0.32f, 0.54f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("flow_c", POSE_CAT_GROOVE,
        0.5f, 0.09f, 5, -3,
        -35, 15, 65, -35,
        8, -10, -8, 10,
        0.3f, 0.52f, 0.5f, 0.5f));
    
    add_pose(d, make_pose("flow_d", POSE_CAT_GROOVE,
        0.5f, 0.09f, 8, -5,
        -25, 5, 80, -45,
        10, -12, -10, 12,
        0.32f, 0.54f, 0.48f, 0.52f));
    
    /* ========== POWER MOVES ========== */
    add_pose(d, make_pose("power_stance", POSE_CAT_INTENSE,
        0.5f, 0.1f, 0, 5,
        -80, 30, 80, -30,
        -25, 35, 25, -35,
        0.6f, 0.9f, 0.55f, 0.55f));
    
    add_pose(d, make_pose("power_reach_l", POSE_CAT_INTENSE,
        0.5f, 0.08f, -12, -8,
        -140, -50, 50, 20,
        -20, 28, 15, -18,
        0.65f, 0.95f, 0.6f, 0.5f));
    
    add_pose(d, make_pose("power_reach_r", POSE_CAT_INTENSE,
        0.5f, 0.08f, 12, 8,
        -50, -20, 140, 50,
        15, -18, -20, 28,
        0.65f, 0.95f, 0.5f, 0.6f));
    
    add_pose(d, make_pose("power_pump", POSE_CAT_INTENSE,
        0.5f, 0.08f, 0, -5,
        -100, -60, 100, 60,
        12, -15, -12, 15,
        0.6f, 0.9f, 0.5f, 0.55f));
    
    /* ========== v3.1: SPIN POSES ========== */
    /* These poses have facing directions for spins */
    {
        Pose spin;
        
        /* Spin wind-up (facing slightly left) */
        spin = make_pose("spin_windup", POSE_CAT_SPIN,
            0.5f, 0.09f, -20, 10,
            -45, 60, 80, -30,
            -15, 20, 20, -25,
            0.5f, 0.9f, 0.6f, 0.5f);
        spin.facing = -0.4f;  /* Slightly turned */
        add_pose(d, spin);
        
        /* Mid-spin (facing side) */
        spin = make_pose("spin_mid_l", POSE_CAT_SPIN,
            0.5f, 0.08f, 0, 5,
            -70, 30, 70, -30,
            -10, 15, 10, -15,
            0.55f, 0.95f, 0.55f, 0.55f);
        spin.facing = -1.57f;  /* 90 degrees left */
        add_pose(d, spin);
        
        spin = make_pose("spin_mid_r", POSE_CAT_SPIN,
            0.5f, 0.08f, 0, -5,
            -70, 30, 70, -30,
            10, -15, -10, 15,
            0.55f, 0.95f, 0.55f, 0.55f);
        spin.facing = 1.57f;  /* 90 degrees right */
        add_pose(d, spin);
        
        /* Back-facing spin moment */
        spin = make_pose("spin_back", POSE_CAT_SPIN,
            0.5f, 0.09f, 0, 0,
            -60, 40, 60, -40,
            5, -8, -5, 8,
            0.6f, 1.0f, 0.5f, 0.5f);
        spin.facing = 3.14f;  /* Facing away */
        add_pose(d, spin);
        
        /* Spin completion (arms out) */
        spin = make_pose("spin_finish", POSE_CAT_SPIN,
            0.5f, 0.07f, 15, -8,
            -90, 10, 90, -10,
            12, -15, -12, 15,
            0.55f, 0.9f, 0.55f, 0.6f);
        spin.facing = 0.3f;  /* Slight turn */
        add_pose(d, spin);
        
        /* Pirouette style */
        spin = make_pose("pirouette_up", POSE_CAT_SPIN,
            0.5f, 0.06f, 0, -10,
            -130, -30, 130, 30,
            -5, 8, 5, -8,
            0.6f, 1.0f, 0.4f, 0.7f);
        spin.facing = 0.0f;
        add_pose(d, spin);
        
        /* Breakdance spin prep */
        spin = make_pose("break_spin_low", POSE_CAT_SPIN,
            0.5f, 0.14f, 25, 20,
            -40, 70, 100, -45,
            -35, 50, 30, -40,
            0.65f, 1.0f, 0.7f, 0.4f);
        spin.facing = -0.8f;
        add_pose(d, spin);
    }
    
    /* ========== v3.1: DIP POSES ========== */
    /* Dramatic dips and drops */
    {
        Pose dip;
        
        /* Deep dip (dramatic low pose) */
        dip = make_pose("dip_deep", POSE_CAT_DIP,
            0.5f, 0.22f, 0, 30,
            -20, 50, 20, -50,
            -35, 55, 35, -55,
            0.5f, 1.0f, 0.75f, 0.3f);
        dip.dip_amount = 0.8f;
        add_pose(d, dip);
        
        /* Side dip left */
        dip = make_pose("dip_left", POSE_CAT_DIP,
            0.45f, 0.18f, -25, 20,
            -60, 40, 30, -20,
            -40, 55, 20, -30,
            0.45f, 0.9f, 0.6f, 0.4f);
        dip.dip_amount = 0.5f;
        dip.facing = -0.3f;
        add_pose(d, dip);
        
        /* Side dip right */
        dip = make_pose("dip_right", POSE_CAT_DIP,
            0.55f, 0.18f, 25, -20,
            -30, 20, 60, -40,
            20, -30, -40, 55,
            0.45f, 0.9f, 0.6f, 0.4f);
        dip.dip_amount = 0.5f;
        dip.facing = 0.3f;
        add_pose(d, dip);
        
        /* Drop it low */
        dip = make_pose("drop_low", POSE_CAT_DIP,
            0.5f, 0.25f, 0, 35,
            -50, 60, 50, -60,
            -45, 70, 45, -70,
            0.6f, 1.0f, 0.85f, 0.25f);
        dip.dip_amount = 1.0f;  /* Maximum dip */
        add_pose(d, dip);
        
        /* Dramatic lean back */
        dip = make_pose("lean_back_dip", POSE_CAT_DIP,
            0.5f, 0.08f, 0, -25,
            -100, -40, 100, 40,
            25, -35, -25, 35,
            0.5f, 0.9f, 0.5f, 0.6f);
        dip.dip_amount = 0.3f;
        add_pose(d, dip);
        
        /* Matrix dodge */
        dip = make_pose("matrix_lean", POSE_CAT_DIP,
            0.5f, 0.06f, 0, -35,
            -80, -20, 80, 20,
            20, -28, -20, 28,
            0.55f, 0.95f, 0.45f, 0.55f);
        dip.dip_amount = 0.4f;
        add_pose(d, dip);
        
        /* Bass drop pose */
        dip = make_pose("bass_drop", POSE_CAT_DIP,
            0.5f, 0.2f, 0, 25,
            -60, 50, 60, -50,
            -30, 45, 30, -45,
            0.7f, 1.0f, 0.9f, 0.2f);
        dip.dip_amount = 0.7f;
        add_pose(d, dip);
        
        /* Recovery from dip */
        dip = make_pose("dip_recover", POSE_CAT_DIP,
            0.5f, 0.12f, 0, 10,
            -40, 35, 40, -35,
            -20, 30, 20, -30,
            0.4f, 0.85f, 0.6f, 0.4f);
        dip.dip_amount = 0.3f;
        add_pose(d, dip);
    }
    
    /* ========== GENRE-SPECIFIC POSES ========== */
    /* Electronic/EDM - Arms up, symmetrical */
    add_pose(d, make_pose("edm_hands_up", POSE_CAT_ENERGETIC,
        0.5f, 0.07f, 0, -5,
        -140, -20, 140, 20,
        8, -10, -8, 10,
        0.5f, 0.9f, 0.4f, 0.8f));
    
    add_pose(d, make_pose("edm_pump", POSE_CAT_INTENSE,
        0.5f, 0.09f, 0, 3,
        -120, -50, 120, 50,
        12, -15, -12, 15,
        0.6f, 1.0f, 0.5f, 0.7f));
    
    /* Hip-hop - More asymmetrical, attitude */
    add_pose(d, make_pose("hiphop_lean", POSE_CAT_GROOVE,
        0.52f, 0.11f, 10, 8,
        -35, 45, 80, -35,
        -18, 25, 15, -20,
        0.35f, 0.65f, 0.7f, 0.35f));
    
    add_pose(d, make_pose("hiphop_bounce", POSE_CAT_ENERGETIC,
        0.5f, 0.12f, -5, 10,
        -45, 55, 45, -55,
        -22, 32, 22, -32,
        0.45f, 0.8f, 0.75f, 0.3f));
    
    /* Rock - Head bang, power stance */
    add_pose(d, make_pose("rock_headbang", POSE_CAT_INTENSE,
        0.5f, 0.14f, 0, 20,
        -30, 40, 30, -40,
        -15, 22, 15, -22,
        0.55f, 0.95f, 0.6f, 0.5f));
    
    add_pose(d, make_pose("rock_power", POSE_CAT_INTENSE,
        0.5f, 0.08f, 0, -8,
        -90, 30, 90, -30,
        -30, 42, 30, -42,
        0.6f, 1.0f, 0.55f, 0.55f));
    
    /* Jazz/Swing - Smooth, flowing */
    add_pose(d, make_pose("jazz_slide", POSE_CAT_GROOVE,
        0.48f, 0.1f, -8, 5,
        -55, 25, 40, -15,
        -25, 35, 10, -15,
        0.3f, 0.6f, 0.4f, 0.6f));
    
    add_pose(d, make_pose("jazz_snap", POSE_CAT_GROOVE,
        0.52f, 0.09f, 5, 3,
        -40, 65, 55, -20,
        12, -18, -10, 15,
        0.35f, 0.65f, 0.35f, 0.7f));
    
    /* Classical/Orchestral - Elegant, conductor-like */
    add_pose(d, make_pose("classical_conduct", POSE_CAT_CALM,
        0.5f, 0.08f, 0, -5,
        -70, 30, 70, -30,
        5, -8, -5, 8,
        0.2f, 0.5f, 0.3f, 0.7f));
    
    add_pose(d, make_pose("classical_sway", POSE_CAT_CALM,
        0.5f, 0.09f, 5, 3,
        -25, 20, 35, -25,
        8, -10, -8, 10,
        0.15f, 0.4f, 0.4f, 0.6f));
    
    /* ========== v3.2: MOONWALK POSES (Pop/Hip-hop easter egg) ========== */
    {
        Pose mw;
        /* Moonwalk slide back - one foot forward, weight back */
        mw = make_pose("moonwalk_slide1", POSE_CAT_MOONWALK,
            0.48f, 0.10f, -3, 5,
            -25, 30, 30, -25,
            15, 20, -15, -10,
            0.3f, 0.7f, 0.8f, 0.3f);
        add_pose(d, mw);
        
        mw = make_pose("moonwalk_slide2", POSE_CAT_MOONWALK,
            0.52f, 0.10f, 3, 5,
            -30, 25, 25, -30,
            -15, -10, 15, 20,
            0.8f, 0.3f, 0.3f, 0.7f);
        add_pose(d, mw);
        
        /* Moonwalk glide - smooth transition */
        mw = make_pose("moonwalk_glide", POSE_CAT_MOONWALK,
            0.5f, 0.10f, 0, 3,
            -20, 40, 20, -40,
            10, 15, -10, -15,
            0.5f, 0.6f, 0.6f, 0.5f);
        add_pose(d, mw);
        
        /* Moonwalk toe point */
        mw = make_pose("moonwalk_toe", POSE_CAT_MOONWALK,
            0.5f, 0.11f, 0, 8,
            -35, 35, 35, -35,
            8, 25, -8, -5,
            0.2f, 0.5f, 0.9f, 0.2f);
        add_pose(d, mw);
    }
    
    /* ========== BALLET/CLASSICAL POSES ========== */
    {
        Pose bl;
        /* First position - heels together, arms rounded low */
        bl = make_pose("ballet_first", POSE_CAT_BALLET,
            0.5f, 0.10f, 0, 0,
            -60, 70, 60, -70,
            10, -15, -10, 15,
            0.3f, 0.6f, 0.3f, 0.6f);
        add_pose(d, bl);
        
        /* Arabesque - one leg extended back, arms out */
        bl = make_pose("ballet_arabesque", POSE_CAT_BALLET,
            0.5f, 0.08f, 15, -10,
            -90, 10, 90, -10,
            5, -5, -80, 20,
            0.2f, 0.35f, 0.2f, 0.95f);
        add_pose(d, bl);
        
        /* Plié - bent knees, arms soft */
        bl = make_pose("ballet_plie", POSE_CAT_BALLET,
            0.5f, 0.15f, 0, 5,
            -50, 60, 50, -60,
            20, 50, -20, -50,
            0.4f, 0.85f, 0.4f, 0.85f);
        add_pose(d, bl);
        
        /* Port de bras - flowing arm movement */
        bl = make_pose("ballet_port_de_bras", POSE_CAT_BALLET,
            0.5f, 0.09f, 5, -8,
            -120, 30, 45, -50,
            8, -10, -8, 10,
            0.25f, 0.5f, 0.35f, 0.65f);
        add_pose(d, bl);
        
        /* Relevé - on toes */
        bl = make_pose("ballet_releve", POSE_CAT_BALLET,
            0.5f, 0.07f, 0, -15,
            -140, 20, 140, -20,
            5, -8, -5, 8,
            0.15f, 0.3f, 0.15f, 0.3f);
        add_pose(d, bl);
    }
    
    /* ========== BREAKDANCE POSES (Hip-hop easter egg) ========== */
    {
        Pose bd;
        /* Toprock stance */
        bd = make_pose("break_toprock", POSE_CAT_BREAKDANCE,
            0.5f, 0.11f, 0, 10,
            -45, 50, 60, -40,
            -25, 35, 25, -35,
            0.5f, 0.85f, 0.7f, 0.4f);
        add_pose(d, bd);
        
        /* Freeze - hand on ground, legs up */
        bd = make_pose("break_freeze", POSE_CAT_BREAKDANCE,
            0.55f, 0.2f, 20, 25,
            -120, 60, 30, -45,
            -70, 80, 45, -60,
            0.7f, 1.0f, 0.3f, 0.9f);
        bd.dip_amount = 0.6f;
        add_pose(d, bd);
        
        /* Indian step */
        bd = make_pose("break_indian", POSE_CAT_BREAKDANCE,
            0.5f, 0.13f, -8, 12,
            -60, 55, 70, -50,
            -30, 40, 35, -45,
            0.6f, 0.95f, 0.55f, 0.65f);
        add_pose(d, bd);
        
        /* Power move prep */
        bd = make_pose("break_power_prep", POSE_CAT_BREAKDANCE,
            0.5f, 0.16f, 0, 18,
            -80, 45, 80, -45,
            -45, 55, 45, -55,
            0.55f, 1.0f, 0.55f, 1.0f);
        add_pose(d, bd);
    }
    
    /* ========== WALTZ/BALLROOM POSES ========== */
    {
        Pose wz;
        /* Waltz frame - partner hold position */
        wz = make_pose("waltz_frame", POSE_CAT_WALTZ,
            0.5f, 0.09f, 0, -3,
            -80, 60, 45, -50,
            8, -10, -8, 10,
            0.25f, 0.5f, 0.35f, 0.6f);
        add_pose(d, wz);
        
        /* Waltz turn */
        wz = make_pose("waltz_turn", POSE_CAT_WALTZ,
            0.5f, 0.10f, 8, 5,
            -75, 55, 50, -55,
            15, -15, -10, 20,
            0.3f, 0.55f, 0.4f, 0.55f);
        add_pose(d, wz);
        
        /* Waltz rise */
        wz = make_pose("waltz_rise", POSE_CAT_WALTZ,
            0.5f, 0.07f, 0, -10,
            -70, 50, 55, -55,
            5, -8, -5, 8,
            0.2f, 0.4f, 0.25f, 0.45f);
        add_pose(d, wz);
        
        /* Waltz sway */
        wz = make_pose("waltz_sway", POSE_CAT_WALTZ,
            0.52f, 0.10f, 10, 3,
            -65, 45, 60, -50,
            12, -12, -8, 15,
            0.28f, 0.52f, 0.32f, 0.55f);
        add_pose(d, wz);
    }
    
    /* ========== ROBOT POSES (Electronic/techno easter egg) ========== */
    {
        Pose rb;
        /* Robot lock - stiff, angular */
        rb = make_pose("robot_lock", POSE_CAT_ROBOT,
            0.5f, 0.09f, 0, 0,
            -90, 90, 90, -90,
            0, 0, 0, 0,
            0.3f, 0.6f, 0.3f, 0.6f);
        add_pose(d, rb);
        
        /* Robot arm extend */
        rb = make_pose("robot_extend", POSE_CAT_ROBOT,
            0.5f, 0.09f, 0, 0,
            -90, 0, 0, 0,
            0, 0, 0, 0,
            0.35f, 0.65f, 0.35f, 0.65f);
        add_pose(d, rb);
        
        /* Robot tilt */
        rb = make_pose("robot_tilt", POSE_CAT_ROBOT,
            0.5f, 0.10f, -20, 0,
            -90, 90, 90, -90,
            5, -5, -5, 5,
            0.35f, 0.65f, 0.35f, 0.65f);
        add_pose(d, rb);
        
        /* Robot wave */
        rb = make_pose("robot_wave", POSE_CAT_ROBOT,
            0.5f, 0.09f, 0, 5,
            -120, -45, 45, -90,
            0, 5, 0, -5,
            0.3f, 0.6f, 0.3f, 0.6f);
        add_pose(d, rb);
        
        /* Robot isolate */
        rb = make_pose("robot_isolate", POSE_CAT_ROBOT,
            0.52f, 0.10f, 0, -5,
            -90, 45, 90, -45,
            8, -8, -8, 8,
            0.32f, 0.62f, 0.32f, 0.62f);
        add_pose(d, rb);
    }
    
    /* ========== HEADBANG POSES (Rock/metal easter egg) ========== */
    {
        Pose hb;
        /* Headbang down */
        hb = make_pose("headbang_down", POSE_CAT_HEADBANG,
            0.5f, 0.14f, 0, 35,
            -30, 40, 30, -40,
            -15, 25, 15, -25,
            0.5f, 0.9f, 0.5f, 0.9f);
        add_pose(d, hb);
        
        /* Headbang up */
        hb = make_pose("headbang_up", POSE_CAT_HEADBANG,
            0.5f, 0.08f, 0, -20,
            -35, 35, 35, -35,
            -12, 20, 12, -20,
            0.45f, 0.85f, 0.45f, 0.85f);
        add_pose(d, hb);
        
        /* Devil horns */
        hb = make_pose("headbang_horns", POSE_CAT_HEADBANG,
            0.5f, 0.09f, 0, 15,
            -120, -60, 120, 60,
            -10, 18, 10, -18,
            0.48f, 0.88f, 0.48f, 0.88f);
        add_pose(d, hb);
        
        /* Power stance headbang */
        hb = make_pose("headbang_power", POSE_CAT_HEADBANG,
            0.5f, 0.11f, 0, 25,
            -60, 45, 60, -45,
            -25, 40, 25, -40,
            0.55f, 1.0f, 0.55f, 1.0f);
        add_pose(d, hb);
    }
    
    /* ========== PROCEDURAL POSE VARIATIONS ========== */
    /* Generate variations of existing poses with subtle modifications */
    int base_poses = d->num_poses;
    generate_pose_variations(d);
    
    /* Log the number of poses for debugging */
    fprintf(stderr, "[skeleton_dancer] %d base poses -> %d total poses after variations\n", 
            base_poses, d->num_poses);
}

/* Generate procedural variations of base poses to reach 1000+ unique poses */
static void generate_pose_variations(SkeletonDancer *d) {
    int base_count = d->num_poses;
    
    /* First pass: Create mirrored versions of all poses */
    for (int i = 0; i < base_count && d->num_poses < MAX_POSES - 100; i++) {
        const Pose *base = &d->poses[i];
        
        /* Variation 1: Mirrored pose (swap left/right) */
        Pose mirror = *base;
        char name[32];
        snprintf(name, sizeof(name), "%s_mir", base->name);
        snprintf(mirror.name, sizeof(mirror.name), "%s", name);
        
        /* Swap arm joints */
        Joint temp;
        temp = mirror.joints[JOINT_SHOULDER_L];
        mirror.joints[JOINT_SHOULDER_L] = mirror.joints[JOINT_SHOULDER_R];
        mirror.joints[JOINT_SHOULDER_R] = temp;
        mirror.joints[JOINT_SHOULDER_L].x = 1.0f - mirror.joints[JOINT_SHOULDER_L].x;
        mirror.joints[JOINT_SHOULDER_R].x = 1.0f - mirror.joints[JOINT_SHOULDER_R].x;
        
        temp = mirror.joints[JOINT_ELBOW_L];
        mirror.joints[JOINT_ELBOW_L] = mirror.joints[JOINT_ELBOW_R];
        mirror.joints[JOINT_ELBOW_R] = temp;
        mirror.joints[JOINT_ELBOW_L].x = 1.0f - mirror.joints[JOINT_ELBOW_L].x;
        mirror.joints[JOINT_ELBOW_R].x = 1.0f - mirror.joints[JOINT_ELBOW_R].x;
        
        temp = mirror.joints[JOINT_HAND_L];
        mirror.joints[JOINT_HAND_L] = mirror.joints[JOINT_HAND_R];
        mirror.joints[JOINT_HAND_R] = temp;
        mirror.joints[JOINT_HAND_L].x = 1.0f - mirror.joints[JOINT_HAND_L].x;
        mirror.joints[JOINT_HAND_R].x = 1.0f - mirror.joints[JOINT_HAND_R].x;
        
        /* Swap leg joints */
        temp = mirror.joints[JOINT_HIP_L];
        mirror.joints[JOINT_HIP_L] = mirror.joints[JOINT_HIP_R];
        mirror.joints[JOINT_HIP_R] = temp;
        mirror.joints[JOINT_HIP_L].x = 1.0f - mirror.joints[JOINT_HIP_L].x;
        mirror.joints[JOINT_HIP_R].x = 1.0f - mirror.joints[JOINT_HIP_R].x;
        
        temp = mirror.joints[JOINT_KNEE_L];
        mirror.joints[JOINT_KNEE_L] = mirror.joints[JOINT_KNEE_R];
        mirror.joints[JOINT_KNEE_R] = temp;
        mirror.joints[JOINT_KNEE_L].x = 1.0f - mirror.joints[JOINT_KNEE_L].x;
        mirror.joints[JOINT_KNEE_R].x = 1.0f - mirror.joints[JOINT_KNEE_R].x;
        
        temp = mirror.joints[JOINT_FOOT_L];
        mirror.joints[JOINT_FOOT_L] = mirror.joints[JOINT_FOOT_R];
        mirror.joints[JOINT_FOOT_R] = temp;
        mirror.joints[JOINT_FOOT_L].x = 1.0f - mirror.joints[JOINT_FOOT_L].x;
        mirror.joints[JOINT_FOOT_R].x = 1.0f - mirror.joints[JOINT_FOOT_R].x;
        
        /* Mirror center joints */
        mirror.joints[JOINT_HEAD].x = 1.0f - mirror.joints[JOINT_HEAD].x;
        mirror.joints[JOINT_NECK].x = 1.0f - mirror.joints[JOINT_NECK].x;
        mirror.joints[JOINT_HIP_CENTER].x = 1.0f - mirror.joints[JOINT_HIP_CENTER].x;
        
        add_pose(d, mirror);
    }
    
    /* Second pass: Create geometric variations for groove+ poses */
    int after_mirrors = d->num_poses;
    for (int i = 0; i < after_mirrors && d->num_poses < MAX_POSES - 50; i++) {
        const Pose *base = &d->poses[i];
        if (base->category < POSE_CAT_GROOVE) continue;
        
        /* Variation: Arms higher */
        Pose arms_up = *base;
        snprintf(arms_up.name, sizeof(arms_up.name), "%s_hi", base->name);
        arms_up.joints[JOINT_ELBOW_L].y -= 0.04f;
        arms_up.joints[JOINT_ELBOW_R].y -= 0.04f;
        arms_up.joints[JOINT_HAND_L].y -= 0.06f;
        arms_up.joints[JOINT_HAND_R].y -= 0.06f;
        add_pose(d, arms_up);
    }
    
    /* Third pass: Create stance variations */
    int after_arms = d->num_poses;
    for (int i = 0; i < after_arms && d->num_poses < MAX_POSES - 50; i++) {
        const Pose *base = &d->poses[i];
        if (base->category < POSE_CAT_GROOVE) continue;
        
        /* Skip some to stay within limits */
        if (i % 3 != 0) continue;
        
        /* Variation: Wider stance */
        Pose wide = *base;
        snprintf(wide.name, sizeof(wide.name), "%s_w", base->name);
        wide.joints[JOINT_FOOT_L].x -= 0.03f;
        wide.joints[JOINT_FOOT_R].x += 0.03f;
        wide.joints[JOINT_KNEE_L].x -= 0.02f;
        wide.joints[JOINT_KNEE_R].x += 0.02f;
        add_pose(d, wide);
    }
    
    /* Fourth pass: Create crouch variations for energetic+ poses */
    int after_wide = d->num_poses;
    for (int i = 0; i < after_wide && d->num_poses < MAX_POSES - 30; i++) {
        const Pose *base = &d->poses[i];
        if (base->category < POSE_CAT_ENERGETIC) continue;
        if (i % 4 != 0) continue;  /* Every 4th pose */
        
        Pose crouch = *base;
        snprintf(crouch.name, sizeof(crouch.name), "%s_cr", base->name);
        /* Lower entire body */
        for (int j = 0; j < MAX_JOINTS; j++) {
            crouch.joints[j].y += 0.02f;  /* Move down */
        }
        /* Bend knees more */
        crouch.joints[JOINT_KNEE_L].y += 0.03f;
        crouch.joints[JOINT_KNEE_R].y += 0.03f;
        crouch.joints[JOINT_KNEE_L].x -= 0.02f;
        crouch.joints[JOINT_KNEE_R].x += 0.02f;
        add_pose(d, crouch);
    }
    
    /* Fifth pass: Create lean variations */
    int after_crouch = d->num_poses;
    for (int i = 0; i < after_crouch && d->num_poses < MAX_POSES - 20; i++) {
        const Pose *base = &d->poses[i];
        if (base->category < POSE_CAT_GROOVE) continue;
        if (i % 5 != 0) continue;  /* Every 5th pose */
        
        /* Lean left */
        Pose lean_l = *base;
        snprintf(lean_l.name, sizeof(lean_l.name), "%s_ll", base->name);
        lean_l.joints[JOINT_HEAD].x -= 0.02f;
        lean_l.joints[JOINT_NECK].x -= 0.015f;
        lean_l.joints[JOINT_SHOULDER_L].x -= 0.01f;
        lean_l.joints[JOINT_SHOULDER_R].x -= 0.01f;
        add_pose(d, lean_l);
        
        if (d->num_poses >= MAX_POSES) break;
        
        /* Lean right */
        Pose lean_r = *base;
        snprintf(lean_r.name, sizeof(lean_r.name), "%s_lr", base->name);
        lean_r.joints[JOINT_HEAD].x += 0.02f;
        lean_r.joints[JOINT_NECK].x += 0.015f;
        lean_r.joints[JOINT_SHOULDER_L].x += 0.01f;
        lean_r.joints[JOINT_SHOULDER_R].x += 0.01f;
        add_pose(d, lean_r);
    }
    
    /* Sixth pass: Arms forward/back variations for intense poses */
    int after_lean = d->num_poses;
    for (int i = 0; i < after_lean && d->num_poses < MAX_POSES - 10; i++) {
        const Pose *base = &d->poses[i];
        if (base->category < POSE_CAT_INTENSE) continue;
        if (i % 6 != 0) continue;
        
        Pose punch = *base;
        snprintf(punch.name, sizeof(punch.name), "%s_pn", base->name);
        /* Extend one arm forward */
        punch.joints[JOINT_HAND_L].y -= 0.03f;
        punch.joints[JOINT_ELBOW_L].y -= 0.02f;
        add_pose(d, punch);
    }
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
    
    /* Faster smoothing when energy changes significantly (quick reaction) */
    float energy_diff = fabsf(a->energy - a->energy_smooth);
    float smooth_factor = (energy_diff > 0.2f) ? 0.6f : 0.75f;  /* Fast attack */
    /* Even faster when music starts (energy jumps from near-zero) */
    if (a->energy_smooth < 0.05f && a->energy > 0.1f) smooth_factor = 0.4f;
    /* Fast decay when music stops */
    if (a->energy < 0.02f && a->energy_smooth > 0.1f) smooth_factor = 0.5f;
    
    a->energy_smooth = a->energy_smooth * smooth_factor + a->energy * (1.0f - smooth_factor);
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
    
    /* Style detection - improved with more genres */
    if (a->bass_ratio > 0.5f && a->dynamics < 0.15f) {
        /* Heavy, repetitive bass = Electronic */
        a->detected_style = STYLE_ELECTRONIC;
    } else if (a->bass_ratio > 0.45f && a->dynamics > 0.15f && a->dynamics < 0.25f) {
        /* Punchy bass with moderate dynamics = Hip-hop */
        a->detected_style = STYLE_HIPHOP;
    } else if (a->energy_long < 0.15f) {
        /* Very low energy = Ambient */
        a->detected_style = STYLE_AMBIENT;
    } else if (a->dynamics > 0.35f && a->treble_ratio > 0.25f) {
        /* High dynamics with treble = Classical */
        a->detected_style = STYLE_CLASSICAL;
    } else if (a->bass_ratio > 0.35f && a->treble_ratio > 0.2f && a->dynamics < 0.3f) {
        /* Balanced with moderate dynamics = Pop */
        a->detected_style = STYLE_POP;
    } else if (a->energy_long > 0.4f && a->dynamics > 0.2f) {
        /* Driving energy = Rock */
        a->detected_style = STYLE_ROCK;
    } else {
        /* Default to Rock for balanced music */
        a->detected_style = STYLE_ROCK;
    }
}

/* ============ Pose Selection ============ */

static bool pose_in_history(const SkeletonDancer *d, int pose_idx) {
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
    
    /* v3.1: Use effective energy (with override) instead of raw audio */
    float energy = skeleton_dancer_get_effective_energy(d);
    
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
    
    /* ========== v3.2: Genre-specific Easter Eggs ========== */
    /* These trigger ~15% of the time when the genre is detected */
    float easter_egg_chance = 0.15f;
    
    switch (a->detected_style) {
        case STYLE_ELECTRONIC:
            /* Electronic/EDM -> Robot moves */
            if (random_float(d) < easter_egg_chance && d->category_counts[POSE_CAT_ROBOT] > 0) {
                return select_pose_from_category(d, POSE_CAT_ROBOT);
            }
            break;
            
        case STYLE_HIPHOP:
            /* Hip-hop -> Moonwalk or Breakdance */
            if (random_float(d) < easter_egg_chance) {
                if (random_float(d) < 0.5f && d->category_counts[POSE_CAT_MOONWALK] > 0) {
                    return select_pose_from_category(d, POSE_CAT_MOONWALK);
                } else if (d->category_counts[POSE_CAT_BREAKDANCE] > 0) {
                    return select_pose_from_category(d, POSE_CAT_BREAKDANCE);
                }
            }
            break;
            
        case STYLE_CLASSICAL:
            /* Classical -> Ballet or Waltz */
            if (random_float(d) < easter_egg_chance) {
                if (random_float(d) < 0.6f && d->category_counts[POSE_CAT_BALLET] > 0) {
                    return select_pose_from_category(d, POSE_CAT_BALLET);
                } else if (d->category_counts[POSE_CAT_WALTZ] > 0) {
                    return select_pose_from_category(d, POSE_CAT_WALTZ);
                }
            }
            break;
            
        case STYLE_ROCK:
            /* Rock/Metal -> Headbang */
            if (random_float(d) < easter_egg_chance && d->category_counts[POSE_CAT_HEADBANG] > 0) {
                return select_pose_from_category(d, POSE_CAT_HEADBANG);
            }
            break;
            
        case STYLE_POP:
            /* Pop -> Moonwalk */
            if (random_float(d) < easter_egg_chance && d->category_counts[POSE_CAT_MOONWALK] > 0) {
                return select_pose_from_category(d, POSE_CAT_MOONWALK);
            }
            break;
            
        default:
            break;
    }
    
    /* Occasional surprise move - jump up or down a category for variety */
    if (random_float(d) < 0.15f && energy > 0.2f) {
        int shift = (random_float(d) < 0.5f) ? -1 : 1;
        int new_cat = primary_cat + shift;
        if (new_cat >= POSE_CAT_IDLE && new_cat <= POSE_CAT_INTENSE) {
            primary_cat = new_cat;
        }
    }
    
    /* Select from primary category */
    int pose_idx = select_pose_from_category(d, primary_cat);
    
    /* Verify energy range */
    const Pose *p = &d->poses[pose_idx];
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
    
    /* v3.1: Apply energy override system */
    float effective_energy = skeleton_dancer_get_effective_energy(d);
    
    /* Decay energy boost over time */
    if (d->energy_boost > 0.01f) {
        d->energy_boost -= dt * d->energy_boost_decay;
        if (d->energy_boost < 0) d->energy_boost = 0;
    } else if (d->energy_boost < -0.01f) {
        d->energy_boost += dt * d->energy_boost_decay;
        if (d->energy_boost > 0) d->energy_boost = 0;
    }
    
    /* SILENCE DETECTION: Use effective energy, not raw audio */
    float silence_threshold = 0.02f;
    bool is_silent = (effective_energy < silence_threshold) && !d->energy_locked;
    
    /* Determine animation tempo based on energy and detected BPM */
    float base_tempo = 0.3f;
    if (a->beat.bpm_estimate > 60.0f && a->beat.bpm_estimate < 200.0f) {
        base_tempo = a->beat.bpm_estimate / 120.0f;  /* Normalize around 120 BPM */
    }
    /* Tempo scales with effective energy */
    float energy_factor = is_silent ? 0.0f : (0.5f + effective_energy);
    d->tempo = base_tempo * energy_factor;
    
    /* v3.1: Update spin/facing system */
    /* Apply spin momentum to facing - directly modify facing for immediate effect */
    if (fabsf(d->spin_momentum) > 0.05f) {
        /* Apply spin directly to facing for responsive feel */
        d->facing += d->spin_momentum * dt * 4.0f;  /* Faster spin speed */
        d->facing_target = d->facing;  /* Keep target in sync during spin */
        
        /* Decay momentum more slowly for longer spins */
        d->spin_momentum *= (1.0f - dt * 1.5f);
        if (fabsf(d->spin_momentum) < 0.05f) d->spin_momentum = 0;
    } else {
        /* Get target facing from current pose */
        const Pose *target_pose = &d->poses[d->pose_secondary];
        float pose_facing = target_pose->facing;
        
        /* When not spinning, smoothly return toward pose facing */
        float facing_diff = pose_facing - d->facing;
        /* Normalize to -PI to PI range */
        while (facing_diff > 3.14159f) facing_diff -= 6.28318f;
        while (facing_diff < -3.14159f) facing_diff += 6.28318f;
        d->facing_target = d->facing + facing_diff;
    }
    
    /* Smooth facing interpolation (only when not actively spinning) */
    if (fabsf(d->spin_momentum) < 0.5f) {
        float facing_speed = 3.0f + effective_energy * 3.0f;  /* Slower return */
        d->facing += (d->facing_target - d->facing) * dt * facing_speed;
    }
    
    /* Keep facing in reasonable range */
    while (d->facing > 6.28f) { d->facing -= 6.28f; d->facing_target -= 6.28f; }
    while (d->facing < -6.28f) { d->facing += 6.28f; d->facing_target += 6.28f; }
    
    /* v3.1: Update dip system */
    const Pose *current_pose = &d->poses[d->pose_secondary];
    d->dip_target = current_pose->dip_amount;
    /* Strong bass can trigger extra dip */
    if (a->bass > 0.8f && a->bass_velocity > 4.0f) {
        d->dip_target += 0.3f * a->bass;
    }
    if (d->dip_target > 1.0f) d->dip_target = 1.0f;
    
    /* Smooth dip interpolation */
    float dip_speed = 6.0f + effective_energy * 8.0f;
    d->dip += (d->dip_target - d->dip) * dt * dip_speed;
    
    /* Calculate pose duration based on tempo - avoid division by zero */
    /* Faster transitions = more dynamic dancing */
    float min_duration = (d->tempo > 0.01f) ? (0.15f / d->tempo) : 10.0f;
    float max_duration = (d->tempo > 0.01f) ? (0.8f / d->tempo) : 30.0f;
    d->pose_duration = min_duration + (1.0f - effective_energy) * (max_duration - min_duration);
    
    /* Check if we should transition to a new pose - only if not silent */
    bool should_transition = false;
    
    if (!is_silent && d->time_in_pose > d->pose_duration) {
        should_transition = true;
    }
    
    /* Beat can trigger early transition - more responsive to music */
    if (!is_silent && a->beat.beat_detected && d->time_in_pose > 0.12f) {
        /* Higher chance at higher energy */
        float transition_chance = 0.3f + effective_energy * 0.4f;
        if (random_float(d) < transition_chance) {
            should_transition = true;
        }
    }
    
    /* Strong bass hits always trigger movement */
    if (!is_silent && a->bass > 0.7f && a->bass_velocity > 3.0f && d->time_in_pose > 0.1f) {
        should_transition = true;
    }
    
    /* v3.1: High energy can trigger spin or dip poses */
    if (!is_silent && effective_energy > 0.8f && d->time_in_pose > 0.3f) {
        if (random_float(d) < 0.1f) {
            /* Chance to select spin pose */
            if (d->category_counts[POSE_CAT_SPIN] > 0) {
                d->time_in_pose = 0;
                d->pose_primary = d->pose_secondary;
                d->pose_secondary = select_pose_from_category(d, POSE_CAT_SPIN);
                add_to_history(d, d->pose_secondary);
                d->blend = 0;
                d->spin_momentum += (random_float(d) < 0.5f ? 1 : -1) * 3.14f;
                should_transition = false;  /* Already transitioned */
            }
        } else if (random_float(d) < 0.08f && a->bass > 0.75f) {
            /* Chance to select dip pose on bass hits */
            if (d->category_counts[POSE_CAT_DIP] > 0) {
                d->time_in_pose = 0;
                d->pose_primary = d->pose_secondary;
                d->pose_secondary = select_pose_from_category(d, POSE_CAT_DIP);
                add_to_history(d, d->pose_secondary);
                d->blend = 0;
                should_transition = false;
            }
        }
    }
    
    if (should_transition) {
        d->time_in_pose = 0;
        d->pose_primary = d->pose_secondary;
        d->pose_secondary = select_best_pose(d);
        add_to_history(d, d->pose_secondary);
        d->blend = 0;
    }
    
    /* Update blend - faster transitions feel more responsive */
    float blend_speed = 5.0f + effective_energy * 10.0f;  /* Range: 5-15 */
    d->blend += dt * blend_speed;
    if (d->blend > 1.0f) d->blend = 1.0f;
    
    /* Calculate modifiers based on frequency bands - scale by energy when silent */
    float mod_scale = is_silent ? 0.0f : 1.0f;
    
    /* Head bob - follows mid frequencies */
    float target_bob = sinf(d->time_total * 4.0f * d->tempo) * 0.02f * a->mid_smooth * mod_scale;
    d->head_bob = d->head_bob * 0.9f + target_bob * 0.1f;
    
    /* Arm swing - treble makes arms more active */
    float arm_phase = d->time_total * 3.0f * d->tempo;
    d->arm_swing_l = sinf(arm_phase) * 0.03f * a->treble_smooth * mod_scale;
    d->arm_swing_r = sinf(arm_phase + M_PI) * 0.03f * a->treble_smooth * mod_scale;
    
    /* Hip sway - bass drives hip movement */
    float hip_phase = d->time_total * 2.0f * d->tempo;
    d->hip_sway = sinf(hip_phase) * 0.02f * a->bass_smooth * mod_scale;
    
    /* Bounce - on beats */
    if (!is_silent && a->beat.beat_detected) {
        d->bounce = 0.03f * effective_energy;
    }
    d->bounce *= 0.85f;  /* Decay */
    
    /* Lean - follows spectral centroid */
    d->lean = (a->spectral_centroid - 0.5f) * 0.03f * mod_scale;
    
    /* Shoulder shimmy - treble reactive (v2.4) */
    float shimmy_phase = d->time_total * 6.0f * d->tempo;
    d->shoulder_shimmy = sinf(shimmy_phase) * 0.015f * a->treble_smooth * mod_scale;
    
    /* Knee pump - bass reactive (v2.4) */
    d->knee_pump = a->bass_smooth * 0.02f * mod_scale;
    
    /* Twist - mid frequencies (v2.4) */
    float twist_phase = d->time_total * 1.5f * d->tempo;
    d->twist = sinf(twist_phase) * 0.02f * a->mid_smooth * mod_scale;
    
    /* === PROCEDURAL MICRO-VARIATIONS === */
    /* These add subtle random variations to make every moment unique */
    /* Increased values for more visible animation variety */
    float micro_var_x = sinf(d->time_total * 7.3f) * 0.012f * mod_scale;
    float micro_var_y = sinf(d->time_total * 5.7f) * 0.008f * mod_scale;
    float micro_arm_l = sinf(d->time_total * 4.1f + 1.0f) * 0.015f * effective_energy * mod_scale;
    float micro_arm_r = sinf(d->time_total * 4.3f + 2.0f) * 0.015f * effective_energy * mod_scale;
    float micro_leg_l = sinf(d->time_total * 3.2f + 0.5f) * 0.012f * a->bass_smooth * mod_scale;
    float micro_leg_r = sinf(d->time_total * 3.4f + 1.5f) * 0.012f * a->bass_smooth * mod_scale;
    
    /* Additional micro-variations for more organic movement */
    float micro_shoulder_l = sinf(d->time_total * 6.1f) * 0.01f * a->treble_smooth * mod_scale;
    float micro_shoulder_r = sinf(d->time_total * 6.3f + 0.7f) * 0.01f * a->treble_smooth * mod_scale;
    float micro_bounce = sinf(d->time_total * 8.0f) * 0.005f * a->bass_smooth * mod_scale;
    
    /* Interpolate base pose */
    const Pose *p1 = &d->poses[d->pose_primary];
    const Pose *p2 = &d->poses[d->pose_secondary];
    float eased_blend = ease_in_out_cubic(d->blend);
    
    for (int i = 0; i < JOINT_COUNT; i++) {
        /* Base interpolation */
        Joint target = joint_lerp(p1->joints[i], p2->joints[i], eased_blend);
        
        /* Apply core modifiers + micro-variations for organic movement */
        if (i == JOINT_HEAD) {
            target.y += d->head_bob - d->bounce + micro_bounce;
            target.x += micro_var_x * 0.7f;  /* Head follows body sway */
        } else if (i == JOINT_NECK) {
            target.x += micro_var_x * 0.5f;
            target.y += micro_bounce * 0.5f;
        } else if (i == JOINT_SHOULDER_L) {
            target.y += d->shoulder_shimmy + micro_shoulder_l;
            target.x += micro_var_x * 0.8f;
        } else if (i == JOINT_SHOULDER_R) {
            target.y -= d->shoulder_shimmy + micro_shoulder_r;
            target.x += micro_var_x * 0.8f;
        } else if (i == JOINT_HAND_L) {
            target.x += d->arm_swing_l + micro_arm_l;
            target.y += micro_var_y + micro_shoulder_l * 0.5f;
        } else if (i == JOINT_ELBOW_L) {
            target.x += d->arm_swing_l * 0.5f + micro_arm_l * 0.6f;
            target.y += micro_shoulder_l * 0.3f;
        } else if (i == JOINT_HAND_R) {
            target.x += d->arm_swing_r + micro_arm_r;
            target.y += micro_var_y + micro_shoulder_r * 0.5f;
        } else if (i == JOINT_ELBOW_R) {
            target.x += d->arm_swing_r * 0.5f + micro_arm_r * 0.6f;
            target.y += micro_shoulder_r * 0.3f;
        } else if (i == JOINT_HIP_CENTER) {
            target.x += d->hip_sway + d->twist;
            target.y -= d->knee_pump * 0.5f;
        } else if (i == JOINT_HIP_L) {
            target.x += d->hip_sway + micro_leg_l * 0.3f;
            target.y -= d->knee_pump * 0.3f;
        } else if (i == JOINT_HIP_R) {
            target.x += d->hip_sway + micro_leg_r * 0.3f;
            target.y -= d->knee_pump * 0.3f;
        } else if (i == JOINT_KNEE_L) {
            target.y -= d->knee_pump;
            target.x += micro_leg_l;
        } else if (i == JOINT_KNEE_R) {
            target.y -= d->knee_pump;
            target.x += micro_leg_r;
        } else if (i == JOINT_FOOT_L) {
            target.x += micro_leg_l * 0.5f;
            target.y += micro_bounce * 0.3f;
        } else if (i == JOINT_FOOT_R) {
            target.x += micro_leg_r * 0.5f;
            target.y += micro_bounce * 0.3f;
        }
        
        /* Global bounce and lean */
        target.y -= d->bounce * 0.5f;
        target.x += d->lean;
        
        /* Update physics */
        d->physics[i].target = target;
        
        /* Adjust physics parameters based on joint and energy */
        float stiffness = 15.0f + effective_energy * 10.0f;
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
    
    /* Knee constraint (v2.4) - prevent knock-kneed look */
    {
        float cx = d->current[JOINT_HIP_CENTER].x;
        float knee_offset = 0.04f;
        
        /* Use internal phase for stance detection */
        bool left_planted = (d->phase < 0.5f || d->phase > 1.5f);
        
        float left_limit = cx - (left_planted ? knee_offset : 0.01f);
        if (d->current[JOINT_KNEE_L].x > left_limit) {
            d->current[JOINT_KNEE_L].x = left_limit;
            d->physics[JOINT_KNEE_L].position.x = left_limit;
        }
        
        float right_limit = cx + (!left_planted ? knee_offset : 0.01f);
        if (d->current[JOINT_KNEE_R].x < right_limit) {
            d->current[JOINT_KNEE_R].x = right_limit;
            d->physics[JOINT_KNEE_R].position.x = right_limit;
        }
    }
    
    /* Advance phase */
    d->phase += dt * d->tempo;
}

/* ============ Rendering ============ */

static void joint_to_pixel(const SkeletonDancer *d, Joint j, int *px, int *py) {
    /* v3.1: Apply facing direction (affects x scale) and dip (affects y offset) */
    float facing_scale = cosf(d->facing);  /* 1.0 when facing forward, 0 when sideways, -1 when back */
    float dip_offset = d->dip * 0.15f;     /* Dip lowers the whole body */
    
    /* Apply facing: when turning, x coordinates compress toward center */
    float centered_x = j.x - 0.5f;  /* Center around 0 */
    float scaled_x = centered_x * fabsf(facing_scale);  /* Compress when turning */
    
    /* Flip x when facing backward (creates rotation illusion) */
    if (facing_scale < 0) {
        scaled_x = -scaled_x;
    }
    
    *px = (int)(scaled_x * d->scale + d->offset_x);
    *py = (int)((j.y + dip_offset) * d->scale + d->offset_y);
}

void skeleton_dancer_render(SkeletonDancer *d, BrailleCanvas *canvas) {
    if (!d || !canvas) return;
    
    /* NOTE: Canvas should be cleared by caller before this function */
    
    /* Draw bones */
    for (int i = 0; i < d->skeleton.num_bones; i++) {
        const Bone *bone = &d->skeleton.bones[i];
        
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
    
    /* Draw torso shape - filled triangle between shoulders and hip */
    int sh_l_x, sh_l_y, sh_r_x, sh_r_y, hip_x, hip_y;
    joint_to_pixel(d, d->current[JOINT_SHOULDER_L], &sh_l_x, &sh_l_y);
    joint_to_pixel(d, d->current[JOINT_SHOULDER_R], &sh_r_x, &sh_r_y);
    joint_to_pixel(d, d->current[JOINT_HIP_CENTER], &hip_x, &hip_y);
    
    /* Draw torso outline */
    braille_draw_thick_line(canvas, sh_l_x, sh_l_y, sh_r_x, sh_r_y, 2);
    braille_draw_line(canvas, sh_l_x, sh_l_y, hip_x - 3, hip_y);
    braille_draw_line(canvas, sh_r_x, sh_r_y, hip_x + 3, hip_y);
    braille_draw_line(canvas, hip_x - 3, hip_y, hip_x + 3, hip_y);
    
    /* Draw hands - slightly larger */
    int hx, hy;
    joint_to_pixel(d, d->current[JOINT_HAND_L], &hx, &hy);
    braille_fill_circle(canvas, hx, hy, 3);
    joint_to_pixel(d, d->current[JOINT_HAND_R], &hx, &hy);
    braille_fill_circle(canvas, hx, hy, 3);
    
    /* Draw feet */
    int fx, fy;
    joint_to_pixel(d, d->current[JOINT_FOOT_L], &fx, &fy);
    braille_draw_ellipse(canvas, fx, fy + 1, 4, 2);  /* Horizontal ellipse for foot */
    braille_fill_circle(canvas, fx, fy + 1, 2);      /* Fill center */
    joint_to_pixel(d, d->current[JOINT_FOOT_R], &fx, &fy);
    braille_draw_ellipse(canvas, fx, fy + 1, 4, 2);
    braille_fill_circle(canvas, fx, fy + 1, 2);
    
    braille_canvas_render(canvas);
}

/* Get current joint positions for effects/shadows */
const Joint* skeleton_dancer_get_joints(SkeletonDancer *d) {
    return d ? d->current : NULL;
}

/* ============ Creation/Destruction ============ */

SkeletonDancer* skeleton_dancer_create(int canvas_cell_width, int canvas_cell_height) {
    SkeletonDancer *d = calloc(1, sizeof(SkeletonDancer));
    if (!d) return NULL;
    
    d->canvas_width = canvas_cell_width * BRAILLE_CELL_W;
    d->canvas_height = canvas_cell_height * BRAILLE_CELL_H;
    
    /* Scale to fit with more headroom at top */
    float scale_x = d->canvas_width * 0.75f;
    float scale_y = d->canvas_height * 0.70f;  /* Smaller to leave room */
    d->scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    d->offset_x = d->canvas_width / 2.0f;
    d->offset_y = d->canvas_height * 0.18f;  /* More offset from top for head room */
    
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
    
    /* v3.1: Initialize facing and energy override */
    d->facing = 0.0f;
    d->facing_target = 0.0f;
    d->facing_velocity = 0.0f;
    d->spin_momentum = 0.0f;
    d->dip = 0.0f;
    d->dip_target = 0.0f;
    d->energy_override = 0.0f;
    d->energy_boost = 0.0f;
    d->energy_boost_decay = 2.0f;  /* Decays over 0.5 seconds */
    d->energy_locked = false;
    
    return d;
}

void skeleton_dancer_destroy(SkeletonDancer *d) {
    free(d);
}

/* ============ Rhythm-Aware Update (v2.3) ============ */

void skeleton_dancer_update_with_phase(SkeletonDancer *d, 
                                       float bass, float mid, float treble,
                                       float dt, float beat_phase, float bpm) {
    if (!d) return;
    
    d->time_total += dt;
    d->time_in_pose += dt;
    
    /* Analyze audio */
    analyze_audio(&d->audio, bass, mid, treble, dt);
    AudioAnalysis *a = &d->audio;
    
    /* v3.1: Calculate effective energy with user override */
    float effective_energy = skeleton_dancer_get_effective_energy(d);
    
    /* SILENCE DETECTION: Use effective energy for silence check */
    float silence_threshold = 0.02f;
    bool is_silent = (effective_energy < silence_threshold);
    
    /* Use provided BPM instead of estimated */
    if (bpm > 60.0f && bpm < 200.0f) {
        a->beat.bpm_estimate = bpm;
    }
    
    /* Animation tempo locked to BPM, but reduced/stopped when silent */
    float base_tempo = bpm / 120.0f;  /* Normalize around 120 BPM */
    float energy_factor = is_silent ? 0.0f : (0.5f + effective_energy * 0.5f);
    d->tempo = base_tempo * energy_factor;
    
    /* v3.1: Update spin/facing system */
    /* Apply spin momentum to facing - directly modify facing for immediate effect */
    if (fabsf(d->spin_momentum) > 0.05f) {
        /* Apply spin directly to facing for responsive feel */
        d->facing += d->spin_momentum * dt * 4.0f;  /* Faster spin speed */
        
        /* Decay momentum */
        d->spin_momentum *= (1.0f - dt * 2.0f);
        if (fabsf(d->spin_momentum) < 0.05f) d->spin_momentum = 0;
    }
    
    /* After spin completes, smoothly return facing toward 0 (front-facing) */
    if (fabsf(d->spin_momentum) < 0.1f) {
        /* Normalize facing to -PI to PI */
        while (d->facing > 3.14159f) d->facing -= 6.28318f;
        while (d->facing < -3.14159f) d->facing += 6.28318f;
        
        /* Smoothly return toward front (facing = 0) */
        float facing_speed = 2.0f + effective_energy * 2.0f;
        d->facing *= (1.0f - dt * facing_speed * 0.5f);  /* Decay toward 0 */
        if (fabsf(d->facing) < 0.05f) d->facing = 0.0f;
    }
    
    /* AUTO-SPIN: Trigger automatic spins on strong beats at high energy */
    if (!is_silent && a->beat.beat_detected && effective_energy > 0.55f) {
        if (a->bass_smooth > 0.5f && fabsf(d->spin_momentum) < 0.5f) {
            /* 25% chance of auto-spin on strong bass beat */
            if (random_float(d) < 0.25f) {
                int spin_dir = (random_float(d) < 0.5f) ? 1 : -1;
                d->spin_momentum += spin_dir * 4.71f;  /* 3/4 spin */
            }
        }
    }
    
    /* Also trigger smaller spins/turns more frequently at medium energy */
    if (!is_silent && a->treble_velocity > 1.5f && effective_energy > 0.4f) {
        if (fabsf(d->spin_momentum) < 0.3f && random_float(d) < 0.1f) {
            int spin_dir = (random_float(d) < 0.5f) ? 1 : -1;
            d->spin_momentum += spin_dir * 1.57f;  /* Quarter turn */
        }
    }
    
    /* Decay energy boost over time (fast for immediate punch) */
    d->energy_boost *= (1.0f - dt * 1.5f);
    if (fabsf(d->energy_boost) < 0.01f) d->energy_boost = 0.0f;
    
    /* Decay energy override VERY slowly (persist for ~15 seconds) */
    d->energy_override *= (1.0f - dt * 0.07f);
    if (fabsf(d->energy_override) < 0.02f) d->energy_override = 0.0f;
    
    /* Calculate pose duration based on tempo */
    float min_duration = (d->tempo > 0.01f) ? (0.3f / d->tempo) : 10.0f;
    float max_duration = (d->tempo > 0.01f) ? (1.0f / d->tempo) : 30.0f;
    d->pose_duration = min_duration + (1.0f - effective_energy) * (max_duration - min_duration);
    
    /* Check if we should transition to a new pose */
    bool should_transition = false;
    
    /* Only transition if there's audio */
    if (!is_silent && d->time_in_pose > d->pose_duration) {
        should_transition = true;
    }
    
    /* Use beat_phase for more precise beat-triggered transitions */
    /* Trigger near the beat (phase close to 0 or 1) */
    bool on_beat = (beat_phase < 0.1f || beat_phase > 0.9f);
    bool on_half_beat = (beat_phase > 0.45f && beat_phase < 0.55f);
    
    /* High energy = more frequent pose changes on beats AND half-beats */
    if (!is_silent && on_beat && d->time_in_pose > 0.12f) {
        float transition_chance = 0.3f + effective_energy * 0.5f;  /* 30-80% chance */
        if (random_float(d) < transition_chance) {
            should_transition = true;
        }
    }
    
    /* At very high energy, also transition on half-beats */
    if (!is_silent && on_half_beat && effective_energy > 0.6f && d->time_in_pose > 0.1f) {
        if (random_float(d) < 0.4f) {
            should_transition = true;
        }
    }
    
    /* Sudden bass hit = instant pose change */
    if (!is_silent && a->bass_smooth > 0.7f && d->time_in_pose > 0.15f) {
        if (random_float(d) < 0.6f) {
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
    
    /* Update blend - MUCH faster at high energy for snappy movements */
    float blend_speed = 3.0f + effective_energy * 12.0f;  /* Range: 3-15 */
    d->blend += dt * blend_speed;
    if (d->blend > 1.0f) d->blend = 1.0f;
    
    /* Use beat_phase for rhythmic modifiers - but only when not silent */
    float beat_sin = sinf(beat_phase * 2.0f * M_PI);  /* Oscillates with beat */
    float beat_cos = cosf(beat_phase * 2.0f * M_PI);
    float beat_bounce = (beat_phase < 0.15f) ? (1.0f - beat_phase * 6.67f) : 0.0f;
    
    /* Energy-based intensity multiplier - makes movements MUCH bigger at high energy */
    float intensity = 0.3f + effective_energy * 2.5f;  /* Range: 0.3 to 2.8 */
    float bass_intensity = 0.5f + a->bass_smooth * 2.0f;
    float treble_intensity = 0.5f + a->treble_smooth * 2.0f;
    
    /* Scale all modifiers by energy (becomes subtle breathing when silent) */
    float mod_scale = is_silent ? 0.1f : 1.0f;  /* Keep tiny movement when silent */
    
    /* Subtle idle breathing animation when silent */
    float breathe = sinf(d->time_total * 1.5f) * 0.005f;
    
    /* Head bob - locked to beat phase, MUCH more pronounced */
    float extra_bob = sinf(d->time_total * 3.5f) * 0.03f * effective_energy;  /* Extra groove bob */
    float target_bob = beat_sin * 0.10f * a->mid_smooth * intensity * mod_scale + extra_bob;
    if (is_silent) target_bob = breathe;  /* Gentle breathing when quiet */
    d->head_bob = d->head_bob * 0.6f + target_bob * 0.4f;  /* Even faster response */
    
    /* Arm swing - quarter beat offset for groove feel, MORE dynamic */
    float arm_phase = beat_phase + 0.25f;  /* Offset by quarter beat */
    if (arm_phase > 1.0f) arm_phase -= 1.0f;
    float arm_base = sinf(arm_phase * 2.0f * M_PI);
    float arm_double = sinf(arm_phase * 4.0f * M_PI) * 0.4f;  /* Double-time accent */
    
    /* Extra arm wiggle based on treble - makes arms more lively */
    float arm_wiggle = sinf(d->time_total * 8.0f) * 0.05f * a->treble_smooth;
    float arm_wiggle2 = sinf(d->time_total * 9.2f + 1.5f) * 0.04f * a->treble_smooth;
    /* Random-ish arm flourish */
    float arm_flourish = sinf(d->time_total * 2.7f) * sinf(d->time_total * 4.3f) * 0.03f * effective_energy;
    
    d->arm_swing_l = (arm_base + arm_double) * 0.12f * treble_intensity * mod_scale + arm_wiggle + arm_flourish;
    d->arm_swing_r = (sinf((arm_phase + 0.5f) * 2.0f * M_PI) - arm_double) * 0.12f * treble_intensity * mod_scale + arm_wiggle2 - arm_flourish;
    
    /* Hip sway - MUCH more pronounced, figure-8 motion */
    float hip_x = sinf(beat_phase * 2.0f * M_PI) * 0.10f * bass_intensity * mod_scale;
    float hip_y = sinf(beat_phase * 4.0f * M_PI) * 0.05f * bass_intensity * mod_scale;
    d->hip_sway = hip_x;
    
    /* Extra body wiggle - continuous subtle movement */
    float body_wiggle = sinf(d->time_total * 5.0f) * 0.025f * effective_energy;
    float body_twist = sinf(d->time_total * 3.3f) * 0.02f * effective_energy;
    
    /* Bounce - sharper attack on beat, BIG bounces on bass hits */
    float target_bounce = beat_bounce * 0.15f * intensity * mod_scale;
    /* Extra bounce on detected bass */
    if (!is_silent && a->bass_smooth > 0.5f) {
        target_bounce += 0.08f * a->bass_smooth;
    }
    /* Continuous micro-bounce for groove */
    target_bounce += fabsf(sinf(d->time_total * 6.0f)) * 0.03f * effective_energy * mod_scale;
    d->bounce = d->bounce * 0.55f + target_bounce * 0.45f;  /* Even faster attack */
    
    /* Lean - follows spectral centroid with more range + groove sway */
    float groove_sway = sinf(d->time_total * 2.5f) * 0.04f * effective_energy;
    float target_lean = (a->spectral_centroid - 0.5f) * 0.10f * intensity * mod_scale + groove_sway + body_wiggle + body_twist;
    d->lean = d->lean * 0.65f + target_lean * 0.35f;
    
    /* Shoulder shimmy - new! Reacts to high frequencies */
    float shoulder_shimmy = beat_cos * 0.03f * a->treble_smooth * treble_intensity * mod_scale;
    
    /* Knee pump - new! Extra bounce for the lower body */
    float knee_pump = beat_bounce * 0.04f * bass_intensity * mod_scale;
    
    /* Interpolate base pose */
    const Pose *p1 = &d->poses[d->pose_primary];
    const Pose *p2 = &d->poses[d->pose_secondary];
    float eased_blend = ease_in_out_cubic(d->blend);
    
    for (int i = 0; i < JOINT_COUNT; i++) {
        /* Base interpolation */
        Joint target = joint_lerp(p1->joints[i], p2->joints[i], eased_blend);
        
        /* Apply rhythmic modifiers - MORE COMPREHENSIVE */
        if (i == JOINT_HEAD) {
            target.y += d->head_bob - d->bounce * 0.8f;
            target.x += shoulder_shimmy * 0.3f;  /* Head follows shoulders slightly */
        } else if (i == JOINT_SHOULDER_L) {
            target.y += shoulder_shimmy;
            target.y -= d->bounce * 0.5f;
        } else if (i == JOINT_SHOULDER_R) {
            target.y -= shoulder_shimmy;
            target.y -= d->bounce * 0.5f;
        } else if (i == JOINT_HAND_L) {
            target.x += d->arm_swing_l;
            target.y += d->arm_swing_l * 0.5f;  /* Arms move in arc */
        } else if (i == JOINT_HAND_R) {
            target.x += d->arm_swing_r;
            target.y += d->arm_swing_r * 0.5f;
        } else if (i == JOINT_ELBOW_L) {
            target.x += d->arm_swing_l * 0.6f;
        } else if (i == JOINT_ELBOW_R) {
            target.x += d->arm_swing_r * 0.6f;
        } else if (i == JOINT_HIP_CENTER) {
            target.x += d->hip_sway;
            target.y += hip_y;  /* Figure-8 motion */
        } else if (i == JOINT_HIP_L) {
            target.x += d->hip_sway + 0.01f;
            target.y += hip_y * 0.8f;
        } else if (i == JOINT_HIP_R) {
            target.x += d->hip_sway - 0.01f;
            target.y += hip_y * 0.8f;
        } else if (i == JOINT_KNEE_L || i == JOINT_KNEE_R) {
            target.y += knee_pump;  /* Knees bend with beat */
        } else if (i == JOINT_FOOT_L || i == JOINT_FOOT_R) {
            target.y += knee_pump * 0.5f;
        }
        
        /* Global bounce and lean */
        target.y -= d->bounce * 0.6f;
        target.x += d->lean;
        
        /* Update physics */
        d->physics[i].target = target;
        
        /* Dynamic physics - loose and flowy at low energy, snappy at high */
        float stiffness = 12.0f + effective_energy * 25.0f;  /* Range: 12-37 */
        float damping = 6.0f + effective_energy * 6.0f;      /* Range: 6-12 */
        
        /* Extremities are looser for natural follow-through */
        if (i == JOINT_HAND_L || i == JOINT_HAND_R) {
            stiffness *= 0.5f;
            damping *= 0.6f;
        } else if (i == JOINT_FOOT_L || i == JOINT_FOOT_R) {
            stiffness *= 0.6f;
            damping *= 0.7f;
        } else if (i == JOINT_ELBOW_L || i == JOINT_ELBOW_R) {
            stiffness *= 0.7f;
            damping *= 0.75f;
        }
        
        d->physics[i].stiffness = stiffness;
        d->physics[i].damping = damping;
        
        update_joint_physics(&d->physics[i], dt);
        d->current[i] = d->physics[i].position;
    }
    
    /* ============ KNEE CONSTRAINT SYSTEM (v2.4) ============
     * Prevents knees from collapsing inward (knock-kneed look)
     * 
     * Rules:
     * - Define centerline cx at hip center
     * - Left knee must be <= cx - knee_offset when planted
     * - Right knee must be >= cx + knee_offset when planted
     * - beat_phase [0..0.5] = left foot planted
     * - beat_phase [0.5..1] = right foot planted
     * - Swinging foot has relaxed constraint (just don't cross center)
     */
    {
        float cx = d->current[JOINT_HIP_CENTER].x;  /* Centerline */
        float knee_offset = 0.04f;  /* Minimum distance from center */
        float knee_offset_swing = 0.01f;  /* Relaxed offset for swinging leg */
        
        /* Determine which foot is planted based on beat_phase */
        bool left_planted = (beat_phase < 0.5f);
        bool right_planted = !left_planted;
        
        /* Apply constraint to left knee */
        float left_limit = cx - (left_planted ? knee_offset : knee_offset_swing);
        if (d->current[JOINT_KNEE_L].x > left_limit) {
            d->current[JOINT_KNEE_L].x = left_limit;
            d->physics[JOINT_KNEE_L].position.x = left_limit;
            d->physics[JOINT_KNEE_L].velocity.x *= -0.3f;  /* Bounce back */
        }
        
        /* Apply constraint to right knee */
        float right_limit = cx + (right_planted ? knee_offset : knee_offset_swing);
        if (d->current[JOINT_KNEE_R].x < right_limit) {
            d->current[JOINT_KNEE_R].x = right_limit;
            d->physics[JOINT_KNEE_R].position.x = right_limit;
            d->physics[JOINT_KNEE_R].velocity.x *= -0.3f;  /* Bounce back */
        }
        
        /* Also constrain feet to follow knees outward */
        float foot_offset = knee_offset * 0.5f;
        float left_foot_limit = cx - foot_offset;
        float right_foot_limit = cx + foot_offset;
        
        if (d->current[JOINT_FOOT_L].x > left_foot_limit && left_planted) {
            d->current[JOINT_FOOT_L].x = left_foot_limit;
            d->physics[JOINT_FOOT_L].position.x = left_foot_limit;
        }
        if (d->current[JOINT_FOOT_R].x < right_foot_limit && right_planted) {
            d->current[JOINT_FOOT_R].x = right_foot_limit;
            d->physics[JOINT_FOOT_R].position.x = right_foot_limit;
        }
    }
    
    /* Update cached body bounds for particle exclusion (v2.4) */
    {
        float min_x = 1.0f, max_x = 0.0f;
        float min_y = 1.0f, max_y = 0.0f;
        float sum_x = 0.0f, sum_y = 0.0f;
        
        for (int i = 0; i < JOINT_COUNT; i++) {
            float x = d->current[i].x;
            float y = d->current[i].y;
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
            sum_x += x;
            sum_y += y;
        }
        
        d->body_center_x = sum_x / JOINT_COUNT;
        d->body_center_y = sum_y / JOINT_COUNT;
        d->body_left_x = min_x - 0.02f;   /* Add margin */
        d->body_right_x = max_x + 0.02f;
        d->body_top_y = min_y - 0.03f;    /* Head is at min Y */
        d->body_bottom_y = max_y + 0.01f;
    }
    
    /* Advance phase using external beat_phase for synchronization */
    d->phase = beat_phase;
}

/* ============ Body Bounds Accessors (v2.4) ============ */

void skeleton_dancer_get_bounds(const SkeletonDancer *d,
                                float *center_x, float *center_y,
                                float *top_y, float *bottom_y,
                                float *left_x, float *right_x) {
    if (!d) return;
    if (center_x) *center_x = d->body_center_x;
    if (center_y) *center_y = d->body_center_y;
    if (top_y) *top_y = d->body_top_y;
    if (bottom_y) *bottom_y = d->body_bottom_y;
    if (left_x) *left_x = d->body_left_x;
    if (right_x) *right_x = d->body_right_x;
}

void skeleton_dancer_get_bounds_pixels(const SkeletonDancer *d,
                                       int *center_x, int *center_y,
                                       int *top_y, int *bottom_y,
                                       int *left_x, int *right_x) {
    if (!d) return;
    
    /* Convert from normalized (0-1) to pixel coordinates */
    if (center_x) *center_x = (int)((d->body_center_x - 0.5f) * d->scale + d->offset_x);
    if (center_y) *center_y = (int)(d->body_center_y * d->scale + d->offset_y);
    if (top_y) *top_y = (int)(d->body_top_y * d->scale + d->offset_y);
    if (bottom_y) *bottom_y = (int)(d->body_bottom_y * d->scale + d->offset_y);
    if (left_x) *left_x = (int)((d->body_left_x - 0.5f) * d->scale + d->offset_x);
    if (right_x) *right_x = (int)((d->body_right_x - 0.5f) * d->scale + d->offset_x);
}

/* ============ v3.1: Energy Override System ============ */

void skeleton_dancer_adjust_energy(SkeletonDancer *d, float amount) {
    if (!d) return;
    
    /* Direct add to override (clamped between -1 and 1) */
    d->energy_override += amount * 3.0f;  /* Triple the effect for visibility */
    if (d->energy_override > 1.0f) d->energy_override = 1.0f;
    if (d->energy_override < -1.0f) d->energy_override = -1.0f;
    
    /* Also add a BIG temporary boost for immediate feedback */
    d->energy_boost += amount * 2.0f;  /* Very strong immediate feedback */
    if (d->energy_boost > 1.0f) d->energy_boost = 1.0f;
    if (d->energy_boost < -1.0f) d->energy_boost = -1.0f;
}

void skeleton_dancer_toggle_energy_lock(SkeletonDancer *d) {
    if (!d) return;
    d->energy_locked = !d->energy_locked;
}

float skeleton_dancer_get_effective_energy(SkeletonDancer *d) {
    if (!d) return 0.5f;
    
    float base_energy = d->energy_locked ? 0.5f : d->audio.energy_smooth;
    
    /* Apply override: maps -1..1 to halving..doubling energy */
    float multiplier = 1.0f + d->energy_override;
    float effective = base_energy * multiplier + d->energy_boost;
    
    /* Clamp to valid range */
    if (effective < 0.0f) effective = 0.0f;
    if (effective > 1.0f) effective = 1.0f;
    
    return effective;
}

bool skeleton_dancer_is_energy_locked(const SkeletonDancer *d) {
    if (!d) return false;
    return d->energy_locked;
}

float skeleton_dancer_get_energy_override(const SkeletonDancer *d) {
    if (!d) return 0.0f;
    return d->energy_override;
}

/* ============ v3.1: Facing/Spin Control ============ */

void skeleton_dancer_trigger_spin(SkeletonDancer *d, int direction) {
    if (!d) return;
    
    /* Add spin momentum (full rotation = 2*PI) */
    d->spin_momentum += direction * 6.28f;  /* Full 360 spin */
    
    /* Cap maximum spin momentum */
    if (d->spin_momentum > 12.56f) d->spin_momentum = 12.56f;   /* 2 full spins max */
    if (d->spin_momentum < -12.56f) d->spin_momentum = -12.56f;
}

float skeleton_dancer_get_facing(const SkeletonDancer *d) {
    if (!d) return 0.0f;
    return d->facing;
}
