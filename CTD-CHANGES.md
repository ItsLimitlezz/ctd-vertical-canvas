# CTD changes to Aitum Vertical

This is a **modified version of [Aitum Vertical](https://github.com/Aitum/obs-vertical-canvas)** ("Vertical Canvas for OBS
Studio") by Aitum and Exeldro, licensed under the **GNU General Public License v2.0** (see [LICENSE](LICENSE)). CTD OBS
builds it in as the core module `vertical-canvas` and shows it to users as **Vertical** (docks: **CTD Vertical …**). All upstream copyright notices
and the licence are kept; every CTD change is listed here so upstream merges stay manageable.

Base: Aitum Vertical 1.6.7.

| Date | Area | What changed | Where |
|---|---|---|---|
| 2026-11-06 | Build | Built in-tree as a core module of CTD OBS; no update check against Aitum's server | `CMakeLists.txt` |
| 2026-11-09 | CTD Relay | `ctd_vertical_set_output` / `ctd_vertical_get_output` procs and a 2 s keyframe default for the vertical stream | `vertical-canvas.cpp` |
| 2026-11-09 | Name | User-visible "Aitum Vertical" / "Vertical Canvas" is now **Vertical**: module name and description, canvas name, log prefix, multitrack messages (English locale, plugin manifest, Windows file details) | `vertical-canvas.cpp`, `data/locale/en-US.ini`, `data/manifest.json`, `resource.rc.in` |
| 2026-11-09 | Name | The canvas is called `Vertical`; a saved canvas called `Aitum Vertical` is renamed on load, so existing scene collections keep their vertical scenes. Multitrack's extra canvas is saved by UUID, so it is unaffected | `vertical-canvas.cpp` (`StartVideo`) |
| 2026-11-09 | Toolbar | Removed the donate (heart) button and the Aitum logo/link button; the settings gear stays | `vertical-canvas.cpp` |
| 2026-11-09 | Network | Removed the version check, the update prompt and the "partner blocks" that were downloaded from Aitum's API and shown in the dock; removed `file-updater.c/.h` and the libcurl dependency. The module makes no network calls of its own | `vertical-canvas.cpp/.hpp`, `CMakeLists.txt` |
| 2026-11-09 | Settings | Removed the Help page (Aitum troubleshooter, guides, Discord), the Support Aitum page and the "View guide" links to Aitum's site. The footer reads "Version x.y.z - based on Aitum Vertical (GPL-2.0)" instead of "made with ♡ by Aitum" | `config-dialog.cpp`, `data/locale/*.ini` |
| 2026-11-09 | Look | Removed hard-coded button and warning colours, so the CTD Night theme (or any OBS theme) styles the dock and its settings | `vertical-canvas.cpp`, `config-dialog.cpp` |
| 2026-11-09 | Locale | Dropped strings for the removed UI from every locale | `data/locale/*.ini` |
| 2026-11-16 | Name | The four docks show as **CTD Vertical**, **CTD Vertical Scenes**, **CTD Vertical Sources** and **CTD Vertical Transitions** (display titles only; new locale key `DockTitle`, English only, other locales fall back to it). Dock ids, config keys and the Backtrack output name are unchanged, so saved layouts still restore | `vertical-canvas.cpp`, `data/locale/en-US.ini` |
| 2026-11-16 | Logs | Log prefixes `[Vertical Canvas]` and `[Vertical]` are now `[CTD] vertical:`; message text is unchanged. The canvas-rename line still names the old and new canvas (`'Aitum Vertical'` → `'Vertical'`) | `vertical-canvas.cpp`, `config-dialog.cpp` |
| 2026-11-17 | Send stream | `ctd_vertical_set_output` (CTD Relay dock "Send stream") also turns on "Start and stop streaming when main OBS starts and stops streaming", so the vertical stream starts and stops with the main one, as the dock's dialog says | `vertical-canvas.cpp` |
| 2026-11-19 | Crash fix | Encoder ownership follows libobs: an output holds its own reference to each encoder it is given and releases it when the encoder is replaced or the output is destroyed, so the canvas no longer releases encoders it reads back from an output (`obs_output_get_video_encoder` / `obs_output_get_audio_encoder` results are borrowed) and drops its own reference as soon as the output has the encoder. Fixes a crash in `obs_encoder_release` when starting the vertical stream after the stream encoder changed (the old encoder was freed by the output, then released again), and encoder leaks on start, on stop and when outputs are removed. `GetStreamVideoEncoder` now always returns a reference of its own (the Backtrack path did not) | `vertical-canvas.cpp`, `config-dialog.cpp` |
| 2026-11-22 | Crash fix | The preview display (`OBSQTDisplay`, the canvas preview and projectors) is no longer re-created on every `QEvent::Expose` (minimize, restore, fullscreen, Space changes). Once its native surface is about to be destroyed the display stays down until the surface is created again, and then it is re-created after the event (queued), not inside it, so libobs never renders to a view that is going away (OBS fixed the same macOS `render_display` crash by not re-creating displays, upstream fd502cb). The window signals use the widget as context, and a missing native window creates nothing | `qt-display.cpp` |
| 2026-11-22 | Crash fix | Scene renames are handled on the UI thread: `source_rename` (called on whichever thread renamed the scene, e.g. obs-websocket's `SetSceneName`) only copies the names and queues `SceneRenamed`, which updates the scene list, the scenes combo and the main scenes' `canvas` settings. A vertical scene's own `rename` handler (the global `source_rename` is sent for the main canvas only) is disconnected when the canvas is cleared and when the dock is destroyed (`DisconnectSceneRenames`), so a rename after a canvas dock is deleted no longer calls into the freed dock. Main scenes no longer get a per-scene handler that was never disconnected (the global signal already covers them) | `vertical-canvas.cpp`, `vertical-canvas.hpp` |
| 2026-11-22 | Crash fix | Output "start" (stream, recording, virtual camera) and recording "stopping" handlers no longer run Backtrack's `CheckReplayBuffer` on the output's own thread: it runs on the UI thread (`OnStreamStart`, `OnRecordStart`, `OnVirtualCamStart`, a queued call for "stopping"). Off the UI thread it could create outputs and encoders, start and stop OBS's own outputs and show a message box (AppKit off the main thread) | `vertical-canvas.cpp` |
| 2026-11-22 | Crash fix | obs-websocket vendor requests `get_scenes` and `current_scene` read the docks' scene lists and current scene on the UI thread (they read list widgets from obs-websocket's thread while the UI changed them), waiting at most 2 s so a request during quit fails instead of deadlocking | `vertical-canvas.cpp` |
| 2026-11-22 | Crash fix | Cancelling the name of a new transition no longer releases it twice (`OBSSourceAutoRelease` plus `obs_source_release`) | `transitions-dock.cpp` |
| 2026-11-22 | Leaks | `pause_recording` / `unpause_recording` vendor requests release the recording output when they skip it; a removed scene's list item is deleted (and the next one is no longer skipped); the Add source menu and the show/hide transition menus made on every right-click or "+" are freed with their popup | `vertical-canvas.cpp`, `sources-dock.cpp` |
| 2026-11-24 | Settings | With "Use main OBS settings" ticked for the stream, the own encoder and its settings are hidden and two read-only rows show what the stream really uses: the profile's stream encoder (with its codec) and its keyframe interval. They showed the first encoder in the list (e.g. SVT-AV1) and a keyframe field the stream didn't use. Rows rebuilt after an encoder change now get the ticked state on Qt 6.7+ too (the re-sync invoked a signal nothing was connected to). New English strings `KeyframeInterval`, `KeyframeIntervalAuto`, `FromMainSettings` | `config-dialog.cpp/.hpp`, `data/locale/en-US.ini` |
| 2026-11-24 | CTD Relay | Keyframes: the own encoder's keyframe interval is saved as an explicit value (2 s when none is set), and never as 0 (auto) while an output feeds CTD Relay (Settings and `ctd_vertical_set_output`). At stream start an enabled CTD Relay output never gets 0, also when it comes from the main profile's `streamEncoder.json` (`ctd_stream_keyint_sec`) | `vertical-canvas.cpp/.hpp`, `config-dialog.cpp` |
| 2026-11-24 | CTD Relay | Encoder procs: `ctd_vertical_get_output` also returns `video_encoder` (the encoder id the next start uses) and `own_encoder`; new `ctd_vertical_use_profile_encoder(out bool success, out string error)` turns the canvas's own stream encoder off (UI thread, refused while the vertical stream is live or its Settings are open) and saves `config.json`, so CTD Relay's "Send stream" / **Fix it** can make the vertical feed H.264 | `vertical-canvas.cpp/.hpp` |
| 2026-11-24 | Tests | `tests/ctd-encoder-refs` (encoder ownership against real libobs, from #14), `tests/ctd-keyint` (keyframe rule against real `obs_data`) and `tests/ctd-regression-guards/check.sh` (source checks for the #12 double release and the `0ee5a7f` display rebuild). None is part of the plugin build | `tests/` |

## Kept on purpose

- Copyright headers, `LICENSE`, the upstream authors in `data/manifest.json`, and the credit line in the Vertical
  settings footer. CTD OBS should also credit Aitum Vertical in Help ▸ About and its third-party licences list.
- Internal identifiers, so saved settings and integrations keep working: module name `vertical-canvas`, config file
  `config.json` and its keys (including `partner_block`, now unused), dock id `VerticalCanvasDock`, Qt resource prefix
  `/aitum`, proc handlers `aitum_vertical_*`, obs-websocket vendor `aitum-vertical-canvas`, bundle id in
  `buildspec.json`.
- Mentions of other Aitum products by name: the log error shown when Aitum Stream Suite is installed, since it names a
  third-party plugin the user installed. The Multistream settings warning no longer names Aitum.

## Not changed yet

- Other languages still use the upstream wording for the strings that remain.
- `buildspec.json` and the out-of-tree packaging still describe upstream (they are not used by the CTD OBS build).
