/*
 * Braille Canvas Implementation
 * High-resolution terminal graphics using Unicode Braille characters
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include "braille_canvas.h"

/* ============ Canvas Management ============ */

BrailleCanvas* braille_canvas_create(int cell_width, int cell_height) {
    BrailleCanvas *canvas = calloc(1, sizeof(BrailleCanvas));
    if (!canvas) return NULL;
    
    canvas->cell_width = cell_width;
    canvas->cell_height = cell_height;
    canvas->pixel_width = cell_width * BRAILLE_CELL_W;
    canvas->pixel_height = cell_height * BRAILLE_CELL_H;
    
    /* Allocate pixel buffer */
    size_t pixel_count = canvas->pixel_width * canvas->pixel_height;
    canvas->pixels = calloc(pixel_count, sizeof(uint8_t));
    
    /* Allocate cell buffer (+ 1 per row for null terminator) */
    size_t cell_count = cell_width * cell_height;
    canvas->cells = calloc(cell_count + cell_height, sizeof(wchar_t));
    
    /* Allocate dirty flags */
    canvas->dirty = calloc(cell_count, sizeof(uint8_t));
    
    if (!canvas->pixels || !canvas->cells || !canvas->dirty) {
        braille_canvas_destroy(canvas);
        return NULL;
    }
    
    /* Initialize cells to empty braille */
    for (int i = 0; i < cell_count; i++) {
        canvas->cells[i] = BRAILLE_BASE;
    }
    
    return canvas;
}

void braille_canvas_destroy(BrailleCanvas *canvas) {
    if (!canvas) return;
    free(canvas->pixels);
    free(canvas->cells);
    free(canvas->dirty);
    free(canvas);
}

void braille_canvas_clear(BrailleCanvas *canvas) {
    if (!canvas) return;
    memset(canvas->pixels, 0, canvas->pixel_width * canvas->pixel_height);
    memset(canvas->dirty, 1, canvas->cell_width * canvas->cell_height);
}

void braille_canvas_render(BrailleCanvas *canvas) {
    if (!canvas) return;
    
    /* Convert each 2x4 pixel block to a braille character */
    for (int cy = 0; cy < canvas->cell_height; cy++) {
        for (int cx = 0; cx < canvas->cell_width; cx++) {
            int cell_idx = cy * canvas->cell_width + cx;
            
            /* Skip if not dirty (optimization) */
            /* if (!canvas->dirty[cell_idx]) continue; */
            
            uint8_t pattern = 0;
            int px_base = cx * BRAILLE_CELL_W;
            int py_base = cy * BRAILLE_CELL_H;
            
            /* Check each dot in the 2x4 grid */
            for (int dy = 0; dy < BRAILLE_CELL_H; dy++) {
                for (int dx = 0; dx < BRAILLE_CELL_W; dx++) {
                    int px = px_base + dx;
                    int py = py_base + dy;
                    
                    if (px < canvas->pixel_width && py < canvas->pixel_height) {
                        int pixel_idx = py * canvas->pixel_width + px;
                        if (canvas->pixels[pixel_idx]) {
                            pattern |= BRAILLE_DOT_BITS[dy][dx];
                        }
                    }
                }
            }
            
            canvas->cells[cell_idx] = BRAILLE_BASE + pattern;
            canvas->dirty[cell_idx] = 0;
        }
    }
}

const wchar_t* braille_canvas_get_row(BrailleCanvas *canvas, int row) {
    if (!canvas || row < 0 || row >= canvas->cell_height) return NULL;
    return &canvas->cells[row * canvas->cell_width];
}

int braille_canvas_to_utf8(BrailleCanvas *canvas, int row, char *out, int max_len) {
    if (!canvas || !out || row < 0 || row >= canvas->cell_height) return 0;
    
    const wchar_t *wrow = braille_canvas_get_row(canvas, row);
    int written = 0;
    
    for (int i = 0; i < canvas->cell_width && written < max_len - 4; i++) {
        wchar_t wc = wrow[i];
        /* Manual UTF-8 encoding for braille range */
        if (wc >= 0x2800 && wc <= 0x28FF) {
            /* 3-byte UTF-8 encoding */
            out[written++] = 0xE0 | ((wc >> 12) & 0x0F);
            out[written++] = 0x80 | ((wc >> 6) & 0x3F);
            out[written++] = 0x80 | (wc & 0x3F);
        }
    }
    out[written] = '\0';
    return written;
}

