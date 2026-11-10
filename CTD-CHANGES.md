# CTD changes to Aitum Vertical

This is a **modified version of [Aitum Vertical](https://github.com/Aitum/obs-vertical-canvas)** ("Vertical Canvas for OBS
Studio") by Aitum and Exeldro, licensed under the **GNU General Public License v2.0** (see [LICENSE](LICENSE)). CTD OBS
builds it in as the core module `vertical-canvas` and shows it to users as **Vertical**. All upstream copyright notices
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
