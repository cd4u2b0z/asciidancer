<- {motion} = Original work by Dr. Baklava • github.com/cd4u2b0z • 2026 -->

# 🕺 ASCII Dancer

**A terminal-based audio visualizer with a dancing Unicode Braille character that reacts to music in real-time.**

![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)
![C](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=black)
![PipeWire](https://img.shields.io/badge/PipeWire-4A86CF?style=flat&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=flat)

```
    ⠀⣀⣀⠀
   ⠀⣿⣿⣿⠀     ♪ ♫
   ⠀⠸⣿⠇⠀      
   ⣴⣿⣿⣿⣦     ⎯⎯∿∿⎯⎯
  ⠀⠻⣿⣿⠟⠀      
   ⠀⢸⣿⡇⠀
   ⠀⢸⣿⡇⠀
  ⠀⢀⣿⠁⣿⡀⠀
```

> *"The rhythm speaks. The terminal dances. Physics handles the rest."*

---

## ✨ Features

### 🎵 Audio-Reactive Animation
- **Real-time frequency analysis** via FFTW3
- **36 unique poses** across 7 energy categories
- **Physics-based animation** with spring-damper joint system

### 🎸 Frequency-Specific Movement
| Frequency Band | Body Response |
|----------------|---------------|
| **Bass** (20-300Hz) | Legs, hips, ground stomps |
| **Mids** (300-2kHz) | Torso, head bob, bounce |
| **Treble** (2kHz+) | Arms, hands, flourishes |

### 🎭 Style Detection
Automatic genre detection adapts the dancer's style:
- **Electronic/EDM** → Precise, mechanical movements
- **Rock** → Aggressive, energetic poses
- **Hip-Hop** → Smooth, flowing transitions
- **Ambient** → Gentle swaying
- **Classical** → Graceful, measured gestures

### 🧠 Smart Animation System
- **Beat detection** with BPM estimation
- **Anti-repetition** pose history (avoids recent 8 poses)
- **Smooth interpolation** via easing functions
- **Momentum** and follow-through physics

---

## 📦 Dependencies

```bash
# Arch Linux
sudo pacman -S fftw libpulse ncurses pipewire pkg-config

# Ubuntu/Debian
sudo apt install libfftw3-dev libpulse-dev libncurses-dev libpipewire-0.3-dev pkg-config

# Fedora
sudo dnf install fftw-devel pulseaudio-libs-devel ncurses-devel pipewire-devel pkg-config
```

---

## 🔧 Building

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

## 🚀 Usage

```bash
./asciidancer
```

### Options
| Flag | Description |
|------|-------------|
| `-s, --source <name>` | Audio source (default: auto) |
| `-p, --pulse` | Use PulseAudio instead of PipeWire |
| `-f, --fps <n>` | Target framerate (default: 60) |
| `-t, --theme <name>` | Color theme (fire, ice, neon, matrix, synthwave, mono) |
| `-c, --config <file>` | Custom config file path |
| `--no-ground` | Disable ground line |
| `--no-shadow` | Disable shadow/reflection |
| `-h, --help` | Show help |

### Controls
| Key | Action |
|-----|--------|
| `q` / `Ctrl+C` | Quit |
| `t` | Cycle color theme |
| `g` | Toggle ground line |
| `r` | Toggle shadow/reflection |

### Configuration

ASCII Dancer loads settings from `~/.config/asciidancer/config.ini`:

```ini
[audio]
source = auto
use_pipewire = true

[visual]
theme = fire
sensitivity = 1.0
show_ground = true
show_shadow = true

[terminal]
target_fps = 60
auto_scale = true

[animation]
smoothing = 0.85
energy_decay = 0.95
```

### Color Themes

| Theme | Colors |
|-------|--------|
| 🎨 **default** | Cyan gradient |
| 🔥 **fire** | Red → Orange → Yellow |
| ❄️ **ice** | Blue → Cyan → White |
| 💜 **neon** | Magenta → Cyan → Green |
| 💚 **matrix** | Green shades |
| 🌆 **synthwave** | Pink → Purple |
| ⬜ **mono** | Grayscale |

---

## 🏗️ Architecture

```
asciidancer/
├── src/
│   ├── main.c                    # Main loop, argument parsing
│   ├── audio/
│   │   ├── audio.h               # Audio data structures
│   │   ├── common.c              # Shared buffer handling
│   │   ├── pipewire.c            # PipeWire capture (from cava)
│   │   └── pulse.c               # PulseAudio fallback (from cava)
│   ├── fft/
│   │   ├── cavacore.h            # FFT processing header
│   │   └── cavacore.c            # FFT analysis (from cava)
│   ├── braille/
│   │   ├── braille_canvas.h/c    # Pixel-to-braille rendering
│   │   ├── skeleton_dancer.h/c   # 36-pose skeleton animation
│   │   └── braille_dancer.c      # Integration layer
│   ├── dancer/                   # Legacy frame-based dancer
│   ├── config/
│   │   ├── config.h/c            # INI config parser
│   └── render/
│       ├── render_new.c          # ncursesw UTF-8 rendering
│       └── colors.h/c            # 256-color theme system
├── Makefile
├── CHANGELOG.md
└── README.md
```

---

## 🎨 How It Works

```
┌─────────────────────────────────────┐
│         PipeWire / PulseAudio       │
│         (Audio Capture Thread)      │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│     cavacore FFT Processing         │
│  ├─ 256 frequency bins              │
│  └─ Low-latency analysis            │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│      Skeleton Dancer Engine         │
│  ├─ Beat detection + BPM            │
│  ├─ Style/genre classification      │
│  ├─ Pose selection (36 poses)       │
│  └─ Physics interpolation           │
└─────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────┐
│       Braille Canvas Renderer       │
│  ├─ 50×52 virtual pixels            │
│  ├─ Bézier curves for limbs         │
│  └─ 25×13 terminal output           │
└─────────────────────────────────────┘
```

### Braille Character Magic

Unicode Braille characters (U+2800–U+28FF) provide **2×4 subpixel resolution** per terminal cell:

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

## 🎭 Pose Categories

| Category | Poses | Trigger |
|----------|-------|---------|
| **IDLE** | 4 | Energy < 0.15 |
| **CALM** | 5 | Energy 0.15-0.35 |
| **GROOVE** | 8 | Energy 0.35-0.55 |
| **ENERGETIC** | 7 | Energy 0.55-0.75 |
| **INTENSE** | 6 | Energy > 0.75 |
| **BASS_HIT** | 4 | Strong bass transient |
| **TREBLE_ACCENT** | 4 | High treble spike |

---

## 🙏 Acknowledgments

This project stands on the shoulders of giants:

### [cava](https://github.com/karlstav/cava) by Karl Stavestrand
The audio capture and FFT processing code is adapted from cava (Console-based Audio Visualizer for ALSA). Cava's elegant approach to PipeWire/PulseAudio integration and its cavacore FFT library made this project possible.

### Inspirations
- **romanm.ch cat animation** — Fluid terminal animation reference
- **The ncurses library** — Terminal rendering foundation
- **FFTW3** — Fast Fourier Transform computation
- **Unicode Consortium** — Braille character specification

---

## 📜 License

MIT License - See [LICENSE](LICENSE) for details.

---

<div align="center">

**"Code dances when the music plays."**

</div>

---

<sub>Original work by **Dr. Baklava** • [github.com/cd4u2b0z](https://github.com/cd4u2b0z) • 2026</sub>

<- {motion} = ZHIuYmFrbGF2YQ== -->
