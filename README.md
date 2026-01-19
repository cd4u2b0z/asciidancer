<- {motion} = {motion} = Original work by Dr. Baklava • github.com/cd4u2b0z • 2026 -->

#  ASCII Dancer

**A terminal-based audio visualizer with a dancing Unicode Braille character that reacts to music in real-time.**

![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)
![C](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=black)
![PipeWire](https://img.shields.io/badge/PipeWire-4A86CF?style=flat&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=flat)

```
    ⠀⣀⣀⠀
   ⠀⣿⣿⣿⠀      
   ⠀⠸⣿⠇⠀      
   ⣴⣿⣿⣿⣦     
  ⠀⠻⣿⣿⠟⠀      
   ⠀⢸⣿⡇⠀
   ⠀⢸⣿⡇⠀
  ⠀⢀⣿⠁⣿⡀⠀
```

> *"The rhythm speaks. The terminal dances. Physics handles the rest."*

---

##  Features

###  Audio-Reactive Animation
- **Real-time frequency analysis** via FFTW3
- **36 unique poses** across 7 energy categories
- **Physics-based animation** with spring-damper joint system

###  Frequency-Specific Movement
| Band | Range | Body Response |
|------|-------|---------------|
|  **Bass** | 20-300Hz | Legs, hips, ground stomps |
|  **Mids** | 300-2kHz | Torso, head bob, bounce |
|  **Treble** | 2kHz+ | Arms, hands, flourishes |

###  Visual Effects (v2.2)
| Effect | Key | Description |
|--------|-----|-------------|
|  **Particles** | `p` | Sparks shoot from feet on bass hits |
|  **Trails** | `m` | Ghost afterimages follow limb movement |
|  **Breathing** | `b` | Subtle idle animation |

###  Style Detection
Automatic genre detection adapts the dancer's style:
-  **Electronic/EDM** — Precise, mechanical movements
-  **Rock** — Aggressive, energetic poses
-  **Hip-Hop** — Smooth, flowing transitions
-  **Ambient** — Gentle swaying
-  **Classical** — Graceful, measured gestures

###  Smart Animation System
- **Beat detection** with BPM estimation
- **Anti-repetition** pose history (avoids recent 8 poses)
- **Smooth interpolation** via easing functions
- **Momentum** and follow-through physics

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

###  Runtime Controls
| Key | Action |
|-----|--------|
| `q` / `Esc` | Quit |
| `+` / `-` | Adjust sensitivity |
| `t` | Cycle color themes |
| `g` | Toggle ground line |
| `r` | Toggle shadow/reflection |
| `p` | Toggle particles |
| `m` | Toggle motion trails |
| `b` | Toggle breathing animation |

###  Color Themes
| Theme | Description |
|-------|-------------|
|  `default` | Classic green terminal |
|  `fire` | Red/orange/yellow gradient |
|  `ice` | Blue/cyan/white gradient |
|  `neon` | Vibrant pink/purple/cyan |
|  `matrix` | Green on black |
|  `synthwave` | Purple/magenta/cyan |
|  `mono` | White/gray monochrome |

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

---

##  Project Structure

```
asciidancer/
├  src/
│   ├  main.c              # Entry point, main loop
│   ├  audio/
│   │   ├  pipewire.c      # PipeWire capture
│   │   ├  pulse.c         # PulseAudio capture
│   │   └  common.c        # Shared audio utilities
│   ├  braille/
│   │   ├  braille_canvas.c # Pixel-to-braille conversion
│   │   ├  braille_dancer.c # Dancer integration
│   │   └  skeleton_dancer.c # Physics & poses
│   ├  effects/
│   │   ├  particles.c     # Particle system
│   │   ├  trails.c        # Motion trails
│   │   └  effects.c       # Effects manager
│   ├  render/
│   │   ├  render_new.c    # ncurses rendering
│   │   └  colors.c        # 256-color themes
│   ├  config/
│   │   └  config.c        # INI config parser
│   └  fft/
│       └  cavacore.c      # FFT processing
├  README.md
├  CHANGELOG.md
├  ROADMAP.md
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
