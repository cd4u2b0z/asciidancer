<- {motion} = {motion} = Original work by Dr. Baklava • github.com/cd4u2b0z • 2026 -->

#  Braille-Boogie v3.2

**A terminal-based audio visualizer with a dancing Unicode Braille character that reacts to music in real-time.**

> ⚠️ **Early Development** — This project is in its infancy and may contain bugs. Contributions, bug reports, and feedback are welcome!

**Latest:** 228 base poses • Genre-specific dance styles • Moonwalk, ballet, breakdance, and more

![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)
![C](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=black)
![PipeWire](https://img.shields.io/badge/PipeWire-4A86CF?style=flat&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=flat)
![Version](https://img.shields.io/badge/Version-3.2.0-blue?style=flat)

![Demo](assets/demo.gif)


> *"The rhythm speaks. The terminal dances. Physics handles the rest."*

---

##  Features

###  Audio-Reactive Animation
- **Real-time frequency analysis** via FFTW3
- **228 base poses** across 13 categories (~1,190 with procedural variations)
- **Physics-based animation** with spring-damper joint system

###  Frequency-Specific Movement
| Band | Range | Body Response |
|------|-------|---------------|
|  **Bass** | 20-300Hz | Legs, hips, ground stomps |
|  **Mids** | 300-2kHz | Torso, head bob, bounce |
|  **Treble** | 2kHz+ | Arms, hands, flourishes |

###  Visual Effects
| Effect | Key | Description |
|--------|-----|-------------|
|  **Particles** | `p` | Sparks shoot from feet on bass hits |
|  **Trails** | `m` | Ghost afterimages follow limb movement |
|  **Breathing** | `b` | Subtle idle animation |

###  Genre Detection & Easter Eggs (v3.2)
Automatic genre detection adapts the dancer's style and triggers special moves:

| Genre | Detection | Easter Egg Moves |
|-------|-----------|------------------|
|  **Electronic/EDM** | High treble, fast BPM | Robot poses (locks, isolations) |
|  **Hip-Hop** | Strong bass, mid-tempo | Moonwalk slides, breakdance freezes |
|  **Rock** | High energy, guitar range | Headbanging, power stances |
|  **Classical** | Low energy, balanced | Ballet positions, waltz frames |
|  **Pop** | Balanced energy, steady beat | Moonwalk, smooth transitions |
|  **Ambient** | Very low energy | Gentle swaying |

Easter egg moves trigger randomly (~15% chance) when a genre is detected.

###  Smart Animation System
- **Beat detection** with BPM estimation
- **Anti-repetition** pose history (avoids recent 8 poses)
- **Smooth interpolation** via easing functions
- **Momentum** and follow-through physics

###  v3.2 Dance Styles
| Style | Poses | Description |
|-------|-------|-------------|
| 🚶 **Moonwalk** | 4 | Smooth backward slides, toe stands |
| 🩰 **Ballet** | 5 | First position, arabesque, plié, relevé |
| 🤸 **Breakdance** | 4 | Toprocks, freezes, power prep |
| 💃 **Waltz** | 4 | Frames, turns, rises, sways |
| 🤖 **Robot** | 5 | Locks, extensions, isolations |
| 🎸 **Headbang** | 4 | Down strokes, horns up, power stance |

###  Audio Analysis
| Feature | Description |
|---------|-------------|
| 🎯 **BPM Tracker** | Multi-tap tempo averaging with confidence + stability |
| 📊 **Energy Analyzer** | RMS energy, intensity zones, spectral features |
| 🌟 **Background FX** | 7 particle effects (ambient, waves, aura, burst, ribbons, rain, vortex) |

###  v3.0 Background Effects
| Effect | Description |
|--------|-------------|
| ✨ **Ambient Field** | Floating twinkling particles |
| 🌊 **Spectral Waves** | Frequency-reactive wave pulses |
| 💫 **Energy Aura** | Pulsing ring around dancer |
| 💥 **Beat Burst** | Explosions synchronized to beats |
| 📊 **Frequency Ribbons** | Vertical frequency bars |
| 🌧️ **Particle Rain** | Falling particles from top |
| 🌀 **Spiral Vortex** | Rotating spiral arms |

---

##  Dependencies

```bash
# Arch Linux
sudo pacman -S fftw libpulse ncurses pipewire pkg-config

# Ubuntu/Debian
sudo apt install libfftw3-dev libpulse-dev libncurses-dev libpipewire-0.3-dev pkg-config

# Fedora
sudo dnf install fftw-devel pulseaudio-libs-devel ncurses-devel pipewire-devel pkg-config
```

---

##  Building

```bash
# Clone the repository
git clone https://github.com/cd4u2b0z/asciidancer.git
cd asciidancer

# Build the braille skeleton dancer (recommended)
make braille

# Or build the frame-based dancer
make
```

###  Generate Demo GIF

Requires [VHS](https://github.com/charmbracelet/vhs) (install with `brew install vhs` or `go install github.com/charmbracelet/vhs@latest`)

```bash
vhs demo.tape
```

This will generate `assets/demo.gif` showcasing the dancer in action.

---

##  Usage

```bash
./asciidancer
```

###  Options
| Flag | Description |
|------|-------------|
| `-s, --source <name>` | Audio source (default: auto) |
| `-p, --pulse` | Use PulseAudio instead of PipeWire |
| `-f, --fps <n>` | Target framerate (default: 60) |
| `-t, --theme <name>` | Color theme |
| `-c, --config <file>` | Custom config file path |
| `--no-ground` | Disable ground line |
| `--no-shadow` | Disable shadow/reflection |
| `--pick-source` | 🆕 Interactive audio source picker |
| `--show-caps` | 🆕 Display terminal capabilities |

###  Runtime Controls

**Core:**
| Key | Action |
|-----|--------|
| `?` / `F1` | Toggle help overlay |
| `q` / `Esc` | Quit |
| `+` / `-` | Adjust sensitivity |
| `t` | Cycle color themes (13 available) |

**Visual Toggles:**
| Key | Action |
|-----|--------|
| `g` | Toggle ground line |
| `r` | Toggle shadow/reflection |
| `p` | Toggle particles |
| `m` | Toggle motion trails |
| `b` | Toggle breathing animation |

**v3.0 Effects:**
| Key | Action |
|-----|--------|
| `f` | Toggle background effects |
| `e` | Cycle background effect types (7 modes) |

**v3.0+ Pro Tools:**
| Key | Action |
|-----|--------|
| `x` | Toggle frame recording (export mode) |
| `i` | Toggle performance profiler overlay |

###  Color Themes (13 Available)

**Original Themes:**
| Theme | Description |
|-------|-------------|
|  `default` | Classic cyan gradient |
|  `fire` | Red/orange/yellow gradient |
|  `ice` | Blue/cyan/white gradient |
|  `neon` | Vibrant magenta/cyan/green |
|  `matrix` | Green on black |
|  `synthwave` | Purple/pink gradient |
|  `mono` | White/gray monochrome |

**v2.4+ New Themes:**
| Theme | Description |
|-------|-------------|
|  `aurora` 🌌 | Northern lights (green→teal→blue→purple) |
|  `sunset` 🌅 | Warm evening sky (orange→pink→purple) |
|  `ocean` 🌊 | Deep sea vibes (navy→teal→aqua→seafoam) |
|  `candy` 🍬 | Soft pastels (pink→mint→lavender) |
|  `vapor` 📼 | Intense vaporwave (hot pink→cyan→purple) |
|  `ember` 🔴 | Glowing coals (dark red→orange→yellow) |

Press `t` to cycle through all themes, or press `?` for the interactive help overlay.

---

##  Configuration

Config file: `~/.config/asciidancer/config.ini`

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
breathing = true

[animation]
fps = 60
```

---

##  Architecture

```
┌─────────────────────────────────────┐
│      PipeWire / PulseAudio          │
│        (Audio Capture Thread)       │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│     cavacore FFT Processing         │
│  ├  256 frequency bins              │
│  └  Low-latency analysis            │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│      Skeleton Dancer Engine         │
│  ├  Beat detection + BPM            │
│  ├  Style/genre classification      │
│  ├  Pose selection (36 poses)       │
│  └  Physics interpolation           │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│       Effects System (v2.2)         │
│  ├  Particle physics simulation     │
│  ├  Motion trail history            │
│  └  Visual enhancements             │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│       Braille Canvas Renderer       │
│  ├  50×52 virtual pixels            │
│  ├  Bézier curves for limbs         │
│  └  25×13 terminal output           │
└─────────────────────────────────────┘
```

###  Braille Character Magic

Unicode Braille (U+2800–U+28FF) provides **2×4 subpixel resolution**:

```
┌───┬───┐     Each cell has 8 dots:
│ 1 │ 4 │     Dots 1-3: left column
├───┼───┤     Dots 4-6: right column
│ 2 │ 5 │     Dots 7-8: bottom row
├───┼───┤     
│ 3 │ 6 │     Encoding: base 0x2800 + bit pattern
├───┼───┤     Example: ⣿ = 0x28FF (all dots)
│ 7 │ 8 │
└───┴───┘
```

---

##  Pose Categories

| Category | Poses | Trigger |
|----------|-------|---------|
|  **IDLE** | 4 | Energy < 0.15 |
|  **CALM** | 5 | Energy 0.15-0.35 |
|  **GROOVE** | 8 | Energy 0.35-0.55 |
|  **ENERGETIC** | 7 | Energy 0.55-0.75 |
|  **INTENSE** | 6 | Energy > 0.75 |
|  **BASS_HIT** | 4 | Strong bass transient |
|  **TREBLE_ACCENT** | 4 | High treble spike |
| 🚶 **MOONWALK** | 4 | Hip-hop/Pop genre detected |
| 🩰 **BALLET** | 5 | Classical genre detected |
| 🤸 **BREAKDANCE** | 4 | Hip-hop genre detected |
| 💃 **WALTZ** | 4 | Classical genre detected |
| 🤖 **ROBOT** | 5 | Electronic genre detected |
| 🎸 **HEADBANG** | 4 | Rock genre detected |

---

##  Project Structure

```
asciidancer/
├  src/
│   ├  main.c              # Entry point, main loop
│   ├  audio/
│   │   ├  pipewire.c      # PipeWire capture
│   │   ├  pulse.c         # PulseAudio capture
│   │   ├  common.c        # Shared audio utilities
│   │   ├  rhythm.c        # Beat detection, BPM
│   │   ├  bpm_tracker.c   # v3.0 Advanced BPM tracking
│   │   ├  energy_analyzer.c # v3.0 Dynamic energy analysis
│   │   └  audio_picker.c  # v3.0+ Audio source picker
│   ├  braille/
│   │   ├  braille_canvas.c # Pixel-to-braille conversion
│   │   ├  braille_dancer.c # Dancer integration
│   │   └  skeleton_dancer.c # Physics & poses (v3.2: 228 poses)
│   ├  effects/
│   │   ├  particles.c     # Particle system
│   │   ├  trails.c        # Motion trails
│   │   ├  effects.c       # Effects manager
│   │   └  background_fx.c # v3.0 Background effects
│   ├  export/
│   │   └  frame_recorder.c # v3.0+ Frame capture for GIF/video
│   ├  control/
│   │   └  control_bus.c   # Unified audio signals
│   ├  ui/
│   │   ├  ui_reactive.c   # Reactive UI elements
│   │   ├  help_overlay.c  # Interactive help
│   │   ├  profiler.c      # v3.0+ Performance profiler
│   │   └  term_caps.c     # v3.0+ Terminal capabilities
│   ├  render/
│   │   ├  render_new.c    # ncurses rendering
│   │   └  colors.c        # 256-color themes
│   ├  config/
│   │   └  config.c        # INI config parser
│   ├  dancer/
│   │   ├  dancer.h        # Dancer API
│   │   └  legacy/         # v3.2: Archived legacy code
│   └  fft/
│       └  cavacore.c      # FFT processing
├  docs/
│   └  V3_FEATURES.md      # v3.0+ Feature guide
├  assets/
│   └  demo.gif            # Demo animation
├  demo.tape               # VHS demo script
├  README.md
├  CHANGELOG.md
├  ROADMAP.md
├  ARCHITECTURE.md
└  Makefile
```

---

##  Acknowledgments

This project stands on the shoulders of giants:

###  [cava](https://github.com/karlstav/cava) by Karl Stavestrand
Audio capture and FFT processing adapted from cava (Console-based Audio Visualizer for ALSA).

###  Inspirations
-  **romanm.ch cat animation** — Fluid terminal animation reference
-  **The ncurses library** — Terminal rendering foundation
-  **FFTW3** — Fast Fourier Transform computation
-  **Unicode Consortium** — Braille character specification

---

##  License

MIT License - See [LICENSE](LICENSE) for details.

---

<div align="center">

**"Code dances when the music plays."**

</div>

---

<sub>Original work by **Dr. Baklava** • [github.com/cd4u2b0z](https://github.com/cd4u2b0z) • 2026</sub>

<- {motion} = {motion} = ZHIuYmFrbGF2YQ== -->
