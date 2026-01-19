# Changelog

All notable changes to ASCII Dancer will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## 🎨 v2.4+ - Help & Themes (January 2026)

### 🆕 New Features
- **Interactive Help Overlay** — Press `?` or `F1` to toggle help screen
  - Shows all keybindings organized by category
  - Live status display (theme, BPM, sensitivity, active effects)
  - Smooth fade-in/fade-out animation
  - Clean box-drawing UI with Unicode characters

- **6 New Color Themes** — Expanded from 7 to 13 total themes
  - **Aurora** 🌌 — Northern lights (green→teal→blue→purple)
  - **Sunset** 🌅 — Warm evening sky (orange→pink→purple)
  - **Ocean** 🌊 — Deep sea vibes (navy→teal→aqua→seafoam)
  - **Candy** 🍬 — Soft pastels (pink→mint→lavender)
  - **Vapor** 📼 — Intense vaporwave (hot pink→cyan→purple)
  - **Ember** 🔴 — Glowing coals (dark red→orange→yellow)

### 🔧 Improvements
- Theme cycling now uses `THEME_COUNT` enum for cleaner iteration
- Better theme preview descriptions with emoji indicators
- Usage text updated to show 13 available themes
- All themes use 256-color xterm palette for smooth gradients

### 📄 Technical
- New files: `src/ui/help_overlay.h`, `src/ui/help_overlay.c`
- Updated: `src/config/config.h` (added 6 theme enums + THEME_COUNT)
- Updated: `src/config/config.c` (theme name parsing)
- Updated: `src/render/colors.c` (6 new theme implementations, ~130 lines each)
- Updated: `src/main.c` (help overlay integration)
- Makefile: Added `help_overlay.c` to V24_SRCS

---

## 󰱒 v2.4 - Polish & UX Pass (January 2026)

### 󱐋 New Modules
- **Control Bus** — Unified audio feature signals with attack/release envelope smoothing
  - Core signals: energy, bass, mid, treble (0-1 normalized)
  - Transient detection: onset signal from energy derivative
  - Beat tracking: beat_phase, beat_hit impulse, on_beat/on_half_beat flags
  - Derived signals: brightness, dynamics, bass_ratio, treble_ratio
  - Silence detection with configurable threshold and debouncing
  - Configurable smoothing presets (FAST, MEDIUM, SLOW, INSTANT)

- **UI Reactivity** — Terminal-safe reactive UI elements using glyph density only
  - Border pulse on beat (4 intensity levels: ─ ━ ▬ █)
  - Energy meter bar with peak hold
  - Beat phase indicator (animated ○◔◑◕●)
  - Mini 3-band spectrum display (bass/mid/treble)
  - BPM display with slow smoothing

### 󰏫 Skeleton Improvements
- **Knee Constraint System** — Prevents knock-kneed look during animation
  - Defines centerline at hip center
  - Left knee constrained left of center, right knee right of center
  - Stance detection from beat_phase: [0-0.5]=left planted, [0.5-1]=right planted
  - Planted leg has strict constraint, swinging leg is relaxed
  - Bounce-back velocity reversal on constraint violation

- **Body Bounds Tracking** — Real-time bounding box for particle exclusion
  - `skeleton_dancer_get_bounds()` — normalized coordinates
  - `skeleton_dancer_get_bounds_pixels()` — pixel coordinates
  - Cached bounds updated every frame

### 󱐋 Particle Enhancements
- **Control Bus Driven Emission** — Particles respond to unified control signals
  - `particles_emit_controlled()` — spawns based on onset + energy
  - Count scales with onset + energy
  - Spread radius scales with energy
  - Velocity scales with onset
  - Lifetime inversely scales with energy (fast decay at high energy)

- **Outward Repulsion** — Particles pushed away from dancer center
  - `particles_set_repulsion()` — configurable repulsion strength
  - Particles actively avoid body center during movement
  - Gentle outward drift for particles near body
  - Default repulsion strength: 60.0

### 󰏫 Technical
- New files: `src/control/control_bus.h`, `src/control/control_bus.c`
- New files: `src/ui/ui_reactive.h`, `src/ui/ui_reactive.c`
- Updated: `src/braille/skeleton_dancer.c`, `src/braille/skeleton_dancer.h`
- Updated: `src/effects/particles.c`, `src/effects/particles.h`
- Makefile: V24_SRCS variable for new source files
- Architecture: Separated concerns — audio → control bus → animation/effects/UI

---

## 󰎔 v2.3 - Audio Upgrade (January 2026)

### 󱐋 New Features
- **Spectral flux onset detection** - more accurate beat detection
- **BPM estimation** - autocorrelation-based tempo tracking
- **Beat phase tracking** - anticipate beats, lock to rhythm
- **Enhanced frequency bands** - sub-bass, low-mid, high-mid

