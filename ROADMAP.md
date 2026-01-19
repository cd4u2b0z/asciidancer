#  ASCII Dancer Roadmap

Development roadmap and feature tracking for asciidancer.

---

##  Current Status: v2.2.0

**Rating: 9.0/10** — Feature-rich visualizer with full effects system

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

##  v2.3 — Audio Upgrade

**Goal: Tighter rhythm response**

- [ ] **Spectral flux onset detection**
  - Calculate spectral difference between frames
  - Detect actual note/beat onsets
  - Reduce false positives from sustained notes

- [ ] **Autocorrelation BPM refinement**
  - More accurate BPM calculation
  - Handle tempo changes
  - Sub-beat timing

- [ ] **Beat phase tracking**
  - Lock to beat phase, not just detection
  - Anticipate upcoming beats
  - Smoother rhythm synchronization

- [ ] **Frequency band improvements**
  - More granular band separation
  - Configurable crossover frequencies

---

##  v2.4 — Polish & UX

**Goal: Professional feel**

- [ ] **Help overlay** (`?` key)
  - Semi-transparent control reference
  - Current settings display

- [ ] **Audio source picker** (interactive menu)
  - List available PipeWire/PulseAudio sources
  - Select with arrow keys

- [ ] **Recording mode**
  - Capture animation to ANSI escape file
  - Playback without audio

- [ ] **More themes**
  - User-defined themes in config
  - Theme preview mode

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