/* ============ Pixel Operations ============ */

static inline int pixel_index(BrailleCanvas *canvas, int x, int y) {
    return y * canvas->pixel_width + x;
}

static inline int in_bounds(BrailleCanvas *canvas, int x, int y) {
    return x >= 0 && x < canvas->pixel_width && 
           y >= 0 && y < canvas->pixel_height;
}

static inline void mark_dirty(BrailleCanvas *canvas, int x, int y) {
    int cx = x / BRAILLE_CELL_W;
    int cy = y / BRAILLE_CELL_H;
    if (cx >= 0 && cx < canvas->cell_width && cy >= 0 && cy < canvas->cell_height) {
        canvas->dirty[cy * canvas->cell_width + cx] = 1;
    }
}

void braille_set_pixel(BrailleCanvas *canvas, int x, int y, bool on) {
    if (!canvas || !in_bounds(canvas, x, y)) return;
    canvas->pixels[pixel_index(canvas, x, y)] = on ? 1 : 0;
    mark_dirty(canvas, x, y);
}

bool braille_get_pixel(BrailleCanvas *canvas, int x, int y) {
    if (!canvas || !in_bounds(canvas, x, y)) return false;
    return canvas->pixels[pixel_index(canvas, x, y)] != 0;
}

void braille_toggle_pixel(BrailleCanvas *canvas, int x, int y) {
    if (!canvas || !in_bounds(canvas, x, y)) return;
    int idx = pixel_index(canvas, x, y);
    canvas->pixels[idx] = !canvas->pixels[idx];
    mark_dirty(canvas, x, y);
}

/* ============ Drawing Primitives ============ */

/* Bresenham's line algorithm */
void braille_draw_line(BrailleCanvas *canvas, int x1, int y1, int x2, int y2) {
    if (!canvas) return;
    
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    
    while (1) {
        braille_set_pixel(canvas, x1, y1, true);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 >= dy) {
            if (x1 == x2) break;
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2) break;
            err += dx;
            y1 += sy;
        }
    }
}

/* Xiaolin Wu's anti-aliased line (approximated with dots for braille) */
void braille_draw_line_aa(BrailleCanvas *canvas, int x1, int y1, int x2, int y2) {
    /* For braille (binary pixels), we just use Bresenham */
    /* True AA would need grayscale which braille can't do */
    braille_draw_line(canvas, x1, y1, x2, y2);
}

/* Midpoint circle algorithm */
void braille_draw_circle(BrailleCanvas *canvas, int cx, int cy, int r) {
    if (!canvas || r < 0) return;
    
    int x = r;
    int y = 0;
    int err = 1 - r;
    
    while (x >= y) {
        /* Draw 8 octants */
        braille_set_pixel(canvas, cx + x, cy + y, true);
        braille_set_pixel(canvas, cx + y, cy + x, true);
        braille_set_pixel(canvas, cx - y, cy + x, true);
        braille_set_pixel(canvas, cx - x, cy + y, true);
        braille_set_pixel(canvas, cx - x, cy - y, true);
        braille_set_pixel(canvas, cx - y, cy - x, true);
        braille_set_pixel(canvas, cx + y, cy - x, true);
        braille_set_pixel(canvas, cx + x, cy - y, true);
        
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void braille_fill_circle(BrailleCanvas *canvas, int cx, int cy, int r) {
    if (!canvas || r < 0) return;
    
    for (int y = -r; y <= r; y++) {
        int half_width = (int)sqrt(r * r - y * y);
        for (int x = -half_width; x <= half_width; x++) {
            braille_set_pixel(canvas, cx + x, cy + y, true);
        }
    }
}

void braille_draw_ellipse(BrailleCanvas *canvas, int cx, int cy, int rx, int ry) {
    if (!canvas || rx < 0 || ry < 0) return;
    
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int two_rx2 = 2 * rx2;
    int two_ry2 = 2 * ry2;
    
    int x = 0;
    int y = ry;
    int px = 0;
    int py = two_rx2 * y;
    
    /* Region 1 */
    int p = (int)(ry2 - rx2 * ry + 0.25 * rx2);
    while (px < py) {
        braille_set_pixel(canvas, cx + x, cy + y, true);
        braille_set_pixel(canvas, cx - x, cy + y, true);
        braille_set_pixel(canvas, cx + x, cy - y, true);
        braille_set_pixel(canvas, cx - x, cy - y, true);
        
        x++;
        px += two_ry2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= two_rx2;
            p += ry2 + px - py;
        }
    }
    
    /* Region 2 */
    p = (int)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        braille_set_pixel(canvas, cx + x, cy + y, true);
        braille_set_pixel(canvas, cx - x, cy + y, true);
        braille_set_pixel(canvas, cx + x, cy - y, true);
        braille_set_pixel(canvas, cx - x, cy - y, true);
        
        y--;
        py -= two_rx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += two_ry2;
            p += rx2 - py + px;
        }
    }
}

