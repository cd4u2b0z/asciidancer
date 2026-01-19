// Procedural Braille dancer with smooth cava-like animation
// Draws a stick figure that moves fluidly based on audio values

#include "dancer.h"
#include "braille.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Pixel dimensions (braille gives 2x4 resolution per char)
#define CANVAS_W (CANVAS_CHARS_W * 2)  // 60 pixels wide
#define CANVAS_H (CANVAS_CHARS_H * 4)  // 60 pixels tall

// Body proportions (in pixels)
#define HEAD_RADIUS 3
#define NECK_LENGTH 3
#define TORSO_LENGTH 14
#define UPPER_ARM 9
#define LOWER_ARM 7
#define UPPER_LEG 11
#define LOWER_LEG 9
#define SHOULDER_WIDTH 8
#define HIP_WIDTH 6

// Smoothing factors
#define SMOOTH_FAST 0.3
#define SMOOTH_SLOW 0.15

// Internal smooth state
static double s_bass = 0, s_mid = 0, s_treble = 0;
static double s_energy = 0;
static struct braille_canvas *canvas = NULL;

void dancer_init(struct dancer_state *state) {
    memset(state, 0, sizeof(*state));
    s_bass = s_mid = s_treble = s_energy = 0;
    
    if (!canvas) {
        canvas = braille_create(CANVAS_CHARS_W, CANVAS_CHARS_H);
    }
}

void dancer_cleanup(void) {
    if (canvas) {
        braille_free(canvas);
        canvas = NULL;
    }
}

// Draw a line with optional thickness
static void draw_limb(int x0, int y0, int x1, int y1) {
    braille_line(canvas, x0, y0, x1, y1);
    // Add slight thickness
    braille_line(canvas, x0+1, y0, x1+1, y1);
}

// Calculate end point of limb given start, length, and angle
static void limb_end(int sx, int sy, double len, double angle, int *ex, int *ey) {
    *ex = sx + (int)(len * sin(angle));
    *ey = sy + (int)(len * cos(angle));
}

void dancer_update(struct dancer_state *state, double bass, double mid, double treble) {
    // Smooth the inputs for fluid motion
    s_bass = s_bass * (1.0 - SMOOTH_FAST) + bass * SMOOTH_FAST;
    s_mid = s_mid * (1.0 - SMOOTH_FAST) + mid * SMOOTH_FAST;
    s_treble = s_treble * (1.0 - SMOOTH_SLOW) + treble * SMOOTH_SLOW;
    s_energy = (s_bass + s_mid + s_treble) / 3.0;
    
    // Store smoothed values
    state->bass_intensity = s_bass;
    state->mid_intensity = s_mid;
    state->treble_intensity = s_treble;
    
    // Advance animation phase - faster with more energy
    double phase_speed = 0.08 + s_energy * 0.15;
    state->phase += phase_speed;
    if (state->phase > M_PI * 4) state->phase -= M_PI * 4;
}

