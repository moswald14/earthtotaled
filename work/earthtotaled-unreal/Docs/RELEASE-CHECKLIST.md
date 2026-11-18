# EARTHtotaled — release checklist and current status

Updated November 17, 2026. This replaces the November 15 assumption that an Unreal project still needed to be created.

## Current baseline

The project is now a native Unreal Engine game. The reviewed `.uproject` selects Unreal 5.8, and the local release manifest records **1.4.0-unreal**, built with **5.8.4**. The Windows release ZIP exists locally and is 407,098,707 bytes.

The current project documentation covers ten guns, inventory and crafting, a prepared starting district, endless forest/city exploration, settlements and quests, water survival, native saving, the skill web, and climbing. It also covers the later environment-detail, synthetic-voice, weather, and original-music updates. **Candybar wrappers are the currency.** The serialized legacy currency field remains for save compatibility.

The saved native verification report reads **230 checks / 0 failures**. That is existing evidence from the game project, not a fresh test run by this planning review. The main project chat reports version 1.4 published at [the existing itch.io page](https://coralsorta.itch.io/earthtotaled), and a dated publication screenshot exists. A fresh public-page fetch failed during this review, so present-day download availability was not independently reconfirmed.

## What no longer needs to be planned from scratch

- Creating the initial Unreal project and selecting an engine.
- Rebuilding the first gameplay loop from the browser version.
- Producing the first native Windows package.
- Designing the initial save format or deciding whether browser saves import automatically. Current documentation explicitly keeps browser/WebView saves separate from the native Unreal slot.
- Treating six weapons or gold-labelled currency as the current release scope.

Future work starts from the existing Unreal source and current release. Preserve the older browser source because the project still uses its design-export tooling.

## Remaining release-quality evidence

Unchecked items below mean **not established by the evidence reviewed here**, not confirmed product defects. Link completed test records before marking them passed.

### Full player journey

- [ ] Record complete packaged-game playthroughs from a new save through both ending choices.
- [ ] Cover the required story route, side quests, settlement rewards, weapon acquisition, trading, skill prerequisites, climbing and continued exploration after the ending.
- [ ] Check menu, workbench, conversation, death/restart, focus loss and resume behavior through normal player input.
- [ ] Confirm the intended final handoff state of the normal game; the last reported reopen did not confirm that it was paused. Do not change a running player session merely to update this document.

### Saves and updates

- [ ] Test a supported native save upgrading to the exact release candidate, including wrapper balances, inventory, quests, perks, time, weather, audio preferences and regional loot.
- [ ] Test recovery from interrupted save writes and clearly handled unavailable/corrupt save data.
- [ ] Keep temporary review/test saves separate from the player's normal slot.
- [ ] Keep browser/WebView-save separation explicit in player instructions. Add an importer only as a separate requested feature.

### Performance and exploration

- [ ] Run a longer packaged exploration test that crosses terrain boundaries, revisits looted areas, enters city interiors and samples combat and all weather states.
- [ ] Record frame timing, memory growth, loading/stalls and save growth on identified hardware/settings.
- [ ] Test several documented seeds and difficult coordinates, while retaining deterministic terrain and persistence checks.
- [ ] Base advertised requirements and performance claims on those measurements.

An existing weather sample reports 39.02 average FPS at 100% render scale over a 25-second stationary view after a five-second warmup. It does not establish long-session performance or a frame-rate guarantee. Current art remains procedural, and people/wildlife are rigid models; presentation claims should match the shipped game.

### Distribution and player materials

- [ ] Download the candidate through the storefront and compare its hash with the retained release manifest.
- [ ] Test a clean launch on a suitable second Windows PC with all required runtime components.
- [ ] Check final controls, settings, audible output, subtitles, installation directions and native save location.
- [ ] Check credits, notices and distribution rights against the actual assets, voices, libraries and engine version being shipped.
- [ ] Keep screenshots, feature counts, currency terminology, pricing and version labels consistent with the exact package.
- [ ] Retain the previous package and matching source revision. Verify save compatibility before recommending a downgrade.

## Build and release order

1. Start from the existing Unreal project and its documented build tools.
2. Implement only the requested change and update its focused checks.
3. Build/import the required content and produce the Windows package.
4. Run native packaged checks and relevant hands-on tests against that exact package.
5. Complete the applicable checklist items above and record open issues accurately.
6. Record source revision, engine/tool versions, package version, size and hash.
7. Update the existing storefront only within the user's release authorization, then verify the player-visible download.

Publication of an update and completion of every release-quality check are distinct facts. Keep each status explicit.

## Repository ownership

This checklist belongs in the **main EARTHtotaled repository**, alongside the Unreal project and required legacy design sources. Release planning is part of the game project and does not need a separate repository.

Track reviewed source, build tools, necessary source assets and documentation. Exclude player saves, credentials, local account settings, caches, generated build output and packaged downloads unless an explicit distribution workflow handles them. Keep generated binary assets and large source assets under an intentional storage policy.

## Evidence used for this update

- Current Unreal `README.md`, `EARTHtotaled.uproject` and player-facing packaged instructions.
- Local `EARTHtotaled-Unreal-release.json` and the version 1.4 Windows ZIP inventory.
- Existing `Unreal-QA/Saved/native-verification.txt` and `Unreal-Weather-QA/Saved/graphics-performance.json`.
- The main project's November 16 weather/music completion report and retained publication screenshot.

The saved 1.4 package hash is `55cd674a714b046416b3fdd1d679b03743a887ddebfd23e357acb13cd7c60dd6`. It is quoted from the local release manifest; this review did not rehash the package, rerun the game, or verify a new download.