void braille_draw_rect(BrailleCanvas *canvas, int x, int y, int w, int h) {
    if (!canvas) return;
    braille_draw_line(canvas, x, y, x + w - 1, y);
    braille_draw_line(canvas, x + w - 1, y, x + w - 1, y + h - 1);
    braille_draw_line(canvas, x + w - 1, y + h - 1, x, y + h - 1);
    braille_draw_line(canvas, x, y + h - 1, x, y);
}

void braille_fill_rect(BrailleCanvas *canvas, int x, int y, int w, int h) {
    if (!canvas) return;
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            braille_set_pixel(canvas, px, py, true);
        }
    }
}

/* Quadratic bezier using De Casteljau's algorithm */
void braille_draw_bezier_quad(BrailleCanvas *canvas,
                               int x0, int y0, int x1, int y1, int x2, int y2) {
    if (!canvas) return;
    
    /* Number of segments (more = smoother) */
    int segments = 20;
    int prev_x = x0, prev_y = y0;
    
    for (int i = 1; i <= segments; i++) {
        double t = (double)i / segments;
        double t2 = t * t;
        double mt = 1.0 - t;
        double mt2 = mt * mt;
        
        /* B(t) = (1-t)²P0 + 2(1-t)tP1 + t²P2 */
        int x = (int)(mt2 * x0 + 2 * mt * t * x1 + t2 * x2);
        int y = (int)(mt2 * y0 + 2 * mt * t * y1 + t2 * y2);
        
        braille_draw_line(canvas, prev_x, prev_y, x, y);
        prev_x = x;
        prev_y = y;
    }
}

/* Cubic bezier */
void braille_draw_bezier_cubic(BrailleCanvas *canvas,
                                int x0, int y0, int x1, int y1,
                                int x2, int y2, int x3, int y3) {
    if (!canvas) return;
    
    int segments = 30;
    int prev_x = x0, prev_y = y0;
    
    for (int i = 1; i <= segments; i++) {
        double t = (double)i / segments;
        double t2 = t * t;
        double t3 = t2 * t;
        double mt = 1.0 - t;
        double mt2 = mt * mt;
        double mt3 = mt2 * mt;
        
        /* B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3 */
        int x = (int)(mt3 * x0 + 3 * mt2 * t * x1 + 3 * mt * t2 * x2 + t3 * x3);
        int y = (int)(mt3 * y0 + 3 * mt2 * t * y1 + 3 * mt * t2 * y2 + t3 * y3);
        
        braille_draw_line(canvas, prev_x, prev_y, x, y);
        prev_x = x;
        prev_y = y;
    }
}

