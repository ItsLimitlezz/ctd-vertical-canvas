#!/usr/bin/env bash
# CTD: checks the vertical canvas's config.json save/load (ctd_load_config, ctd_save_config) against real libobs.
# Not part of the plugin build. Needs g++ and libobs dev files (Ubuntu: apt-get install libobs-dev). Linux only:
# it fails libobs's rename() on demand to stand in for a Windows ReplaceFileW that doesn't land.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
awk '/^static const char \*kCtdSaveSeq/,/^static bool save_canvas\(\)/' "$here/../../vertical-canvas.cpp" | sed '$d' >"$work/helpers.inc"
g++ -std=c++17 -Wall -I/usr/include/obs -I"$work" "$here/test.cpp" -Wl,--export-dynamic -lobs -o "$work/test"
mkdir "$work/run"
"$work/test" "$work/run" | grep -v '^info:'
