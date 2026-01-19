# 󰛤 ASCII Dancer Roadmap

Development roadmap and feature tracking for asciidancer.

---

## 󰐕 Current Status: v3.2.0 󰝚

### 󰄬 Completed Features
- [x] **228 base poses** across 13 categories (~1,190 with variations)
- [x] **6 genre-specific dance styles** (moonwalk, ballet, breakdance, waltz, robot, headbang)
- [x] **Easter egg system** (~15% trigger rate for genre-specific moves)
- [x] **Enhanced genre detection** (electronic, rock, hip-hop, pop, ambient, classical)
- [x] Physics-based joint animation (spring-damper system)
- [x] Beat detection with BPM estimation
- [x] Frequency-specific movement (bass→legs, mids→torso, treble→arms)
- [x] Unicode Braille rendering (2×4 subpixel resolution)
- [x] PipeWire + PulseAudio audio backends
- [x] Anti-repetition pose history (avoids recent 8 poses)
- [x] Easing functions (quad, cubic, bounce, elastic)
- [x] INI config file (`~/.config/asciidancer/config.ini`)
- [x] 256-color gradients with 13 themes
- [x] Ground line with shadow/reflection
- [x] Adaptive terminal scaling (SIGWINCH resize)
- [x] Runtime theme cycling (`t` key)
- [x] Particle system with physics
- [x] Motion trails with fading
- [x] Visual enhancements (breathing, floor vibe)
- [x] Control bus architecture with unified signals
- [x] UI reactivity (border pulse, energy meter, beat indicator)
- [x] Knee constraint system (prevents inward collapse)
- [x] Body bounds tracking for particle exclusion
- [x] Enhanced particle repulsion system
- [x] Interactive help overlay (? or F1 key)
- [x] 13 color themes (aurora, sunset, ocean, candy, vapor, ember, etc.)
- [x] Frame recording/export mode (X key)
- [x] Performance profiler overlay (I key)
- [x] Audio source picker menu (--pick-source)
- [x] Terminal capabilities detection (--show-caps)
- [x] **7 background particle effects** (ambient, waves, aura, burst, ribbons, rain, vortex)

---

## 󰝚 v3.2.0 — Dance Revolution (COMPLETED)

**Achieved: 9.5 → 9.7** | Released 2026-01-19

### 󰛿 New Dance Styles
- [x] **Moonwalk** (4 poses) — Smooth backward slides with toe stands and glides
- [x] **Ballet** (5 poses) — Classical positions including arabesque, plié, relevé
- [x] **Breakdance** (4 poses) — Power moves with toprocks, freezes, prep stances
- [x] **Waltz** (4 poses) — Elegant ballroom frames, turns, rises, sways
- [x] **Robot** (5 poses) — Mechanical locks, isolations, extensions
- [x] **Headbang** (4 poses) — Rock power stances with horns up

### 󰎈 Enhanced Genre Detection
- [x] **Pop style** category added for balanced, steady-beat tracks
- [x] **Easter egg triggers** — Genre-specific moves activate randomly (~15% chance)
- [x] **Improved thresholds** for better electronic, hip-hop, rock, classical detection
- [x] **Genre-pose mapping** — Electronic→Robot, Hip-hop→Moonwalk/Breakdance, etc.

### 󰄧 Pose System Expansion
- [x] **228 base poses** (up from 36 — 6× increase)
- [x] **~1,190 total variations** with procedural mirroring
- [x] **13 pose categories** (added 6 new genre-specific categories)

### 󰧹 Code Quality (v3.2.1)
- [x] **Thread-safe profiler** — `_Atomic` for cross-thread timing
- [x] **Centralized constants** — New `constants.h` with ~150 tuning parameters
- [x] **Scanline flood fill** — Bounded O(4096) memory vs previous O(width*height)

### 󰆍 Build System (v3.2.3)
- [x] **Version strings** — Git version, hash, and build date embedded via $(shell ...)
- [x] **Make targets** — Added help, info, run, debug phony targets
- [x] **Developer workflow** — make run builds and runs, make debug launches gdb

