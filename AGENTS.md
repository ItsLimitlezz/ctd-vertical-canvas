# AGENTS.md — ctd-vertical-canvas
> Team rules live in the hub: github.com/ItsLimitlezz/CTD-Software-Suite → AGENTS.md (read it first). This file is repo-specific.

## What this is
Fork of **Aitum Vertical** ("Vertical Canvas for OBS Studio", upstream: github.com/Aitum/obs-vertical-canvas), an OBS plugin that adds a vertical canvas. Plugin name `vertical-canvas`, version 1.6.7, bundle id `tv.aitum.vertical`. GPL-2.0 (`LICENSE`). Public repo, open-source. CTD uses it for the vertical (9:16) output.
**CTD delta: TBD (owner to confirm).** This clone is `--depth 1` (one upstream commit, `Bump version` by Exeldro, 2026-10-27) with large blobs filtered, so history is unavailable. No CTD-specific files or README notes were found; assume plain upstream. Upstream sync policy: TBD (owner to confirm).

## Layout
Flat C/C++ plugin at the repo root (Qt6, libobs, obs-frontend-api):
| Path | What |
| --- | --- |
| `vertical-canvas.cpp/.hpp` | Plugin entry and canvas logic |
| `config-dialog.*`, `name-dialog.*`, `hotkey-edit.*` | Settings and dialogs |
| `scenes-dock.*`, `sources-dock.*`, `transitions-dock.*`, `source-tree.*` | Docks |
| `projector.*`, `qt-display.*`, `display-helpers.hpp` | Preview and projector |
| `multi-canvas-source.*`, `audio-wrapper-source.*` | Sources |
| `data/` | Locale `.ini` files (`en-US.ini` is the base), images, `manifest.json` |
| `CMakeLists.txt`, `CMakePresets.json`, `buildspec.json`, `cmake/`, `build-aux/` | obs-plugintemplate build; `buildspec.json` pins OBS 31.1.0 |

## Build & test
From README: out-of-tree build with `cmake -S . -B build -DBUILD_OUT_OF_TREE=On && cmake --build build` (needs OBS development files); in-tree build by checking this repo out into OBS's `UI/frontend-plugins/vertical-canvas` and adding `add_subdirectory(vertical-canvas)`. `CMakePresets.json` defines presets `windows-x64`, `macos`, `ubuntu-x86_64`, `linux-aarch64` (+ `-ci`); the exact CTD invocation: TBD (owner to confirm). No automated tests in the repo.
- Windows (and Linux, TBD) test builds on the **PC**; the macOS build on the **Mac**.
- Cloud sessions cannot build this. Tag PRs `Needs local build: PC` or `Needs local build: Mac`.
- Formatting: `.clang-format`, `.cmake-format.json`; CI has a format check.

## Release
Open-source (GPL-2.0), so prod builds may use GitHub Actions. The existing `push.yaml` builds on push to `main`/`master`/`release/**` and on tags, and creates a release for a valid version tag; `dispatch.yaml` is the manual build. Procedure for CTD releases (version bump, tag format): TBD (owner to confirm). Release needs the hub's Senior check first.

## Conventions & gotchas
- Keep CTD changes small and isolated so upstream merges stay clean; no drive-by reformatting.
- Upstream branding: do not translate "Aitum" or "Backtrack" (`TRANSLATIONS.md`).
- Workflows `pr-pull.yaml` and `push.yaml` run on every PR and push to `main`, not only on tags/dispatch. This differs from the hub's build policy; TBD (owner to confirm).
- `buildspec.json` pins OBS 31.1.0 and deps 2025-05-23, older than the ctd-obs-plugin's OBS 32.2 target: check compatibility before pairing them. TBD (owner to confirm).
- Do not sync from upstream, rebase, or merge without a Senior.

## Required reading
- `README.md`, `TRANSLATIONS.md`
- Lessons file: none yet.
