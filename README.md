# EARTHtotaled

A solo survival shoot-and-loot game about Mara Venn following the last signal through endless forests and ruined cities. Water keeps you alive. Candybar wrappers are the currency.

The current game is the **Unreal Windows edition, Weather and Music 1.4.1**. It has enterable buildings, human settlements, quests, a friendly robot faction, ten weapons, a connected skill web, day/night survival, changing weather, and three original music themes.

- [Download the playable Windows release on itch.io](https://coralsorta.itch.io/earthtotaled)
- [Unreal source, build instructions, and controls](work/earthtotaled-unreal/README.md)
- [Release record and verification limits](docs/RELEASES.md)
- [Remaining full-release checks](docs/RELEASE-CHECKLIST.md)

## Source layout

`work/earthtotaled-unreal` is the active Unreal Engine 5.8 project. Its C++ source, design data, original art/audio source files, and generation/import tools are versioned. Unreal's generated assets, caches, compiled binaries, and packaged downloads are rebuilt locally.

`work/earthtotaled` preserves the older browser game and its WebView2 launcher source. Its `dist` directory contains the actual browser source modules, including the Three.js vendor file and license; it is intentionally tracked. The Unreal design-export tool reads this sibling project. Browser and Unreal saves are separate.

Run `npm test` from `work/earthtotaled` for the legacy source checks and standalone HTML build. Generated test copies stay local and are recreated by the test scripts.

Release scripts write disposable packages and reports into the repository's ignored `outputs` directory; the legacy browser build writes `work/earthtotaled/standalone/EARTHtotaled.html`. Player saves, local work logs, credentials, and editor caches are excluded. Keep the two project folders in this layout when running their existing scripts.

## Unreal build

Install Unreal Engine 5.8, compatible Visual Studio C++ build tools, the Windows SDK, .NET Framework 4.8 development components, Python 3.11 or newer, and Node.js. Epic's engine is installed separately; it is not redistributed here.

From `work/earthtotaled-unreal`, use the commands in its README. The checked-in `ArtSource` files and `Content/Data` can be imported directly with `./Build-Unreal.ps1 -Mode Content`; regenerating the source art and speech is optional. Then run `./Build-Unreal.ps1 -Mode Package`. Keep every file in the resulting Windows game folder together.

The game opens fullscreen. F11 changes between fullscreen and a window. Start it and right-click its taskbar icon to pin it. Escape opens the saved music/weather volume controls and pauses survival.

## Export the complete source

From the repository root, run `python work/earthtotaled-unreal/Tools/package-source.py`. It writes `outputs/EARTHtotaled-complete-source.zip` with both project folders, shared documentation, and original art/audio source. The older source ZIP files are left intact.

The exporter requires a Git checkout and packages tracked files with their current edits. Review and stage intended new files first. It refuses unmerged changes, unreviewed new files, and tracked player data or local work logs. `SOURCE-MANIFEST.json` records each included file's SHA-256, the base commit, and whether the source has uncommitted changes. A Git checkout is needed only for this export command; extracting the ZIP preserves the layout needed by the game build and check scripts.

## Rights and dependencies

This repository does not grant an open-source license for EARTHtotaled's original code, story, art, or music. Retained third-party notices apply to their respective components. The older browser edition includes Three.js under its bundled MIT license. Its optional desktop launcher requires Microsoft's separately downloaded WebView2 SDK; that SDK and compiled game/engine files are not committed.
