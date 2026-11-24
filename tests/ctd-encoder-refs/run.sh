#!/usr/bin/env bash
# CTD: checks the vertical canvas's encoder ownership rule ("encoder ownership" in vertical-canvas.cpp) against real
# libobs, and shows what the old get -> set -> release(old) pattern did. Not part of the plugin build. Linux only.
#
# Needs g++, an X display (xvfb-run is used when there is none) and libobs 31.0 or newer: outputs take their own
# encoder reference from 31.0 on. Ubuntu 24.04's libobs-dev is 30.0, so point this at a libobs built from ctd-obs
# (obs-studio at tag 33.0.1 works too; it also needs extra-cmake-modules installed):
#
#   cmake -S <ctd-obs> -B <build> -G Ninja -DOBS_VERSION_OVERRIDE=33.0.1 -DENABLE_FRONTEND=OFF -DENABLE_PLUGINS=OFF \
#     -DENABLE_SCRIPTING=OFF -DENABLE_WAYLAND=OFF -DENABLE_PULSEAUDIO=OFF -DENABLE_BROWSER=OFF
#   ninja -C <build> libobs
#   OBS_SRC=<ctd-obs> OBS_BUILD=<build> tests/ctd-encoder-refs/run.sh
#
# (On Ubuntu 24.04, lower "find_package(FFmpeg 8.0" in libobs/CMakeLists.txt to 6.1 in a throwaway checkout first.)
# Expected: the old pattern is balanced on the plain path, crashes in obs_encoder_release on a slot the canvas holds
# no reference for, and the fixed pattern runs clean with no encoder left. REFTEST_DIRECT=1 runs the crashing case
# without a child process (for gdb); REFTEST_VERBOSE=1 shows libobs's log.
set -euo pipefail
: "${OBS_SRC:?set OBS_SRC to a ctd-obs checkout and OBS_BUILD to its cmake build directory}"
: "${OBS_BUILD:?set OBS_BUILD to the cmake build directory of OBS_SRC (ninja libobs)}"
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
awk '/^static void output_take_video_encoder/,/^\/\* CTD: config.json is saved/' "$here/../../vertical-canvas.cpp" | sed '$d' >"$work/helpers.inc"
grep -q output_take_audio_encoder "$work/helpers.inc"
g++ -std=c++17 -g -Wall -I"$OBS_SRC/libobs" -I"$OBS_BUILD/config" -I"$work" "$here/test.cpp" \
	-L"$OBS_BUILD/libobs" -lobs -Wl,-rpath,"$OBS_BUILD/libobs" -o "$work/test"
if [[ -n "${DISPLAY:-}" ]]; then
	"$work/test"
else
	xvfb-run -a "$work/test"
fi
