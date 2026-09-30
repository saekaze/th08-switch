<img width="256" height="253" alt="Touhou 8" src="https://github.com/user-attachments/assets/57d8c201-1bc7-4ac9-af62-d41f41f9c7e0" />

## Touhou 8 — 1.00d-r4: new base, old bugs gone

r1–r3 were built on the N0zoM1z0/th08 decompilation. **r4 moves the whole port to a new base:** YomotsuHisami's TH08 C++ reconstruction (the same author as the Touhou 9 and 11 ports), compiled natively for ARM64. The old decompilation's bugs don't carry over:

* ~~Lag / slowdown in busy scenes~~ — gone.
* ~~Background flickering on one of the stages~~ — gone.
* ~~Invisible lasers (hitbox active, beam not drawn)~~ — gone.
* ~~Black background behind pre-boss dialogue, item auto-collect below full power, garbled boss-name banner, laser-cancel item burst, respawn animation speed, effect scatter, mirror-barrier warp~~ — gone (fixed in r3, stay fixed).

Also new:

* 🎯 Exact PC arithmetic (x87 behaviour reproduced with SoftFloat) for faithful replays and RNG.
* 🎮 **Remappable controls:** the in-game Key Config works with the Switch buttons. Same default layout as before; the D-Pad and sticks only move.
* ⏳ Loading bar at startup, 👆 touch controls in handheld mode.
* 📁 Recommended folder: `sd:/switch/touhou/touhou8/` — keep every Touhou port in one `sd:/switch/touhou/` folder instead of separate folders. The NRO's own folder is always checked first, and the old locations still work.
* 🏷️ **The NRO is now `touhou8.nro`.** Delete the old `touhou08.nro` when updating. `th08.cfg`, `score.dat` and replays keep working.
* The MIDI music option isn't available; music comes from `thbgm.dat`.
