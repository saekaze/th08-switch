# Upstream

`game/`, `platform/`, `third_party/`, `portable/` and most of `sdl/` come from
**[YomotsuHisami/th08](https://github.com/YomotsuHisami/th08)** (TH08 portable 3.4)
at commit `fa94b0d43b525cef99c43eebb7e53dbb8f9fb588` (2026-09-21):

| Here | Upstream |
| :--- | :--- |
| `game/` | `th08_web/cpp/game/` (`RuntimeExports.cpp` is not built) |
| `platform/` | `th08_web/cpp/platform/` (`BrowserExports.cpp` and `LegacyDevices.cpp` are not built) |
| `third_party/softfloat.c` | `th08_web/cpp/third_party/softfloat.c` |
| `portable/sdl/`, `portable/numeric/`, `portable/input/` | `portable/sdl/`, `portable/numeric/`, `portable/input/` |
| `sdl/AudioHost.cpp`, `FileHost.cpp`, `FontHost.cpp`, `GraphicsHost.*`, `PlatformHost.hpp` | `th08_web/cpp/sdl/` (adapted) |
| `sdl/main.cpp` | replaces `th08_web/cpp/sdl/GameHost.cpp` (browser exports → native loop) |

Every change to those upstream files is in [`switch-port.patch`](switch-port.patch)
(`patch -p1` inside a copy of upstream laid out as above). In short:

* **64-bit (AArch64)** — TH08 upstream already keeps pointers out of 32-bit
  fields; its `static_assert`s on original byte layouts become
  `TH_LAYOUT_ASSERT` (still active on 32-bit builds), and `BrowserTexture`
  carries a real pointer instead of a wasm address.
* **GCC / native link** — the rounding-mode globals SoftFloat exports are
  declared outside the unnamed namespace (GCC otherwise gives them internal
  linkage), and two SoftFloat primitives the amalgamation `#define`d away are
  compiled (a wasm link never needed them).
* **Renderer** — SDL3/WebGL2 → SDL2/GLES 3 (Switch Mesa): window creation,
  `EXT_clip_control` through the GLES loader, aspect-correct pillarboxed
  present, `packed` (a reserved GLSL ES word Mesa rejects) renamed.
* **Host** — `th08.dat` read in place with stdio, saves on the SD card, SDL2
  queued audio with BGM streamed from `thbgm.dat` (MIDI is not available),
  SDL2_ttf text with the Switch system font as fallback, the Switch
  controller as TH08's DirectInput pad, a start-up loading bar.
* Include paths flattened (`../../../portable/` → `../portable/`).

New files: `sdl/main.cpp`, `sdl/Platform.*`, `sdl/BgmStream.*`,
`sdl/MiniaudioConfig.hpp`, `tests/`, `scripts/`.

Game logic, timing, RNG and the replay format are untouched.

When upstream updates: copy the new folders over these and re-apply
`switch-port.patch`.

Licences: see [`THIRD-PARTY-NOTICES.txt`](THIRD-PARTY-NOTICES.txt) and
[`licenses/`](licenses/).
