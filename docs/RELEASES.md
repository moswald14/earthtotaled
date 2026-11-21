# EARTHtotaled release record

## Unreal 1.4.1 — Fullscreen rebuild

Published on November 21, 2026 (Eastern) at [coralsorta.itch.io/earthtotaled](https://coralsorta.itch.io/earthtotaled). This rebuild retains the Weather and Music game content and save format. The stable local executable path is unchanged, so existing game shortcuts keep working.

Editor and Shipping compilation, cooking, staging, and archiving completed successfully with Unreal 5.8.4. Fresh verification passed 1,415 source/content checks and all 230 checks in the packaged executable. Those native checks cover capture restoration after repeated workbench, skill, contact, journal, pause, and inventory panels, along with game content and save roundtrips.

An actual fullscreen session on the development PC checked workbench interaction and return, pause/resume, inventory return, F11 switching, and Alt-Tab/return. Mouse movement changed the camera yaw with both mouse buttons released, look input enabled, and the game cursor hidden. Five such samples were recorded by the new opt-in Shipping input audit. All tests used temporary QA data; the player's existing survivor remained byte-for-byte unchanged. This is not a clean-machine installation or long-session endurance test.

The Windows archive contains 34 files matching the verified installed build, excluding saves, test logs, and debug symbols. The public page shows 1.4.1 as the current Windows download; 1.4.0 is retained but hidden, and the legacy browser download remains separate. Pricing and the existing AI disclosure were preserved. A fresh download through Chrome's public free-download flow matched the uploaded archive's size and SHA-256 exactly.

Release archive: `EARTHtotaled-Unreal-Windows-v1.4.1.zip` (407,098,669 bytes).

SHA-256: `e0e0d6588ff36a98b1398c478603b7716a4fb60958ee676c6c69d6c0bd43d150`.

Shipping executable SHA-256: `48307710478a7fff880c8817413f75579cc4c3b62db15412bcd3dbd13dec8b71`.

## Source maintenance — November 21, 2026

The complete source export is now `EARTHtotaled-complete-source.zip`. It preserves both project folders, shared documentation, original art/audio, and the sibling browser data needed by the Unreal design-export tool. It includes a file-hash manifest and excludes local work logs, player saves, compiled builds, and caches. The older Unreal-only source ZIP is obsolete.

The legacy `npm test` command now checks the standalone HTML it just built. Its former output path failed in a fresh extraction and could read an unrelated old build in an existing workspace.

Validation used a new extraction with Node.js 24.22.2 and Python 3.15.0: all 327 source files matched their manifest hashes and the checkout; the standalone build and 64 gameplay plus 18 wilderness checks passed; the design-data export reproduced the checked-in data apart from line endings; 1,415 Unreal content checks and six weather/music audio checks passed. The browser suite also passed in the original checkout.

This source-maintenance update does not rebuild or republish the Windows game. Unreal compilation, cooking, rendered gameplay, installation, and the remaining full-release checks require separate verification.

## Unreal 1.4.0 — Weather and Music

Published on November 16, 2026 (Eastern) at [coralsorta.itch.io/earthtotaled](https://coralsorta.itch.io/earthtotaled). This is a playable game in development, not a claim that every full-release milestone is complete.

- Clear skies, overcast, mist, rain, and thunderstorms blend with the saved world clock. Clouds, lighting, and haze respond to weather. Roof checks keep nearby rain above shelter; indoor rain and wind are muffled. Lightning is followed by delayed thunder.
- Three original 64-second stereo themes crossfade between daytime forest, daytime city, and nighttime. Dialogue reduces music volume. Escape provides independent saved music and weather levels, including mute.
- The update retains the existing story, ten weapons, settlements, water rules, 18-node skill web, endless scenery streaming, and candybar-wrapper currency. Existing Unreal survivors continue; browser saves remain separate.

### Recorded release verification

The November 16 build passed 1,415 source/content checks, audio artifact checks, and 230 checks in the packaged executable with zero failures. Actual fullscreen review sessions displayed forest thunderstorms, a misty city, rainy nighttime, and working sound controls. Playback audits reported the expected forest, city, and night music channels playing. These audits establish playback state, not subjective listening quality or an overnight endurance test.

The player's existing save was unchanged throughout temporary QA sessions. After publication, the normal launcher was reopened; computer control ended before its paused state could be confirmed. No subsequent play session is implied by this record.

Release archive: `EARTHtotaled-Unreal-Windows-v1.4.0.zip` (407,098,707 bytes).

SHA-256: `55cd674a714b046416b3fdd1d679b03743a887ddebfd23e357acb13cd7c60dd6`.

The release archive is distributed through itch.io rather than committed to Git. The upload reported success and the public page displayed the new Windows download. A fresh download hash comparison was not performed.

## Earlier Unreal updates

- **1.3.1 — Candybar wrappers:** changed currency labels, pickups, rewards, prices, and SALT-7 dialogue while retaining serialized identifiers and balances.
- **1.3 — Wilds and Ruins:** branching trees, forest-floor detail, damaged architecture, more city clutter, and twenty synthetic spoken lines with subtitles.
- **1.2 — Detail:** higher-resolution rendering options, denser meshes and terrain, improved surface textures, and performance review tools.
- **1.1 — Scavenger update:** expanded weapons, inventory and crafting, a prepared starting district, staged surrounding scenery generation, and additional sound effects.

The legacy browser prototype remains a separate download and behavioral reference. Unreal is the active development direction.
