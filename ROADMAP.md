#  ASCII Dancer Roadmap

Development roadmap and feature tracking for asciidancer.

---

##  Current Status: v2.4+

**Rating: 9.7/10** — Feature-complete visualizer with help overlay and expanded themes

###  Completed Features
- [x] 36 unique poses across 7 energy categories
- [x] Physics-based joint animation (spring-damper system)
- [x] Beat detection with BPM estimation
- [x] Style/genre detection (electronic, rock, hip-hop, ambient, classical)
- [x] Frequency-specific movement (bass→legs, mids→torso, treble→arms)
- [x] Unicode Braille rendering (2×4 subpixel resolution)
- [x] PipeWire + PulseAudio audio backends
- [x] Anti-repetition pose history
- [x] Easing functions (quad, cubic, bounce, elastic)
- [x] INI config file (`~/.config/asciidancer/config.ini`)
- [x] 256-color gradients with 7 themes
- [x] Ground line with shadow/reflection
- [x] Adaptive terminal scaling (SIGWINCH resize)
- [x] Runtime theme cycling (`t` key)
- [x] Particle system with physics
- [x] Motion trails with fading
- [x] Visual enhancements (breathing, floor vibe, shake)
- [x] Control bus architecture with unified signals
- [x] UI reactivity (border pulse, energy meter, beat indicator)
- [x] Knee constraint system (prevents inward collapse)
- [x] Body bounds tracking for particle exclusion
- [x] Enhanced particle repulsion system
- [x] Interactive help overlay (? or F1 key)
- [x] 13 color themes (6 new: aurora, sunset, ocean, candy, vapor, ember)

---

##  v2.1 — Quick Polish (COMPLETED)

**Achieved: 7.5 → 8.5** | Released 2026-01-19

- [x] **Config file (INI format)**
- [x] **256-color gradients** with 7 themes
- [x] **Ground line with shadow**
- [x] **Adaptive terminal scaling**

---

##  v2.2 — Visual Feast (COMPLETED)

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

##  v2.3 — Audio Upgrade ✅

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

##  v2.4 — Polish & UX ✅

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

- [x] **Help overlay** (`?` key) — ✅ Completed in v2.4+
- [ ] **Audio source picker** — Deferred to v2.5
- [ ] **Recording mode** — Deferred to v3.0
- [x] **More themes** (6 new) — ✅ Completed in v2.4+

---

##  v2.4+ — Help & Themes (COMPLETED)

**Achieved: 9.5 → 9.7** | Released 2026-01-18

- [x] **Interactive help overlay**
  - Toggleable with ? or F1 key
  - Shows all keybindings and current settings
  - Smooth fade animation
  - Box-drawing UI with live status

- [x] **6 new color themes** (13 total)
  - Aurora 🌌 (northern lights)
  - Sunset 🌅 (warm gradient)
  - Ocean 🌊 (deep sea)
  - Candy 🍬 (soft pastels)
  - Vapor 📼 (intense vaporwave)
  - Ember 🔴 (glowing coals)

- [x] **Theme system improvements**
  - THEME_COUNT enum for cleaner iteration
  - Better preview descriptions with emojis
  - All use 256-color xterm palette

---

##  v3.0 — Multi-Dancer & Effects

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

##  Future Ideas

- [ ] **MIDI input support** — React to MIDI notes
- [ ] **OSC protocol** — External control interface
- [ ] **Wayland native** — Direct Wayland rendering
- [ ] **Web version** — WASM + Canvas port
- [ ] **Preset poses** — Load custom pose files
- [ ] **Dance battles** — Two dancers, split screen
- [ ] **VJ mode** — Full-screen, no UI elements

---

##  Version History

| Version | Date | Highlights |
|---------|------|------------|
| v2.2.0 | 2026-01-18 | Particles, trails, visual effects |
| v2.1.0 | 2026-01-19 | Config, themes, ground/shadow |
| v2.0.0 | 2026-01-18 | Braille skeleton, physics |
| v1.0.0 | 2026-01-17 | Initial release |
