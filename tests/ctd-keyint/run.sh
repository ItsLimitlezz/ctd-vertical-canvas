#!/usr/bin/env bash
# CTD: checks the vertical stream's keyframe rule (ctd_stream_keyint_sec in vertical-canvas.hpp) against real libobs
# obs_data: 2 s when no value is set (an encoder default of 0 doesn't count), the user's own value otherwise, and never
# 0 (auto) for an output that feeds CTD Relay. Not part of the plugin build. Needs g++ and a libobs build, as for
# tests/ctd-encoder-refs (see its run.sh header for the cmake line); no display needed.
#
#   OBS_SRC=<ctd-obs or obs-studio 33.0.1> OBS_BUILD=<build> tests/ctd-keyint/run.sh
set -euo pipefail
: "${OBS_SRC:?set OBS_SRC to a ctd-obs checkout and OBS_BUILD to its cmake build directory}"
: "${OBS_BUILD:?set OBS_BUILD to the cmake build directory of OBS_SRC (ninja libobs)}"
here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
{
	grep '^#define CTD_DEFAULT_KEYINT_SEC' "$here/../../vertical-canvas.hpp"
	awk '/^static inline int ctd_stream_keyint_sec/,/^}/' "$here/../../vertical-canvas.hpp"
} >"$work/keyint.inc"
grep -q 'ctd_stream_keyint_sec' "$work/keyint.inc"
g++ -std=c++17 -Wall -I"$OBS_SRC/libobs" -I"$OBS_BUILD/config" -I"$work" "$here/test.cpp" \
	-L"$OBS_BUILD/libobs" -lobs -Wl,-rpath,"$OBS_BUILD/libobs" -o "$work/test"
"$work/test"