### 󰣇 Platform Support (v3.2.2)
- [x] **macOS CoreAudio backend** — Native audio capture via AudioQueue API
- [x] **Cross-platform Makefile** — Automatic OS detection and framework linking
- [x] **Homebrew support** — Easy installation on macOS

### 󰃢 Code Cleanup
- [x] **Legacy code archived** — Moved 9 unused files (~1,342 lines) to `src/dancer/legacy/`
- [x] **Removed stub features** — Cleaned up non-functional visualizer code
- [x] **API compatibility maintained** — Stub functions retained for integrations

---

## 󰎁 v3.0.1 — Production Tools (COMPLETED)

**Achieved: 9.9 → 9.5 (realistic)** | Released 2026-01-19

### 󰎁 Export & Recording
- [x] Frame recorder with ANSI color preservation
- [x] Timestamped recording directories
- [x] GIF/video export workflow support

### 󰾆 Performance Profiler
- [x] Real-time FPS tracking (current/avg/min/max)
- [x] Component timing breakdown (audio/update/render)
- [x] Particle and trail count monitoring
- [x] Visual performance bar (green/yellow/red zones)

### 󰎈 Audio Source Picker
- [x] PulseAudio/PipeWire source enumeration
- [x] Interactive selection menu
- [x] --pick-source CLI flag

### 󰍹 Terminal Detection
- [x] Sixel graphics support detection
- [x] Kitty protocol detection
- [x] iTerm2 inline image detection
- [x] True color (24-bit) verification
- [x] --show-caps CLI flag

## 󰸞 v3.0 — Big Audio Upgrade (COMPLETED)

**Achieved: 9.7 → 9.9** | Released 2026-01-18

### 󰾆 Advanced BPM Tracker
- [x] Multi-tap tempo averaging with histogram clustering
- [x] Confidence scoring (0-1) for BPM reliability
- [x] Stability tracking over time
- [x] Tempo locking when confident + stable
- [x] Half/double time detection
- [x] 40-240 BPM range (expanded from 60-200)
- [x] Adaptive tracking for gradual tempo changes

### 󰄧 Dynamic Energy Analyzer
- [x] RMS energy calculation
- [x] Peak detection and envelope follower
- [x] 5 intensity zones (Silent, Low, Medium, High, Peak)
- [x] Spectral features (centroid, spread, rolloff)
- [x] Dynamic range analysis (dB)
- [x] Pace intensity metric (BPM + energy)
- [x] Adaptive thresholds based on history
- [x] 6-band energy tracking

### 󱐋 Background Particle Effects
- [x] 7 spectacular effect modes
- [x] Ambient Field (floating twinkling particles)
- [x] Spectral Waves (frequency-reactive pulses)
- [x] Energy Aura (pulsing ring around dancer)
- [x] Beat Burst (synchronized explosions)
- [x] Frequency Ribbons (vertical bars)
- [x] Particle Rain (falling from top)
- [x] Spiral Vortex (rotating arms)
- [x] Intensity and speed control
- [x] Full audio reactivity integration

## 󰏘 v2.1 — Quick Polish (COMPLETED)

**Achieved: 7.5 → 8.5** | Released 2026-01-19

- [x] **Config file (INI format)**
- [x] **256-color gradients** with 7 themes
- [x] **Ground line with shadow**
- [x] **Adaptive terminal scaling**

---

## 󰸞 v2.2 — Visual Feast (COMPLETED)

**Achieved: 8.5 → 9.0** | Released 2026-01-18

- [x] **Braille particle system**
  - Spark particles on bass hits
  - Particle velocity, gravity, lifetime
  - Fade out with decreasing brightness
  - Configurable spawn count/spread

- [x] **Motion trails**
  - Store last N joint positions
  - Draw ghost limbs with decreasing opacity
  - Trail length based on movement speed

- [x] **Visual enhancements**
  - Subtle idle breathing animation
  - Floor "vibration" on heavy bass
  - Glow effect on intense movement

---

## 󰎈 v2.3 — Audio Upgrade (COMPLETED)