void dancer_compose_frame(struct dancer_state *state, char *output) {
    if (!canvas) {
        canvas = braille_create(CANVAS_CHARS_W, CANVAS_CHARS_H);
    }
    braille_clear(canvas);
    
    double bass = state->bass_intensity;
    double mid = state->mid_intensity;
    double treble = state->treble_intensity;
    double phase = state->phase;
    
    // Base position
    int cx = CANVAS_W / 2;
    int base_y = 6;  // Head Y position
    
    // === MOVEMENT CALCULATIONS ===
    // Bounce (bass drives vertical bounce)
    double bounce = sin(phase * 2) * bass * 4;
    
    // Sway (mid drives horizontal sway)
    double sway = sin(phase) * mid * 6;
    
    // Body lean (follows sway)
    double lean = sin(phase) * mid * 0.25;
    
    // === HEAD ===
    int head_x = cx + (int)sway;
    int head_y = base_y + (int)bounce;
    braille_filled_circle(canvas, head_x, head_y, HEAD_RADIUS);
    
    // === NECK & SHOULDERS ===
    int neck_x = head_x + (int)(lean * 2);
    int neck_y = head_y + HEAD_RADIUS + NECK_LENGTH;
    braille_line(canvas, head_x, head_y + HEAD_RADIUS, neck_x, neck_y);
    
    int shoulder_y = neck_y + 2;
    int lshoulder_x = neck_x - SHOULDER_WIDTH/2;
    int rshoulder_x = neck_x + SHOULDER_WIDTH/2;
    
    // === TORSO ===
    int hip_y = shoulder_y + TORSO_LENGTH + (int)(bounce * 0.3);
    int hip_x = neck_x + (int)(lean * 4);
    
    // Spine
    braille_line(canvas, neck_x, neck_y, hip_x, hip_y);
    // Shoulders
    braille_line(canvas, lshoulder_x, shoulder_y, rshoulder_x, shoulder_y);
    
    // === ARMS ===
    // Arm angles driven by mid and treble
    double arm_base = M_PI * 0.15;  // Slight outward angle
    double arm_swing = sin(phase * 2) * (0.3 + mid * 0.5);
    double arm_raise = treble * M_PI * 0.4;  // Treble raises arms
    
    // Left arm
    double l_upper_angle = -arm_base - arm_swing - arm_raise;
    double l_lower_angle = l_upper_angle + 0.3 + bass * 0.5;
    
    int l_elbow_x, l_elbow_y, l_hand_x, l_hand_y;
    limb_end(lshoulder_x, shoulder_y, UPPER_ARM, l_upper_angle, &l_elbow_x, &l_elbow_y);
    limb_end(l_elbow_x, l_elbow_y, LOWER_ARM, l_lower_angle, &l_hand_x, &l_hand_y);
    
    draw_limb(lshoulder_x, shoulder_y, l_elbow_x, l_elbow_y);
    draw_limb(l_elbow_x, l_elbow_y, l_hand_x, l_hand_y);
    
    // Right arm (mirrored swing)
    double r_upper_angle = arm_base + arm_swing - arm_raise;
    double r_lower_angle = r_upper_angle - 0.3 - bass * 0.5;
    
    int r_elbow_x, r_elbow_y, r_hand_x, r_hand_y;
    limb_end(rshoulder_x, shoulder_y, UPPER_ARM, r_upper_angle, &r_elbow_x, &r_elbow_y);
    limb_end(r_elbow_x, r_elbow_y, LOWER_ARM, r_lower_angle, &r_hand_x, &r_hand_y);
    
    draw_limb(rshoulder_x, shoulder_y, r_elbow_x, r_elbow_y);
    draw_limb(r_elbow_x, r_elbow_y, r_hand_x, r_hand_y);
    
    // === HIPS ===
    int lhip_x = hip_x - HIP_WIDTH/2;
    int rhip_x = hip_x + HIP_WIDTH/2;
    braille_line(canvas, lhip_x, hip_y, rhip_x, hip_y);
    
    // === LEGS ===
    // Leg angles driven by bass (stomping/stepping)
    double leg_spread = 0.15 + bass * 0.25;
    double leg_step = sin(phase) * (0.2 + bass * 0.4);
    
    // Left leg
    double l_thigh_angle = leg_spread - leg_step;
    double l_shin_angle = l_thigh_angle + 0.1 + (leg_step > 0 ? leg_step * 0.8 : 0);
    
    int l_knee_x, l_knee_y, l_foot_x, l_foot_y;
    limb_end(lhip_x, hip_y, UPPER_LEG, l_thigh_angle, &l_knee_x, &l_knee_y);
    limb_end(l_knee_x, l_knee_y, LOWER_LEG, l_shin_angle, &l_foot_x, &l_foot_y);
    
    draw_limb(lhip_x, hip_y, l_knee_x, l_knee_y);
    draw_limb(l_knee_x, l_knee_y, l_foot_x, l_foot_y);
    
    // Right leg (opposite phase)
    double r_thigh_angle = -leg_spread + leg_step;
    double r_shin_angle = r_thigh_angle - 0.1 + (leg_step < 0 ? -leg_step * 0.8 : 0);
    
    int r_knee_x, r_knee_y, r_foot_x, r_foot_y;
    limb_end(rhip_x, hip_y, UPPER_LEG, r_thigh_angle, &r_knee_x, &r_knee_y);
    limb_end(r_knee_x, r_knee_y, LOWER_LEG, r_shin_angle, &r_foot_x, &r_foot_y);
    
    draw_limb(rhip_x, hip_y, r_knee_x, r_knee_y);
    draw_limb(r_knee_x, r_knee_y, r_foot_x, r_foot_y);
    
    // === OUTPUT ===
    braille_render(canvas, output);
}

void calculate_bands(double *cava_out, int num_bars, 
                     double *bass, double *mid, double *treble) {
    int bass_end = num_bars / 3;
    int mid_end = 2 * num_bars / 3;
    
    double bass_sum = 0, mid_sum = 0, treble_sum = 0;
    
    for (int i = 0; i < num_bars; i++) {
        if (i < bass_end) bass_sum += cava_out[i];
        else if (i < mid_end) mid_sum += cava_out[i];
        else treble_sum += cava_out[i];
    }
    
    *bass = bass_sum / bass_end;
    *mid = mid_sum / (mid_end - bass_end);
    *treble = treble_sum / (num_bars - mid_end);
    
    // Boost and compress for responsiveness
    *bass = sqrt(*bass) * 1.2;
    *mid = sqrt(*mid) * 1.1;
    *treble = sqrt(*treble);
    
    if (*bass > 1.0) *bass = 1.0;
    if (*mid > 1.0) *mid = 1.0;
    if (*treble > 1.0) *treble = 1.0;
}
