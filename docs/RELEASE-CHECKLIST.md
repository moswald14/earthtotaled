# From playable release to full release

The Windows Unreal game is already published through version 1.4.1. This checklist carries the remaining full-release work into the same project rather than treating the earlier migration plan as a current blocker. Unchecked items are future verification gates, not new implementation commitments.

- [x] Export the complete source layout and check a fresh extraction. Verified November 21, 2026: all 327 source-file hashes and archive contents; legacy standalone build and 64 gameplay plus 18 wilderness checks; sibling design-data export; 1,415 Unreal content checks; six weather/music audio checks. These are source checks, not a clean Unreal compile or installation test.
- [x] Rebuild Unreal Windows 1.4.1 and check fullscreen input on the development PC. November 21, 2026: editor and Shipping compilation, cooking, staging, 230 packaged checks, and actual mouse look after workbench/menu use, F11 switching, and Alt-Tab passed. Temporary QA data kept the player's survivor unchanged. A clean Windows installation test remains below.
- [x] Download the published 1.4.1 archive through itch.io's public free-download flow. Its 407,098,669 bytes and SHA-256 matched the verified upload on November 21, 2026; see the release record.
- [ ] Play the entire story from a fresh survivor through the irrigation ending.
- [ ] Play the warning ending separately and verify continued exploration after both endings.
- [ ] Test installation on a clean Windows PC, fullscreen/mouse behavior, and taskbar launching.
- [ ] Exercise update-safe saves with real prior-version fixtures, including inventory, wrappers, quests, perks, emptied loot, position, time, and sound preferences.
- [ ] Run extended forest-to-city exploration across multiple weather fronts and complete day/night cycles; record frame timing, memory use, and loading interruptions.
- [ ] Check the water economy and repeatable human rewards over a long session without relying on robot water drops.
- [ ] Review music, voices, weather, and sound-effect balance by listening on speakers and headphones.
- [ ] Review small-screen and lower-resolution menus, skill-web clarity, and control instructions.
- [ ] Exercise installation, upgrade, and rollback instructions with a freshly downloaded itch.io archive before the next release candidate.

Use temporary QA saves for destructive or scripted tests. Preserve the player's real survivor, keep currency as candybar wrappers, and retain Unreal as the active engine.