**Goal: Tighter rhythm response**

- [x] **Spectral flux onset detection**
  - Calculate spectral difference between frames
  - Detect actual note/beat onsets
  - Reduce false positives from sustained notes

- [x] **Autocorrelation BPM refinement**
  - More accurate BPM calculation
  - Handle tempo changes
  - Sub-beat timing

- [x] **Beat phase tracking**
  - Lock to beat phase, not just detection
  - Anticipate upcoming beats
  - Smoother rhythm synchronization

- [x] **Frequency band improvements**
  - More granular band separation
  - Configurable crossover frequencies

---

## 󱓻 v2.4 — Polish & UX (COMPLETED)

**Achieved: 9.0 → 9.5** | Released 2026-01-18

- [x] **Control Bus architecture**
  - Unified audio feature signals
  - Attack/release envelope smoothing
  - Derived signals (brightness, dynamics)
  - Silence detection

- [x] **UI Reactivity**
  - Border pulse on beat
  - Energy meter with peak hold
  - Beat phase indicator animation
  - Mini spectrum display

- [x] **Skeleton improvements**
  - Knee constraint system (prevents knock-knees)
  - Body bounds tracking for particle exclusion
  - Beat-phase-based stance detection

- [x] **Particle enhancements**
  - Control bus driven emission
  - Outward repulsion from body center
  - Dynamic spawn parameters based on audio

- [x] **Help overlay** (`?` key) — Completed in v2.4+
- [ ] **Audio source picker** — Deferred to v2.5
- [ ] **Recording mode** — Deferred to v3.0
- [x] **More themes** (6 new) — Completed in v2.4+

---

## 󰌌 v2.4+ — Help & Themes (COMPLETED)

**Achieved: 9.5 → 9.7** | Released 2026-01-18

- [x] **Interactive help overlay**
  - Toggleable with ? or F1 key
  - Shows all keybindings and current settings
  - Smooth fade animation
  - Box-drawing UI with live status

- [x] **6 new color themes** (13 total)
  - 󱝂 Aurora (northern lights)
  - 󰖛 Sunset (warm gradient)
  - 󰘸 Ocean (deep sea)
  - 󰄛 Candy (soft pastels)
  - 󱥒 Vapor (intense vaporwave)
  - 󰈸 Ember (glowing coals)

- [x] **Theme system improvements**
  - THEME_COUNT enum for cleaner iteration
  - Better preview descriptions
  - All use 256-color xterm palette

---

## 󰐕 v4.0 — Multi-Dancer & Effects (PLANNED)

**Goal: Visual spectacle**

- [ ] **Multiple dancers**
  - 2-4 dancers with different styles
  - Synchronized or independent movement

- [ ] **Background effects**
  - Starfield that pulses to beat
  - Gradient backgrounds

- [ ] **Text overlays**
  - Now playing info (if available)
  - BPM display
  - Energy meters

---

## 󰆃 Future Ideas

- [ ] **MIDI input support** — React to MIDI notes
- [ ] **OSC protocol** — External control interface
- [ ] **Wayland native** — Direct Wayland rendering
- [ ] **Web version** — WASM + Canvas port
- [ ] **Preset poses** — Load custom pose files
- [ ] **Dance battles** — Two dancers, split screen
- [ ] **VJ mode** — Full-screen, no UI elements

---

## 󰔏 Version History

| Version | Date | Highlights |
|---------|------|------------|
| v3.2.0 | 2026-01-19 | 228 poses, genre-specific dance styles, code quality |
| v3.0.1 | 2026-01-19 | Production tools, profiler, export |
| v3.0.0 | 2026-01-18 | BPM tracker, energy analyzer, background FX |
| v2.4.0 | 2026-01-18 | Control bus, UI reactivity, help overlay |
| v2.2.0 | 2026-01-18 | Particles, trails, visual effects |
| v2.1.0 | 2026-01-19 | Config, themes, ground/shadow |
| v2.0.0 | 2026-01-18 | Braille skeleton, physics |
| v1.0.0 | 2026-01-17 | Initial release |
