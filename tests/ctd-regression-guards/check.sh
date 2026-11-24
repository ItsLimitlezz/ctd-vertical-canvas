#!/usr/bin/env bash
# CTD: source checks that fail if one of the two fixed crashes can come back. Not part of the plugin build; needs only
# bash, awk and a C++ compiler's preprocessor (comments are stripped first, so explanations don't count).
#
# 1. Encoder double release (ctd-vertical-canvas#12, crash in obs_encoder_release on stream start). An output holds its
#    own reference to every encoder it is given (OBS 31+), so the canvas releases only references it owns, right after
#    handing them over, and only in output_take_video_encoder / output_take_audio_encoder. Anywhere else, use
#    OBSEncoderAutoRelease for a reference you own and never release a pointer from obs_output_get_*_encoder.
# 2. Preview display rebuilt on QEvent::Expose (hotfix 0ee5a7f, render_display crash on minimize, fullscreen and Space
#    changes on macOS). The display is destroyed with its native surface and re-created only after a new surface,
#    queued after the event; nothing on Expose.
#
# Usage: tests/ctd-regression-guards/check.sh   (exit 0 = all hold)
set -uo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
cxx="${CXX:-g++}"
fail=0
bad() {
	echo "FAIL: $*"
	fail=1
}
strip() { "$cxx" -fpreprocessed -dD -E -P -x c++ "$1" 2>/dev/null; }
# body() REGEX: print each function whose first line matches REGEX, up to its closing brace (brace counting: the
# preprocessor doesn't keep indentation)
body() {
	awk -v re="$1" '
		!f && $0 ~ re { f = 1; d = 0; open = 0 }
		f {
			print
			n = split($0, c, "")
			for (i = 1; i <= n; i++) {
				if (c[i] == "{") { d++; open = 1 }
				else if (c[i] == "}") d--
			}
			if (open && d == 0) f = 0
		}'
}

# --- 1. encoder releases -------------------------------------------------------------------------------------------
total=0
for f in "$root"/*.c "$root"/*.cpp; do
	n=$(strip "$f" | grep -c 'obs_encoder_release[[:space:]]*(')
	total=$((total + n))
	if strip "$f" | grep -q 'obs_encoder_release[[:space:]]*([[:space:]]*obs_output_get_'; then
		bad "$(basename "$f") releases a pointer from obs_output_get_*_encoder (borrowed: the output owns it)"
	fi
done
inside=$(strip "$root/vertical-canvas.cpp" | body '^static void output_take_(video|audio)_encoder[(]' |
	grep -c 'obs_encoder_release[[:space:]]*(')
if [[ "$inside" -ne 2 ]]; then
	bad "expected obs_encoder_release once in each of output_take_video_encoder / output_take_audio_encoder, found $inside"
fi
if [[ "$total" -ne "$inside" ]]; then
	bad "obs_encoder_release outside output_take_*_encoder ($((total - inside)) call(s)): hand an owned encoder over with" \
		"output_take_*_encoder, or hold it in OBSEncoderAutoRelease"
fi
[[ $fail -eq 0 ]] && echo "ok: encoder references released only by output_take_video_encoder / output_take_audio_encoder"
f1=$fail
fail=0

# --- 2. preview display ----------------------------------------------------------------------------------------------
src=$(strip "$root/qt-display.cpp")
filter=$(echo "$src" | body 'bool eventFilter[(]QObject')
[[ -n "$filter" ]] || bad "qt-display.cpp: SurfaceEventFilter::eventFilter not found (update this check if it moved)"
if echo "$src" | grep -q 'QEvent::Expose'; then
	bad "qt-display.cpp handles QEvent::Expose: re-creating the display there crashed in render_display (0ee5a7f)"
fi
if echo "$filter" | grep -q 'CreateDisplay[[:space:]]*('; then
	bad "qt-display.cpp: eventFilter calls CreateDisplay directly; re-create it queued after SurfaceCreated (RestoreDisplay)"
fi
if ! echo "$filter" | grep -q 'Qt::QueuedConnection' || [[ $(echo "$filter" | grep -c 'RestoreDisplay[[:space:]]*(') -ne 1 ]]; then
	bad "qt-display.cpp: SurfaceCreated must re-create the display through one queued RestoreDisplay call"
fi
if ! echo "$filter" | grep -q 'SurfaceAboutToBeDestroyed' || ! echo "$filter" | grep -q 'DestroyDisplay[[:space:]]*('; then
	bad "qt-display.cpp: SurfaceAboutToBeDestroyed must call DestroyDisplay"
fi
if echo "$src" | grep -q 'destroying[[:space:]]*=[[:space:]]*false'; then
	bad "qt-display.cpp clears 'destroying' outside RestoreDisplay (qt-display.hpp)"
fi
if ! echo "$src" | body '^void OBSQTDisplay::CreateDisplay[(][)]' | grep -q 'if (destroying)'; then
	bad "qt-display.cpp: CreateDisplay must return while the surface is being destroyed (if (destroying))"
fi
[[ $fail -eq 0 ]] && echo "ok: preview display not re-created on Expose; re-created only queued after a new surface"

exit $((f1 | fail))
