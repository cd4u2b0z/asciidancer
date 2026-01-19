/*
 * Help Overlay Module - ASCII Dancer v2.4+
 *
 * Toggleable help screen showing controls and status.
 * Renders a semi-transparent overlay with keybindings.
 */

#ifndef HELP_OVERLAY_H
#define HELP_OVERLAY_H

#include <stdbool.h>

/* Help overlay state */
typedef struct {
    bool visible;
    float fade_alpha;       /* 0.0 = hidden, 1.0 = fully visible */
    float fade_speed;       /* Fade animation speed */
    int scroll_offset;      /* For scrollable content */
    int content_height;     /* Total help content lines */
} HelpOverlay;

/* ============ Lifecycle ============ */

/* Create help overlay state */
HelpOverlay* help_overlay_create(void);

/* Destroy help overlay */
void help_overlay_destroy(HelpOverlay *help);

/* ============ Control ============ */

/* Toggle help visibility (with fade animation) */
void help_overlay_toggle(HelpOverlay *help);

/* Show/hide explicitly */
void help_overlay_show(HelpOverlay *help);
void help_overlay_hide(HelpOverlay *help);

/* Check if currently visible or animating */
bool help_overlay_is_visible(HelpOverlay *help);
bool help_overlay_is_active(HelpOverlay *help);  /* visible or fading */

/* Scroll content (for long help) */
void help_overlay_scroll(HelpOverlay *help, int delta);

/* ============ Update & Render ============ */

/* Update fade animation */
void help_overlay_update(HelpOverlay *help, float dt);

/* Render help overlay to ncurses
 * Parameters for dynamic status display */
void help_overlay_render(HelpOverlay *help,
                         int screen_width, int screen_height,
                         const char *theme_name,
                         float bpm, float sensitivity,
                         bool ground_on, bool shadow_on,
                         bool particles_on, bool trails_on,
                         bool breathing_on);

#endif /* HELP_OVERLAY_H */
