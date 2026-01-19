# 🗺️ ASCII Dancer Roadmap

Development roadmap and feature tracking for asciidancer.

---

## 📊 Current Status: v2.0.0

**Rating: 7.5/10** — Solid terminal art with real physics animation

### ✅ Completed Features
- [x] 36 unique poses across 7 energy categories
- [x] Physics-based joint animation (spring-damper system)
- [x] Beat detection with BPM estimation
- [x] Style/genre detection (electronic, rock, hip-hop, ambient, classical)
- [x] Frequency-specific movement (bass→legs, mids→torso, treble→arms)
- [x] Unicode Braille rendering (2×4 subpixel resolution)
- [x] PipeWire + PulseAudio audio backends
- [x] Anti-repetition pose history
- [x] Easing functions (quad, cubic, bounce, elastic)

---

## 🎯 v2.1 — Quick Polish

**Goal: 7.5 → 8.5** | Low effort, high impact

- [ ] **Config file (INI format)**
  - Custom poses path
  - Color scheme selection
  - Sensitivity/gain curves
  - FPS target
  - Audio source default

- [ ] **256-color gradients**
  - Energy → color mapping
  - Multiple preset themes (fire, ice, neon, monochrome)
  - Smooth color transitions

- [ ] **Ground line with shadow**
  - Horizontal floor line
  - Dimmed reflection/shadow of dancer below
  - Shadow sways with movement

- [ ] **Adaptive terminal scaling**
  - Detect terminal size with `getmaxyx()`
  - Scale skeleton coordinates proportionally
  - Handle resize events (SIGWINCH)

---

## 🎨 v2.2 — Visual Feast

**Goal: 8.5 → 9.0** | Medium effort, high visual impact

- [ ] **Braille particle system**
  - Spark particles on bass hits
  - Particle velocity, gravity, lifetime
  - Fade out with decreasing brightness
  - Configurable spawn count/spread

- [ ] **Motion trails**
  - Store last N joint positions
  - Draw ghost limbs with decreasing opacity
  - Trail length based on movement speed

- [ ] **Color themes**
  - `fire` — Red/orange/yellow gradient
  - `ice` — Blue/cyan/white gradient
  - `neon` — Magenta/cyan/green
  - `mono` — White/gray
  - `matrix` — Green shades
  - `synthwave` — Pink/purple/cyan

- [ ] **Visual enhancements**
  - Subtle idle breathing animation
  - Floor "vibration" on heavy bass
  - Glow effect on intense movement (double-draw offset)

---

## 🎵 v2.3 — Audio Upgrade

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
  - Per-band sensitivity curves

---

## 🚀 v3.0 — Major Features (Future)

**Goal: 9.0 → 10** | High effort, transformative

- [ ] **Sixel/Kitty graphics protocol (optional backend)**
  - 10× resolution improvement
  - True 24-bit color support
  - Anti-aliased limb rendering
  - Graceful fallback to braille

- [ ] **Mirror mode / multiple dancers**
  - Side-by-side mirrored dancer
  - Multiple independent dancers
  - Configurable layout

- [ ] **Recording/export**
  - asciinema integration
  - GIF export via ffmpeg
  - Frame-by-frame capture mode

- [ ] **Interactive features**
  - Runtime sensitivity adjustment (↑/↓ keys)
  - Theme cycling (t key)
  - Pause/freeze frame (space)
  - Debug overlay toggle (d key)

---

## 🐛 Known Issues

- [ ] High CPU on very fast music (>180 BPM)
- [ ] Occasional pose "snap" on style change
- [ ] Some terminals render braille with gaps

---

## 💡 Ideas (Maybe Someday)

- [ ] Audio file playback mode (not just live capture)
- [ ] Lyrics display integration (synchronized)
- [ ] Network mode (visualize remote audio stream)
- [ ] Multiple character styles (robot, human, abstract)
- [ ] Plugin system for custom pose generators
- [ ] WebSocket output for browser visualization

---

## 📈 Progress Tracking

| Version | Status | Rating Target |
|---------|--------|---------------|
| v2.0.0 | ✅ Released | 7.5 |
| v2.1.0 | 🔄 In Progress | 8.5 |
| v2.2.0 | 📋 Planned | 9.0 |
| v2.3.0 | 📋 Planned | 9.0+ |
| v3.0.0 | 💭 Future | 10 |

---

<sub>Last updated: 2026-01-18</sub>
