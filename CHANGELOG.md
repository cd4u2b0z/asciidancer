# Changelog

All notable changes to ASCII Dancer will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 󰆍 v3.2.3 - Build System Improvements (January 2026)

### 󰆍 Makefile Enhancements
- **Version embedding** — VERSION, GIT_HASH, BUILD_DATE via $(shell ...) in binary
- **make help** — Shows all available build targets
- **make info** — Displays build configuration (OS, compiler, audio backends)
- **make run** — Build braille dancer and run immediately
- **make debug** — Build with -O0 -DDEBUG and launch in gdb

### 󰣇 macOS Support (v3.2.2)
- **CoreAudio backend** — Native audio capture via AudioQueue API
- **Cross-platform Makefile** — Automatic Darwin/Linux detection
- **Homebrew paths** — Apple Silicon and Intel ncurses support

### 󰧹 Code Quality (v3.2.1)
- **Thread-safe profiler** — _Atomic for cross-thread timing
- **Centralized constants** — constants.h with ~150 tuning parameters
- **Scanline flood fill** — Bounded O(4096) memory allocation

---

## 💃 v3.2.0 - Dance Revolution (January 2026)

### 🕺 New Dance Styles
- **Moonwalk** — 4 smooth backward slides with toe stands and glides
- **Ballet** — 5 classical poses including arabesque, plié, and relevé
- **Breakdance** — 4 power moves with toprocks, freezes, and prep stances
- **Waltz** — 4 elegant ballroom positions with frames and turns
- **Robot** — 5 mechanical movements with locks, isolations, and extensions
- **Headbang** — 4 rock poses with power stances and horns up

### 🎵 Enhanced Genre Detection
- **Pop style** — New music style category for balanced, steady-beat tracks
- **Easter egg triggers** — Special genre-specific moves activate randomly (~15% chance)
- **Improved thresholds** — Better detection for electronic, hip-hop, rock, classical music
- **Genre-pose mapping** — Electronic → Robot, Hip-hop → Moonwalk/Breakdance, Classical → Ballet/Waltz, Rock → Headbang

### 📊 Pose System Expansion
- **228 base poses** — Up from 36 poses (6× increase)
- **~1,190 total variations** — Including procedural mirroring and tweaks
- **13 pose categories** — Added 6 new genre-specific categories
- **Anti-repetition** — Still avoids recent 8 poses for variety

### 🧹 Code Cleanup
- **Legacy code archived** — Moved 9 unused dancer files (~1,342 lines) to `src/dancer/legacy/`
- **Removed stub features** — Cleaned up non-functional visualizer code
- **Maintained API compatibility** — Stub functions retained for existing integrations

### 📄 Technical
- Modified: `src/braille/skeleton_dancer.h` — Added 6 pose categories, STYLE_POP enum
- Modified: `src/braille/skeleton_dancer.c` — Added 26 new base poses, enhanced genre detection
- Modified: `src/braille/braille_dancer.c` — Removed visualizer implementation
- Archived: 9 legacy files → `src/dancer/legacy/`

---

> ⚠️ **Note:** This project is in early development and may contain bugs.

---

## 🛠️ v3.0.1 - Production Tools (January 2026)

### 🎬 Frame Recording & Export
- **Frame recorder** — Capture terminal frames to timestamped directories
- **ANSI text export** — Preserve colors for post-processing
- **GIF/video workflow** — Compatible with asciinema, agg, or vhs tools
- Press `x` to start/stop recording

### 📊 Performance Profiler
- **Real-time FPS display** — Current, average, min, max
- **Component timing** — Audio, update, render breakdown in ms
- **Particle/trail counts** — Live object monitoring
- **Visual performance bar** — Green/yellow/red zone indicators
- Press `i` to toggle profiler overlay

### 🎵 Audio Source Picker
- **Interactive menu** — Arrow keys to select audio source
- **PulseAudio/PipeWire** — Enumerate available sources
- Launch with `--pick-source` flag

### 🖥️ Terminal Capabilities Detection
- **Sixel graphics** — Detect bitmap rendering support
- **Kitty protocol** — Check for Kitty terminal graphics
- **iTerm2 inline** — Detect iTerm2 image protocol
- **True color** — Verify 24-bit color support
- Launch with `--show-caps` flag

### 📝 New Controls
| Key | Action |
|-----|--------|
| `f` | Toggle background effects |
| `e` | Cycle effect types (7 modes) |
| `x` | Toggle frame recording |
| `i` | Toggle performance profiler |

### 📝 New CLI Flags
| Flag | Description |
|------|-------------|
| `--pick-source` | Interactive audio source picker |
| `--show-caps` | Display terminal capabilities |

### 📄 Technical
- New files: `src/export/frame_recorder.h/.c` (~200 lines)
- New files: `src/audio/audio_picker.h/.c` (~180 lines)
- New files: `src/ui/term_caps.h/.c` (~120 lines)
- New files: `src/ui/profiler.h/.c` (~180 lines)
- Updated: `src/main.c` (v3.0+ integration)
- Updated: `Makefile` (V30P_SRCS variable)
- Documentation: `docs/V3_FEATURES.md` comprehensive guide
- Total new code: ~680 lines

---

## 🚀 v3.0 - Big Audio Upgrade (January 2026)

### 🎯 Advanced BPM Tracker
- **Multi-tap tempo averaging** — Analyzes multiple beat intervals with histogram clustering
- **Confidence scoring (0-1)** — Shows how reliable the BPM estimate is
- **Stability tracking** — Measures tempo consistency over time
- **Tempo locking** — High confidence + high stability = locked tempo
- **Half/double time detection** — Identifies alternative tempo interpretations
- **40-240 BPM range** — Wider than original 60-200 range
- **Adaptive tracking** — Handles gradual tempo changes smoothly

### 📊 Dynamic Energy Analyzer
- **RMS energy calculation** — Root mean square power measurement
- **Peak detection** — Maximum amplitude tracking
- **Intensity zones** — Silent, Low, Medium, High, Peak classifications
- **Envelope follower** — Attack/release smoothing for smooth visualization
- **Spectral features** — Centroid (brightness), spread, rolloff analysis
- **Dynamic range analysis** — Peak vs RMS in dB
- **Pace intensity** — Combined BPM + energy metric
- **Adaptive thresholds** — Self-adjusting zone boundaries based on history
- **6-band energy tracking** — Sub-bass through treble

### 🌟 Spectacular Background Effects
7 particle-based background effect modes:
- ✨ **Ambient Field** — Floating twinkling particles
- 🌊 **Spectral Waves** — Frequency-reactive wave pulses from bottom
- 💫 **Energy Aura** — Pulsing ring around dancer
- 💥 **Beat Burst** — Explosions synchronized to beats (with cooldown)
- 📊 **Frequency Ribbons** — Vertical frequency bars
- 🌧️ **Particle Rain** — Falling particles from top
- 🌀 **Spiral Vortex** — Rotating spiral arms

All effects feature:
- Intensity adjustment (0-1)
- Speed control
- Full audio reactivity (frequency bands, energy, beats)
- Integration with existing 256-particle system

### 📄 Technical
- New files: `src/audio/bpm_tracker.h`, `src/audio/bpm_tracker.c` (~270 lines)
- New files: `src/audio/energy_analyzer.h`, `src/audio/energy_analyzer.c` (~380 lines)
- New files: `src/effects/background_fx.h`, `src/effects/background_fx.c` (~460 lines)
- Updated: `Makefile` (V30_SRCS variable for new modules)
- Total new code: ~1,540 lines

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
