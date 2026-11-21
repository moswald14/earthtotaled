# From playable release to full release

The Windows Unreal game is already published through version 1.4. This checklist carries the remaining full-release work into the same project rather than treating the earlier migration plan as a current blocker. Unchecked items are future verification gates, not new implementation commitments.

- [x] Export the complete source layout and check a fresh extraction. Verified November 21, 2026: all 327 source-file hashes and archive contents; legacy standalone build and 64 gameplay plus 18 wilderness checks; sibling design-data export; 1,415 Unreal content checks; six weather/music audio checks. These are source checks, not a clean Unreal compile or installation test.
- [ ] Play the entire story from a fresh survivor through the irrigation ending.
- [ ] Play the warning ending separately and verify continued exploration after both endings.
- [ ] Test installation on a clean Windows PC, fullscreen/mouse behavior, and taskbar launching.
- [ ] Exercise update-safe saves with real prior-version fixtures, including inventory, wrappers, quests, perks, emptied loot, position, time, and sound preferences.
- [ ] Run extended forest-to-city exploration across multiple weather fronts and complete day/night cycles; record frame timing, memory use, and loading interruptions.
- [ ] Check the water economy and repeatable human rewards over a long session without relying on robot water drops.
- [ ] Review music, voices, weather, and sound-effect balance by listening on speakers and headphones.
- [ ] Review small-screen and lower-resolution menus, skill-web clarity, and control instructions.
- [ ] Test a freshly downloaded itch.io archive, record its hash, and verify upgrade/rollback instructions before the next release candidate.

Use temporary QA saves for destructive or scripted tests. Preserve the player's real survivor, keep currency as candybar wrappers, and retain Unreal as the active engine.
