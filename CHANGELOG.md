# Changelog — Touhou 8 Switch port

## 1.00d-r4 — 2026-09-30

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

## 1.00d-r3 — 2026-09-13

Upstream sync of the vendored decompilation gameplay fixes: from snapshot
`50077aca` (2026-08-30, `v0.2.0-linux-64bit`) to the gameplay-relevant part of
`a45e99fb` (`N0zoM1z0/th08`, branch `port/portable-64bit`, 2026-09-11).
Upstream has no tagged release newer than `v0.2.0`, so these were hand-ported
into the port tree instead of a plain merge — a full `git cherry-pick` of the
upstream commits conflicts on `config/match-units.toml`, `docs/`, `scripts/` and
on the game files the port guards with `TH08_PORTABLE_NATIVE_LAYOUT`.

### Fixed

| # | Symptom | Root cause | Upstream commit | File |
| --- | --- | --- | --- | --- |
| 1 | Items past the point-of-collection line were auto-attracted at any power | gate read `GetPower() >= 0.0` instead of `>= 128.0` | `7148a76` | `src/ItemManager.cpp` |
| 2 | Pre-boss dialogue played over a solid flat background; sprites left after-image trails | the three Background draw gates used `Gui::IsDialoguePresent()` where the original uses `Gui::IsStageFinished()` (runtime ledger RT-006) | `af72ca9` | `src/Background.cpp` (3 gates) |
| 3 | Boss-name banner read sprite indices 16…27 from a 6-sprite ANM (garbled name, OOB read; crash on native VC7) | `Gui::CopyEnemyNameTexture` used `stageTextAnm` instead of `frontAnm` (33 sprites) (RT-002) | `63256e0` | `src/Gui.cpp` (8 lookups) |
| 4 | Cancelling lasers (bomb, boss death) awarded far too few items/score | `RemoveAllBullets` stepped the laser split by `1.0f`; the sibling `DespawnBullets` already used `32.0f` | `a393f40` | `src/BulletManager.cpp` |
| 5 | Respawn (post-death) animation stretched at the wrong rate | timer divided by `60.0f` instead of `30.0f` | `a393f40` | `src/Player.cpp` |
| 6 | Radial effect scatter offset wrong | random-range addend `0.0f` instead of `1.0f` | `a393f40` | `src/EffectManager.cpp` |
| 7 | Narrow "mirror barrier" bullet warp scaled by the inverted ratio | the two branches swapped `67.8822556` and `135.7645111` | `a393f40` | `src/EclExIns.cpp` |

### Hardened (upstream `a45e99fb`)

- `src/pbg/Lzss.cpp`: `DECODE_HANDLE_FETCH` bounds-checks before reading, so a
  truncated LZSS stream can no longer read one byte past the buffer (this is the
  path that decodes `th08.dat` / `thbgm.dat` entries).
- `src/Global.{hpp,cpp}`: `FileSystem::TryDecryptFromTable` reports the real
  decrypted size (`size - 4`) instead of leaving the caller's size at the
  encrypted length; callers that bound-check now stop reading past the buffer.
- `src/Global.hpp`, `src/MusicRoom.cpp`, `src/TitleScreen.cpp`, `src/zwave.cpp`,
  `src/main.cpp`: legacy array allocations are released with `delete[]` under
  `TH08_MODERN_PORT` via new `ZUN_DELETE_ARRAY` / `ZUN_DELETE_ARRAY2` /
  `SAFE_DELETE_LEGACY_ARRAY` macros; scalar `delete` is kept for the VC7-exact
  build, so no behavioural difference on the original target.

### not fixed 

- **Invisible lasers.** No commit in `N0zoM1z0/th08` — on any branch

- **Background flickering** is not addressed by a dedicated upstream commit
### Build / release

- NACP version is now `1.00d-r3`.
- `.github/workflows/build-switch.yml` now also triggers on `v*` tags and, on a
  tag, publishes a GitHub Release with `touhou08.nro` and `touhou08.nro.sha256`,
  using `RELEASE_NOTES.md` as the release body.