/* Draw thick line by drawing multiple parallel lines */
void braille_draw_thick_line(BrailleCanvas *canvas, 
                              int x1, int y1, int x2, int y2, int thickness) {
    if (!canvas || thickness < 1) return;
    
    if (thickness == 1) {
        braille_draw_line(canvas, x1, y1, x2, y2);
        return;
    }
    
    /* Calculate perpendicular offset */
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len = sqrt(dx * dx + dy * dy);
    if (len < 0.001) {
        braille_fill_circle(canvas, x1, y1, thickness / 2);
        return;
    }
    
    /* Perpendicular unit vector */
    double px = -dy / len;
    double py = dx / len;
    
    /* Draw parallel lines */
    int half = thickness / 2;
    for (int i = -half; i <= half; i++) {
        int ox = (int)(px * i);
        int oy = (int)(py * i);
        braille_draw_line(canvas, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
    }
    
    /* Round caps */
    braille_fill_circle(canvas, x1, y1, half);
    braille_fill_circle(canvas, x2, y2, half);
}

/*
 * Scanline flood fill algorithm with bounded memory usage.
 * 
 * Instead of allocating worst-case width*height, we use a fixed-size
 * queue and process scanlines. This is both faster and uses O(height)
 * memory in the worst case for the queue, rather than O(width*height).
 */
#define FLOOD_FILL_QUEUE_SIZE 4096

void braille_flood_fill(BrailleCanvas *canvas, int x, int y, bool fill_value) {
    if (!canvas || !in_bounds(canvas, x, y)) return;
    
    bool target_value = braille_get_pixel(canvas, x, y);
    if (target_value == fill_value) return;
    
    /* Fixed-size circular queue for scanline seeds */
    typedef struct { int x, y; } Point;
    Point *queue = malloc(FLOOD_FILL_QUEUE_SIZE * sizeof(Point));
    if (!queue) return;
    
    int head = 0, tail = 0;
    int queue_size = 0;
    
    /* Helper macro for queue operations */
    #define ENQUEUE(px, py) do { \
        if (queue_size < FLOOD_FILL_QUEUE_SIZE) { \
            queue[tail] = (Point){px, py}; \
            tail = (tail + 1) % FLOOD_FILL_QUEUE_SIZE; \
            queue_size++; \
        } \
    } while(0)
    
    #define DEQUEUE(p) do { \
        p = queue[head]; \
        head = (head + 1) % FLOOD_FILL_QUEUE_SIZE; \
        queue_size--; \
    } while(0)
    
    ENQUEUE(x, y);
    
    while (queue_size > 0) {
        Point seed;
        DEQUEUE(seed);
        
        /* Skip if already filled or out of bounds */
        if (!in_bounds(canvas, seed.x, seed.y)) continue;
        if (braille_get_pixel(canvas, seed.x, seed.y) != target_value) continue;
        
        /* Find left edge of this scanline segment */
        int left = seed.x;
        while (left > 0 && braille_get_pixel(canvas, left - 1, seed.y) == target_value) {
            left--;
        }
        
        /* Find right edge of this scanline segment */
        int right = seed.x;
        while (right < canvas->pixel_width - 1 && 
               braille_get_pixel(canvas, right + 1, seed.y) == target_value) {
            right++;
        }
        
        /* Fill the entire scanline segment */
        for (int i = left; i <= right; i++) {
            braille_set_pixel(canvas, i, seed.y, fill_value);
        }
        
        /* Scan the line above and below for new segments to fill */
        for (int dy = -1; dy <= 1; dy += 2) {
            int ny = seed.y + dy;
            if (ny < 0 || ny >= canvas->pixel_height) continue;
            
            bool in_segment = false;
            for (int i = left; i <= right; i++) {
                bool is_target = braille_get_pixel(canvas, i, ny) == target_value;
                
                if (is_target && !in_segment) {
                    /* Start of a new segment - add seed point */
                    ENQUEUE(i, ny);
                    in_segment = true;
                } else if (!is_target) {
                    in_segment = false;
                }
            }
        }
    }
    
    #undef ENQUEUE
    #undef DEQUEUE
    
    free(queue);
}

void braille_copy_region(BrailleCanvas *dst, int dx, int dy,
                         BrailleCanvas *src, int sx, int sy, int w, int h) {
    if (!dst || !src) return;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool pixel = braille_get_pixel(src, sx + x, sy + y);
            braille_set_pixel(dst, dx + x, dy + y, pixel);
        }
    }
}
