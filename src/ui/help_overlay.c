/*
 * Help Overlay Implementation - ASCII Dancer v2.4+
 *
 * Renders a toggleable help screen with controls and current settings.
 * Uses box-drawing characters for clean terminal appearance.
 */

#include "help_overlay.h"
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

/* ============ Box Drawing Characters ============ */

#define BOX_TL "╭"
#define BOX_TR "╮"
#define BOX_BL "╰"
#define BOX_BR "╯"
#define BOX_H  "─"
#define BOX_V  "│"
#define BOX_T  "┬"
#define BOX_B  "┴"

/* ============ Help Content ============ */

typedef struct {
    const char *key;
    const char *desc;
} HelpLine;

static const HelpLine help_controls[] = {
    {"q / ESC",    "Quit application"},
    {"? / F1",     "Toggle this help"},
    {"",           ""},
    {"t",          "Cycle color themes"},
    {"+/-",        "Adjust sensitivity"},
    {"",           ""},
    {"g",          "Toggle ground line"},
    {"r",          "Toggle reflection/shadow"},
    {"p",          "Toggle particles"},
    {"m",          "Toggle motion trails"},
    {"b",          "Toggle breathing effect"},
    {"",           ""},
    {"d",          "Toggle debug mode"},
    {NULL, NULL}
};

/* ============ Creation / Destruction ============ */

HelpOverlay* help_overlay_create(void) {
    HelpOverlay *help = calloc(1, sizeof(HelpOverlay));
    if (!help) return NULL;
    
    help->visible = false;
    help->fade_alpha = 0.0f;
    help->fade_speed = 8.0f;  /* Fast fade */
    help->scroll_offset = 0;
    
    /* Count content lines */
    help->content_height = 0;
    for (int i = 0; help_controls[i].key != NULL; i++) {
        help->content_height++;
    }
    
    return help;
}

void help_overlay_destroy(HelpOverlay *help) {
    if (help) free(help);
}

/* ============ Control ============ */

void help_overlay_toggle(HelpOverlay *help) {
    if (!help) return;
    help->visible = !help->visible;
}

void help_overlay_show(HelpOverlay *help) {
    if (!help) return;
    help->visible = true;
}

void help_overlay_hide(HelpOverlay *help) {
    if (!help) return;
    help->visible = false;
}

bool help_overlay_is_visible(HelpOverlay *help) {
    return help && help->visible;
}

bool help_overlay_is_active(HelpOverlay *help) {
    return help && (help->visible || help->fade_alpha > 0.01f);
}

void help_overlay_scroll(HelpOverlay *help, int delta) {
    if (!help) return;
    help->scroll_offset += delta;
    if (help->scroll_offset < 0) help->scroll_offset = 0;
    /* Max scroll handled in render based on visible height */
}

/* ============ Update ============ */

void help_overlay_update(HelpOverlay *help, float dt) {
    if (!help) return;
    
    float target = help->visible ? 1.0f : 0.0f;
    float diff = target - help->fade_alpha;
    
    if (diff > 0.01f || diff < -0.01f) {
        help->fade_alpha += diff * help->fade_speed * dt;
        if (help->fade_alpha > 1.0f) help->fade_alpha = 1.0f;
        if (help->fade_alpha < 0.0f) help->fade_alpha = 0.0f;
    } else {
        help->fade_alpha = target;
    }
}

/* ============ Rendering ============ */

/* Draw a horizontal line with box characters */
static void draw_hline(int y, int x, int width, const char *left, 
                       const char *mid, const char *right) {
    mvprintw(y, x, "%s", left);
    for (int i = 1; i < width - 1; i++) {
        printw("%s", mid);
    }
    printw("%s", right);
}

/* Render centered text */
static void render_centered(int y, int x, int width, const char *text) {
    int len = strlen(text);
    int pad = (width - len) / 2;
    if (pad < 0) pad = 0;
    mvprintw(y, x + pad, "%s", text);
}