### 󰏫 Improvements  
- Tighter rhythm response in animations
- Beat-locked pose transitions
- Phase-synced modifiers (bounce, sway)
- Reduced smoothing for snappier movements
- BPM display in status bar

---

## [2.2.0] - 2026-01-18

###  Added
-  **Particle System** — Dynamic visual effects
  - Spark particles on bass hits from dancer's feet
  - Physics simulation (velocity, gravity, drag)
  - Particle lifetime with fade out
  - Multiple spawn patterns (burst, fountain, explosion, sparkle)
  - Toggle with `p` key

-  **Motion Trails** — Ghost effect on movement
  - Stores history of joint positions
  - Renders trailing ghost limbs with fading opacity
  - Velocity-based trail intensity
  - Toggle with `m` key

-  **Visual Enhancements**
  - Breathing animation (subtle idle motion)
  - Floor vibration on heavy bass
  - Screen shake on intense bass hits
  - Glow effect on high energy
  - Toggle breathing with `b` key

###  Technical
- New directory: `src/effects/`
- New files: `particles.h/c`, `trails.h/c`, `effects.h/c`
- Added `braille_canvas_render()` call for proper pixel-to-cell conversion
- Joint coordinate conversion (normalized → pixel space)
- Effects automatically trigger from audio analysis

###  Fixed
- Particle rendering now properly converts pixel buffer to braille characters
- Joint positions correctly transformed for effects system

---

## [2.1.0] - 2026-01-19

###  Added
-  **256-Color Theme System** — Rich visual customization
  - 7 built-in themes: Default, Fire, Ice, Neon, Matrix, Synthwave, Mono
  - 10-step color gradients based on energy level
  - Theme cycling with `t` key during playback
  - `--theme <name>` CLI option

-  **Configuration System** — Persistent settings via INI files
  - Auto-loads from `~/.config/asciidancer/config.ini`
  - Sections: [audio], [visual], [terminal], [animation], [debug]
  - `--config <file>` CLI option for custom config paths
  - `config_create_default()` generates sample config

-  **Ground Line & Shadow** — Enhanced visual depth
  - Horizontal ground line at dancer's feet
  - Shadow/reflection effect (inverted, faded dancer below ground)
  - Toggle ground with `g` key, shadow with `r` key
  - `--no-ground` and `--no-shadow` CLI options

-  **Adaptive Terminal Scaling** — Dynamic resize handling
  - SIGWINCH handler for terminal resize detection
  - Automatic canvas rescaling to fit new dimensions
  - Maintains aspect ratio during resize

###  Changed
- Render system refactored for 256-color support
- Main loop updated with config integration
- Help output now shows theme list

###  Technical
- New files: `src/config/config.h`, `src/config/config.c`
- New files: `src/render/colors.h`, `src/render/colors.c`
- Updated: `src/render/render_new.c`, `src/render/render.h`, `src/main.c`
- Uses xterm 256-color palette (color cube + grayscale ramp)

---

## [2.0.0] - 2026-01-18

###  Added
-  **Braille Skeleton Dancer** — Complete rewrite with procedural animation
  - 36 unique poses across 7 energy categories
  - Physics-based joint animation with spring-damper system
  - Smooth interpolation via easing functions (quadratic, cubic, bounce, elastic)
  - Momentum and follow-through for natural movement

-  **Braille Canvas System** — High-resolution terminal graphics
  - Pixel-to-braille conversion (2×4 subpixel resolution per cell)
  - Drawing primitives: lines, circles, filled circles, arcs
  - Quadratic and cubic Bézier curves for smooth limbs
  - Thick line support for body parts

-  **Advanced Audio Analysis**
  - Beat detection with BPM estimation
  - Style/genre classification (electronic, rock, hip-hop, ambient, classical)
  - Frequency-specific body mapping (bass→legs, mids→torso, treble→arms)

-  **Smart Animation System**
  - Anti-repetition pose history (avoids last 8 poses)
  - Energy-based category selection
  - Transient detection for instant reactions
  - Smooth blend between poses

###  Technical
- New files: `src/braille/braille_canvas.h`, `src/braille/braille_canvas.c`
- New files: `src/braille/skeleton_dancer.h`, `src/braille/skeleton_dancer.c`
- New files: `src/braille/braille_dancer.c`
- 15 joint skeleton with hierarchical bone structure

---

## [1.0.0] - 2026-01-17

###  Added
- Initial release
- Frame-based ASCII dancer with 36 poses
- PipeWire and PulseAudio audio capture
- cavacore FFT processing integration
- ncurses terminal rendering
- Basic frequency band analysis (bass, mid, treble)
