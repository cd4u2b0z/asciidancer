# 󰙵 Architecture

Technical architecture documentation for ASCII Dancer v2.2.

---

## 󰈏 Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                        ASCII Dancer v2.2                          │
├──────────────────────────────────────────────────────────────────┤
│  Audio Layer      FFT Layer       Animation       Render Layer   │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐   │
│  │ PipeWire │───>│ cavacore │───>│ Skeleton │───>│  Braille │   │
│  │  Pulse   │    │   FFT    │    │  Dancer  │    │  Canvas  │   │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘   │
│        │               │               │               │          │
│        └───────────────┴───────────────┴───────────────┘          │
│                              │                                    │
│                    ┌─────────┴─────────┐                         │
│                    │   Effects System   │                         │
│                    │  Particles/Trails  │                         │
│                    └───────────────────┘                         │
└──────────────────────────────────────────────────────────────────┘
```

---

## 󰉋 Directory Structure

```
src/
├── 󰈮 main.c                 # Entry point, main loop
│
├── 󰎈 audio/                 # Audio capture
│   ├── pipewire.c          # PipeWire stream
│   ├── pulse.c             # PulseAudio capture
│   └── common.c            # Shared utilities
│
├── 󰓾 fft/                   # Frequency analysis
│   └── cavacore.c          # FFT (from cava)
│
├── 󰕮 braille/               # Rendering
│   ├── braille_canvas.c    # Pixel→braille
│   ├── braille_dancer.c    # Interface
│   └── skeleton_dancer.c   # Physics/poses
│
├── 󱐋 effects/               # Visual effects
│   ├── particles.c         # Particle system
│   ├── trails.c            # Motion trails
│   └── effects.c           # Manager
│
├── 󰍹 render/                # Terminal output
│   ├── render_new.c        # ncurses
│   └── colors.c            # 256-color themes
│
└── 󰒓 config/                # Configuration
    └── config.c            # INI parser
```

---

## 󰔏 Audio Pipeline

**Thread Model:**
```
┌─────────────────┐     ┌─────────────────┐
│   Main Thread   │     │  Audio Thread   │
│  - FFT process  │◄────│  - PW/PA stream │
│  - Animation    │     │  - Sample write │
│  - Rendering    │     └─────────────────┘
└─────────────────┘
```

**Flow:** Audio → Ring Buffer → FFT → 256 bins → bass/mid/treble

---

## 󰚹 Skeleton System

**15 Joints:**
```
          HEAD
           │
    SHOULDER─┼─SHOULDER
        │  SPINE  │
      ELBOW   │   ELBOW
        │     │     │
      HAND  HIP   HAND
          ┌─┼─┐
        HIP   HIP
         │     │
       KNEE   KNEE
         │     │
       FOOT   FOOT
```

**Physics:** Spring-damper per joint
- Stiffness: 0.1-0.5
- Damping: 0.7-0.9

**36 Poses** across 7 categories:
| Category | Energy Range | Count |
|----------|--------------|-------|
| IDLE | 0.00-0.15 | 4 |
| CALM | 0.15-0.35 | 5 |
| GROOVE | 0.35-0.55 | 8 |
| ENERGETIC | 0.55-0.75 | 7 |
| INTENSE | 0.75-1.00 | 6 |
| BASS_HIT | Transient | 4 |
| TREBLE_ACCENT | Transient | 4 |

---

## 󰕮 Braille Canvas

**Resolution:** 50×52 pixels → 25×13 terminal cells

**Encoding:** Unicode U+2800-U+28FF (8 dots per char)
```
┌─┬─┐
│1│4│  Bits: 0x01, 0x08
├─┼─┤
│2│5│  Bits: 0x02, 0x10
├─┼─┤
│3│6│  Bits: 0x04, 0x20
├─┼─┤
│7│8│  Bits: 0x40, 0x80
└─┴─┘
```

**Drawing Primitives:**
- `braille_draw_line()` — Bresenham's algorithm
- `braille_draw_circle()` — Midpoint circle
- `braille_draw_bezier_quad()` — Quadratic curves
- `braille_draw_bezier_cubic()` — Cubic curves

---

## 󱐋 Effects System (v2.2)

### 󰸞 Particles
- Pool of 256 particles (no allocations)
- Physics: velocity, gravity, drag
- Spawn patterns: burst, fountain, explosion, sparkle
- Fade based on lifetime

```c
// Particle update
p->vx += p->ax * dt;
p->vy += p->ay * dt;
p->x += p->vx * dt;
p->y += p->vy * dt;
p->brightness = p->lifetime / p->max_life;
```

### 󰘵 Motion Trails
- Ring buffer per joint (8 positions)
- Velocity-based recording
- Alpha fade over time

### 󰓾 Coordinate Transform
```c
// Joint (0-1) → Pixel coordinates
px = (joint.x - 0.5) * (width * 0.8) + (width / 2);
py = joint.y * (height * 0.8) + (height * 0.1);
```

---

## 󰓾 Render Pipeline

```
1. Audio capture (thread)
        ↓
2. FFT → frequency bins
        ↓
3. bass/mid/treble calculation
        ↓
4. Skeleton physics update
        ↓
5. Effects triggers (bass→particles, etc.)
        ↓
6. Canvas clear
        ↓
7. Render trails → canvas
        ↓
8. Render skeleton → canvas
        ↓
9. Render particles → canvas
        ↓
10. braille_canvas_render() → convert pixels to cells
        ↓
11. braille_canvas_to_utf8() → UTF-8 string
        ↓
12. ncurses mvprintw() → terminal
```

---

## 󰾆 Performance

- **60 FPS** target (~16.6ms per frame)
- **Particle pooling** — fixed array, no malloc
- **Dirty cell tracking** — skip unchanged cells
- **8× compression** — braille packs 8 pixels/char
- **Single-threaded render** — simplicity over parallelism

---

## 󰒓 Configuration

**File:** `~/.config/asciidancer/config.ini`

```ini
[audio]
source = auto
backend = pipewire
sensitivity = 1.0

[visual]
theme = matrix
ground = true
shadow = true
particles = true
trails = true

[animation]
fps = 60
```

---

## 󰘦 Key Algorithms

### Beat Detection
```c
if (bass > threshold && bass_velocity > min_velocity) {
    trigger_bass_hit();
}
```

### Pose Selection
```c
// Avoid recent poses, select from energy category
Category cat = energy_to_category(energy);
do {
    pose = random_from_category(cat);
} while (in_recent_history(pose));
```

### Spring-Damper Physics
```c
force = (target - position) * stiffness;
velocity += force;
velocity *= damping;
position += velocity * dt;
```
