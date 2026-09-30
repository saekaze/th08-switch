# Touhou 8: Imperishable Night — Nintendo Switch Port
*(東方永夜抄　〜 Imperishable Night)*

![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![Status](https://img.shields.io/badge/Status-Fully%20Playable-brightgreen?style=for-the-badge)
![License](https://img.shields.io/badge/Port%20Code-CC0%201.0-blue?style=for-the-badge)

A native homebrew port of ZUN's 2004 danmaku classic **Touhou 8: Imperishable Night** for the **Nintendo Switch** (Horizon OS).

From 1.00d-r4 this port runs the C++ reconstruction from [YomotsuHisami/th08](https://github.com/YomotsuHisami/th08) (the same author as the TH09/TH11 ports), compiled **natively for ARM64** and driven by an SDL2 host on an OpenGL ES 3 context — no Linux, Box64 or Wine involved. Earlier releases (r1–r3) used the N0zoM1z0/th08 decompilation.

Companion to the [Touhou 6](https://github.com/saekaze/th06-switch), [Touhou 7](https://github.com/saekaze/th07-switch), [Touhou 9](https://github.com/saekaze/th09-switch), [Touhou 10](https://github.com/saekaze/th10-switch) and [Touhou 11](https://github.com/saekaze/th11-switch) Switch ports, with the same `touhou8.nro` + `sd:/switch/touhou/touhou8/` layout.

---

## 🆕 What's New in 1.00d-r4 — a new base, the old bugs are gone

r1–r3 were built on the N0zoM1z0/th08 decompilation, which could get laggy and kept a few rendering bugs that could only be fixed upstream. **r4 moves the whole port to a new base:** YomotsuHisami's TH08 C++ reconstruction (the same author as the Touhou 9 and Touhou 11 ports), compiled natively for ARM64. It is a different code base, so the old decompilation's bugs don't carry over:

* ~~Lag / slowdown in busy scenes~~ — gone with the new base.
* ~~**Background flickering** on one of the stages~~ — gone with the new base.
* ~~**Invisible lasers** (hitbox active, beam not drawn)~~ — gone with the new base.
* ~~Pre-boss dialogue over a solid black background~~ — gone (was fixed in r3, stays fixed).
* ~~Items auto-collected below full power~~ — gone (was fixed in r3, stays fixed).
* ~~Garbled boss-name banner, laser-cancel item burst, respawn animation speed, effect scatter, mirror-barrier warp~~ — gone (were fixed in r3, stay fixed).

Also new in r4:

* 🎯 **Exact PC arithmetic:** the game's x87 floating-point behaviour is reproduced bit-for-bit (SoftFloat), like upstream does for replays and RNG.
* 🎮 **Remappable controls:** the Switch buttons are now TH08's own gamepad, so the in-game **Key Config** works. The defaults are the same layout as before; the D-Pad and sticks only move.
* ⏳ **Loading bar** at startup, 👆 **touch controls** in handheld mode.
* 📁 **One folder for all Touhou ports:** `sd:/switch/touhou/touhou8/` (recommended) — the NRO's own folder is always checked first, so your current setup keeps working.
* 🏷️ **The NRO is now `touhou8.nro`**, matching the other ports. Delete the old `touhou08.nro` when updating so hbmenu doesn't list the game twice. Your `th08.cfg`, `score.dat` and replays keep working.
* The MIDI music option isn't available; music comes from `thbgm.dat`.

---

## ✨ Key Features

* 🎯 **Faithful Arithmetic:** the game's x87 floating-point behaviour is reproduced bit-for-bit with SoftFloat, as upstream does for replay and RNG accuracy.
* ⬛ **OLED-Friendly Pillarboxing:** the original 640×480 picture is centred with pure black (`#000000`) bars and an aspect-correct upscale.
* 🔊 **Full Audio:** sound effects plus BGM streamed straight out of your `thbgm.dat` (original 16-bit PCM and loop points — no conversion step).
* 🎮 **Sane, Remappable Controls:** Joy-Con (handheld, grip, detached) and Pro Controller with the same default layout as every other Touhou Switch port (B shoot, A bomb, L/ZL focus, R/ZR skip, + pause); the in-game **Key Config** can rebind them.
* 👆 **Touch Screen:** in handheld mode, drag to move — upstream's touch controller, mapped through the pillarbox.
* 🈂️ **Japanese Text:** MS Gothic (`msgothic.ttc`) like the PC game, or the Switch's built-in Japanese font as a fallback.
* ⏳ **Loading Bar:** start-up shows its progress while the archive, fonts and first animations are prepared.
* 🌏 **Language-Aware Title:** hbmenu shows `東方永夜抄　～ Imperishable Night` on consoles set to 日本語 and the romanised title everywhere else.
* 💾 **Saves Next to the Data:** `th08.cfg`, `score.dat`, replays (`replay/th8_01.rpy` …) and snapshots are written into the SD folder the game loaded from.

---

## 📥 Installation Guide

> ⚠️ **Disclaimer:** In compliance with ZUN's guidelines and copyright law, this repository contains **ONLY the homebrew engine code**. No game assets are distributed. You must legally own a copy of *Touhou 8: Imperishable Night v1.00d*.

### 1. SD Card File Structure

1. Ensure your Nintendo Switch is running custom firmware (Atmosphère CFW).
2. Download the latest `touhou8.nro` from the [Releases](../../releases) tab (or build from source).
3. Create the folder `sd:/switch/touhou/touhou8/` and copy the following into it:

```text
sd:/switch/touhou/touhou8/
    ├── touhou8.nro           # Nintendo Switch homebrew executable
    ├── th08.dat              # Main game archive (v1.00d)
    ├── thbgm.dat             # Background music archive
    └── msgothic.ttc          # MS Gothic (C:\Windows\Fonts) - recommended
```

`thbgm.dat` is optional (the game runs without music; the MIDI music option is not available). Without `msgothic.ttc` the console's built-in Japanese font is used.

**Recommended place: `sd:/switch/touhou/touhou8/`.** Keeping every Touhou port in one `sd:/switch/touhou/` folder (`touhou6`, `touhou7`, `touhou8` …) is much tidier than a separate folder per game. Other places still work: the port first looks in its own folder (wherever the NRO is), then for a `th08` / `touhou8` folder (any capitalisation) directly on the SD card, in `switch/`, `touhou/`, `switch/touhou/`, `games/` or `roms/`. Saves from r1–r3 (`th08.cfg`, `score.dat`, replays) sit in the same place and keep working.

### 2. Launching

Run `touhou8.nro` from the **Homebrew Menu (hbmenu)**, **Sphaira launcher**, or a home screen forwarder. Title-menu **Quit** returns to hbmenu. If something is missing, the port shows what and where it looked, and `th08-switch.log` is written next to the saves.

---

## 🕹 Controls

| Nintendo Switch Button | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Character Movement |
| **B** | Shoot / Confirm |
| **A** | Bomb / Cancel |
| **L / ZL** | Focus (Precision Slow-Motion Movement) |
| **R / ZR** | Skip Dialogue (hold) |
| **+ (Plus)** | Pause / In-Game Menu |
| **− (Minus)** | Snapshot (saved to `snapshot/` next to the game data) |
| **Touch** (handheld) | Drag to move |

**The same default layout in every Touhou Switch port:** B shoots, A bombs, L/ZL focuses, R/ZR skips dialogue, + pauses. These are only defaults — the Switch buttons act as the game's own gamepad, so the in-game **Key Config** can rebind them, and **Default** there brings this layout back. The D-Pad and sticks only move — they can never be picked as a button. Key Config numbers: 0 B, 1 A, 2 L/ZL, 3 R/ZR, 4 +, 5 X, 6 Y.

---

## 🛠 Building from Source

### Automated Build (GitHub Actions)

`.github/workflows/build-switch.yml` (a copy also lives in `scripts/`) compiles `touhou8.nro` inside the official `devkitpro/devkita64` container and uploads it as an artifact; tagging `v*` also publishes it as a release asset. A second job builds the same sources for Linux and runs the host tests.

### Local Build (Linux / macOS / WSL)

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with `devkitA64` and `libnx`.
2. Install the required Switch portlibs:

   ```bash
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-sdl2 switch-sdl2_ttf switch-freetype switch-libpng switch-bzip2 switch-zlib switch-harfbuzz
   ```

3. Build:

   ```bash
   export DEVKITPRO=/opt/devkitpro
   ./scripts/build_switch.sh
   ```

   The result is `build-switch/touhou8.nro`.

Desktop Linux test build and host tests (same sources; needs `clang libsdl2-dev libsdl2-ttf-dev libfreetype-dev libgles-dev`, and `xvfb` + a CJK font for the full test set):

```bash
CC=clang CXX=clang++ cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
xvfb-run -a ./build/th08_host_tests
./build/touhou8 /path/to/folder/with/th08.dat
```

---

## 📂 How It Works

The game logic is the architecture-independent C++ from upstream (`game/`, `platform/`), compiled natively for AArch64 with the upstream's own code-generation contract (`-ffp-contract=off -fno-strict-aliasing -fno-exceptions -fno-rtti`). See [`upstream/UPSTREAM.md`](upstream/UPSTREAM.md) and the exact [`switch-port.patch`](upstream/switch-port.patch).

| File | Purpose |
| :--- | :--- |
| `game/` | upstream TH08 game logic (stages, ECL/ANM, players, spell cards, menus, replays, …) |
| `platform/` | upstream runtime shell (`BrowserRuntime`: archive, textures, audio manager, fonts) |
| `third_party/softfloat.c` | SoftFloat 3e — the x87 arithmetic |
| `portable/sdl/Renderer.*` | upstream semantic GLES renderer, ported SDL3/WebGL2 → SDL2/GLES 3 |
| `sdl/main.cpp` | Switch main loop, controller, touch, loading bar, snapshots |
| `sdl/FileHost.cpp` | `th08.dat` reader (stdio), SD-card saves |
| `sdl/GraphicsHost.*` | game textures and draws → renderer |
| `sdl/AudioHost.cpp` / `sdl/BgmStream.*` | miniaudio mixer; BGM streamed from `thbgm.dat` with original loop points |
| `sdl/FontHost.cpp` | Japanese text through SDL2_ttf (MS Gothic or system font) |
| `sdl/Platform.*` | SD paths, files, log |
| `platform/switch/icon.jpg` | 256×256 NRO icon (the original game cover) |
| `scripts/build_switch.sh` | one-shot Switch build |
| `scripts/nacp_lang.py` | Japanese NACP title slot |
| `scripts/gen_cp932.py` | CP932 → Unicode table for text |
| `tests/test_host.cpp` | host tests for the port layer |

The game ticks once per vsync (60 Hz); after a real stall it catches up by at most four ticks, like upstream's frame cadence. The CPU is set to the same 1785 MHz boost the other native ports use.

---

## 🤝 Credits & Acknowledgments

* **ZUN / Team Shanghai Alice** — original creator and developer of the Touhou Project series.
* **[YomotsuHisami](https://github.com/YomotsuHisami/th08)** — the TH08 C++ reconstruction this port is built on.
* **KSS** — the TH08 reference code the reconstruction adapts (MIT, see `upstream/licenses/`).
* **[N0zoM1z0](https://github.com/N0zoM1z0/th08)** — the decompilation used by r1–r3 of this port.
* **John R. Hauser / UC Berkeley** — SoftFloat.
* **miniaudio**, **SDL2 / SDL2_ttf**, **FreeType**, **stb_image** — audio mixing, windowing, text, images.
* **Switchbrew & devkitPro Team** — the open-source `libnx` SDK and Switch toolchain.

**Licensing:** the Switch host code (`sdl/main.cpp`, `sdl/Platform.*`, `sdl/BgmStream.*`, `scripts/`, `tests/`, build files) is CC0 1.0 (see `LICENSE`). `game/`, `platform/`, `portable/`, `third_party/` and the adapted `sdl/` hosts are upstream code — see [`upstream/THIRD-PARTY-NOTICES.txt`](upstream/THIRD-PARTY-NOTICES.txt) and [`upstream/licenses/`](upstream/licenses/). Bundled third-party headers keep their own licences (`portable/sdl/third_party/`).