void help_overlay_render(HelpOverlay *help,
                         int screen_width, int screen_height,
                         const char *theme_name,
                         float bpm, float sensitivity,
                         bool ground_on, bool shadow_on,
                         bool particles_on, bool trails_on,
                         bool breathing_on) {
    if (!help || help->fade_alpha < 0.01f) return;
    
    /* Calculate overlay dimensions */
    int box_width = 44;
    int box_height = 22;
    
    /* Ensure box fits on screen */
    if (box_width > screen_width - 4) box_width = screen_width - 4;
    if (box_height > screen_height - 2) box_height = screen_height - 2;
    
    /* Center the box */
    int box_x = (screen_width - box_width) / 2;
    int box_y = (screen_height - box_height) / 2;
    
    /* Semi-transparent effect: dim/fade appearance based on alpha
     * Since terminal can't do true transparency, we use dimmer colors */
    if (help->fade_alpha < 1.0f) {
        attron(A_DIM);
    }
    
    /* Draw background (fill with spaces) */
    for (int y = box_y; y < box_y + box_height; y++) {
        mvhline(y, box_x, ' ', box_width);
    }
    
    /* Draw box border */
    attron(A_BOLD);
    draw_hline(box_y, box_x, box_width, BOX_TL, BOX_H, BOX_TR);
    draw_hline(box_y + box_height - 1, box_x, box_width, BOX_BL, BOX_H, BOX_BR);
    
    for (int y = box_y + 1; y < box_y + box_height - 1; y++) {
        mvprintw(y, box_x, "%s", BOX_V);
        mvprintw(y, box_x + box_width - 1, "%s", BOX_V);
    }
    attroff(A_BOLD);
    
    /* Title */
    attron(A_BOLD | A_UNDERLINE);
    render_centered(box_y + 1, box_x, box_width, "ASCII DANCER HELP");
    attroff(A_BOLD | A_UNDERLINE);
    
    /* Version / subtitle */
    render_centered(box_y + 2, box_x, box_width, "v2.4 - Audio Visualizer");
    
    /* Separator */
    draw_hline(box_y + 3, box_x, box_width, "├", "─", "┤");
    
    /* Controls section */
    int content_y = box_y + 4;
    int key_col = box_x + 3;
    int desc_col = box_x + 14;
    
    attron(A_BOLD);
    mvprintw(content_y++, key_col, "CONTROLS");
    attroff(A_BOLD);
    content_y++;
    
    for (int i = 0; help_controls[i].key != NULL && content_y < box_y + box_height - 8; i++) {
        if (strlen(help_controls[i].key) == 0) {
            content_y++;  /* Empty line = spacer */
            continue;
        }
        attron(A_BOLD);
        mvprintw(content_y, key_col, "%-10s", help_controls[i].key);
        attroff(A_BOLD);
        mvprintw(content_y, desc_col, "%s", help_controls[i].desc);
        content_y++;
    }
    
    /* Separator before status */
    draw_hline(box_y + box_height - 7, box_x, box_width, "├", "─", "┤");
    
    /* Current status section */
    int status_y = box_y + box_height - 6;
    
    attron(A_BOLD);
    mvprintw(status_y++, key_col, "CURRENT STATUS");
    attroff(A_BOLD);
    status_y++;
    
    /* Status values */
    mvprintw(status_y, key_col, "Theme:");
    attron(A_BOLD);
    mvprintw(status_y++, desc_col, "%s", theme_name);
    attroff(A_BOLD);
    
    mvprintw(status_y, key_col, "BPM:");
    mvprintw(status_y, desc_col, "%.0f", bpm);
    mvprintw(status_y, desc_col + 8, "Sens:");
    mvprintw(status_y++, desc_col + 14, "%.1f", sensitivity);
    
    /* Toggle states */
    mvprintw(status_y, key_col, "Effects:");
    mvprintw(status_y, desc_col, "%s %s %s %s %s",
             ground_on ? "[G]" : " G ",
             shadow_on ? "[R]" : " R ",
             particles_on ? "[P]" : " P ",
             trails_on ? "[M]" : " M ",
             breathing_on ? "[B]" : " B ");
    
    /* Footer hint */
    render_centered(box_y + box_height - 2, box_x, box_width, 
                    "Press ? or F1 to close");
    
    if (help->fade_alpha < 1.0f) {
        attroff(A_DIM);
    }
}
