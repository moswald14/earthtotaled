# EARTHtotaled release record

## Unreal 1.19.1 — Enemy movement and startup

Walkers and ground patrols follow the floors of buildings, paved streets and tunnels. They can turn around cover and attack from within reach. Flying enemies can steer past obstructions. Hives hatch guards on their own level. Highway troops patrol when neutral and still engage their faction rivals. Shelter 09 and reclaimed cities retain their protection.

The game draws its title menu without starting an installer. The 1.19.1 Windows installer repairs standard shortcuts from a registered portable copy and updates the pinned launcher. Opening a registered 1.19.1 portable copy directly offers a choice between the installed game and the portable one. Use **Game updates → v1.19.1** or run the Windows installer if the old game cannot reach its title menu. [Windows release](https://github.com/moswald14/earthtotaled/releases/tag/v1.19.1)

## Unreal 1.19.0 — Dry Season

The allotment southwest of Shelter 09 and restored glades support pumps, cisterns, crop beds and clinics. Carve downhill furrows with the Borer to irrigate Cinderbean, Saltroot and Mendleaf. Follow dry channels to find stranded docks and buried service hatches.

Nine skills now have ranks. Existing learned skills carry over at rank one. East Dispatch and the faction relay boards offer optional night hunts for XP and salvage. Choose the world score or three offline Doodle Jazz tracks in **Esc → Music**.

Choose **Game updates → v1.19.0** at the title or pause menu to continue your survivor. [Dry Season field guide](../work/earthtotaled-unreal/Docs/Dry-Season.md)

## Unreal 1.16.0 — MegaWAKE

Meet Thrum at Shelter 09 to search for Mara’s son, Kit. Lead a glitched deer herd to safety, drive SALT-7’s roadster to East Dispatch and follow the relay records to fraudCORP. Bug crews silence relays and Supertech crews retune them. The journal warns on the third job for one faction. Its fourth job turns the rival faction hostile. Both friends stay with Mara.

Repair a survey plane at Hollow Field and fly to Crown Field. In the vault, choose the Bug’s MOLT path or the Supertech frame’s OVERRIDE path. Both bodies can drive and fly. Escort Kit out before firing the Sunseed. Up to nine restored herds change the homecoming, and the survivor can keep exploring afterward.

Choose **Game updates** from the title or pause menu and select **v1.16.0**. Setup backs up all ten slots, previous checkpoints, exports, settings and mods. Returning to an older version gives it a separate save profile.

## Unreal 1.15.0 — Interchange

The West Ring and East Ring each have eight stops. Every city has two named, enterable businesses with supplies tied to the district. Find a crane over Cinder Quay's dockyard, potters' kilns at Kiln Exchange and a clock tower overlooking the bakery at Lastbell. Stock stays collected across saves and return visits.

The North Shuttle runs between Kiln Exchange and Windfall Reach. The South Shuttle runs between Signal Heights and Saltmarket. Both reverse on straight 3.064 km tracks. The atlas shows moving trains, walking transfers and searchable buildings. Press M then R for the whole system or L for the nearest station district. At the rear of a platform, E descends to street level. At the boarding edge, E waits for the next departure.

In the packaged Windows game a survivor boarded and jumped off at all sixteen ring stations, walked inside all 32 businesses and collected their stock. Reopening the save kept every shop searched. A second trip took both shuttles in both directions at 396 km/h and resumed on the moving westbound South Shuttle.

## Unreal 1.14.0 — Ringline

An open freight platform runs a 22.5 km circle through eight city districts. It reaches 396 km/h between 12-second stops. Each station has walkable stairs, a freight lift and a way to wait for the next train. The timetable and the passenger's position on the deck travel with the survivor save. On the atlas, R frames the entire line and clicking a stop tracks its platform.

Seven surfaces have native 4096 × 4096 base color, normal and roughness maps. Asphalt keeps the same four-metre texture scale on old city blocks and newer highway sections. The two conifers grew from 23,404 and 21,184 source triangles to 60,248 and 63,624. The ruined house has 14,680 triangles and the three-storey tenement has 48,156. The new default render scale is 125%. The pause menu ranges from 100% to 200%.

A new survivor boarded from Shelter 09, jumped at 396 km/h and landed back on the open deck. The packaged game loaded the surrounding terrain at all eight stops. The survivor returned to Shelter 09 and jumped onto the station, then reopened a mid-ride checkpoint through Continue and travelled with the train again. The fullscreen atlas test tracked a station by clicking it, restored mouse look on close and kept that station selected after reloading.

On a GeForce GTX 1080 at 1920 × 1080, a stationary forest view with 18 regions loaded averaged 42.1 FPS at 100%, 32.0 FPS at 125% and 16.1 FPS at 200%. The previous release averaged 43.3 FPS at 100% in the same view. Each run sampled 25 seconds after a five-second warmup. At 200%, the GPU reached 100% utilization and 58 °C.

## Unreal 1.13.0 — Landfall

A new survivor begins aboard a fraudCORP flight in 1986 and crash-lands at SHELTER 09 in 2253. The opening can be skipped. The pilot receives the Drifter .44 and a survey borer. Four marked banks form the first quest. Boring each one cuts a connected trench into the terrain. Finishing the perimeter pays 30 candybar wrappers and three water bottles. Ground Bugs avoid surface holes. Returning survivors retain their terrain and current story.

Settled human cities suppress hostile spawns across the entire district. Cleared cities stay settled after a missed watch payment. A funded watch repairs town defenses. Hostile deaths have a one-in-400 chance to scatter a unique gun. Each has two distinct perks, rolled base stats and one of three damage-driven upgrade paths. The pack's **Unique gun rack** displays the actual stats, perks and progress.

The Windows installer includes the engine's Microsoft Visual C++ x64 and GameInput prerequisites. The game checks GitHub at the title screen and offers a release browser in the pause menu. Confirming installs the selected release and relaunches. Setup verifies the ZIP before activation, backs up all ten slots and mods and keeps the main survivor separate from older-version play. An existing portable shortcut follows the managed launcher after an in-game update.

The packaged build passed 456 native checks and both story endings (38 checks each). A fresh survivor watched the crash, played the spoken line, cut all four banks with four actual borer shots and reloaded with the pistol, trench and mouse capture intact. The in-game picker fetched public GitHub releases and returned to pause when the player declined. The installer preserved all ten test slots on version switches. Using a copy of a real survivor, it installed the real 1.12.0 ZIP, switched to 1.13.0, back to 1.12.0 and forward again. The original and the downgraded copy matched byte for byte. A separate copied survivor loaded successfully through the installed Windows launcher. All 42 archived files matched the tested build and installed game. Both Microsoft prerequisites were already installed on the test PC. A clean-machine prerequisite install was not exercised.

## Unreal 1.12.0 — Reclamation

Daylight lasts 52 real minutes, followed by 22 minutes of night. Older saves retain the time of day at which they were last played. Shelter 09 stays safe. Most other towns start with zombies. Clear the streets to reopen trade, then spend candybar wrappers at the settlement board to fund the watch or build gated walls and powered turrets. The board marks the hill water tower on the atlas. Its tap refills at dawn.

A shield takes damage before armor and health. After ten seconds out of combat it recovers two points per second. Press **Q** to treat wounds with a medkit. Defeated enemies spill separate objects that persist when a region unloads. Start within 2.5 metres of one and hold **Ctrl**. The crouched sweep grows to 36 metres in five seconds and respects walls.

Bugs have segmented shells and Supertechs have armored bodies, sensors and rotors. Forests stream grass blades and broadleaf weeds around the player. New color, surface-relief and roughness textures distinguish gold, iron, shale and forest soil. The two highways have broken shoulders and stretches of missing asphalt. Their roadbeds cut through steep ridges on gentler grades. Existing roadside structures and excavations keep their previous ground height when a survivor upgrades.

The title and pause menus include a Mod Workshop for creating, sharing, installing and redeeming custom loot. The game ships with a starter pack and an author kit for people, ChatGPT and other AIs. During play, **Ctrl + Shift + F10** opens the hidden test menu. [Modding guide](../work/earthtotaled-unreal/Content/ModWorkshop/README.md) · [Contributing](../CONTRIBUTING.md)

The Windows game passed 445 in-game checks, including highway grades, preservation of earlier excavations and loading a checkpoint inside a roof. On resume the game checks for room to stand and uses a nearby clear spot when construction or a low ceiling overlaps the standing survivor. A clear checkpoint keeps its exact position. An unmodified copy of a survivor saved on the Shelter 09 roof passed eight upgrade checks through both the installed game and the top-level Windows launcher. Inventory, water, the story and mouse look survived the reload. Both story endings passed 38 checks each. Fullscreen play and restart passed 45 checks. A separate input test pressed Ctrl to crouch, opened the hidden menu, granted supplies and returned to captured mouse look. Bugs and Supertechs dealt damage to each other on both highway fronts.

A 15-minute scripted eastbound trip on the graded roads covered 5.96 km at normal movement speed across 12 streamed regions and two outlying towns. The survivor crossed nine city boundaries, looted four containers, traded for water three times and returned to previously searched streets. Consumed caches stayed empty. All 49 journey checks passed. Only the day and weather clock ran at 13 times normal speed, covering two full cycles and all five weather types.

The final executable completed a second scripted trip west after the rooftop fix. It covered 5.50 km, crossed eight city boundaries, entered two towns, and looted four containers. All 49 checks passed, including collision, day and night, all five weather types and empty caches on the return trip.

The ChatGPT companion produced and copied a pack in a test with a simulated signed-in response. No live model request was made. The in-game workshop separately imported, disabled and redeemed a pack. All 40 files in the Windows ZIP matched the packaged game byte for byte. ZIP SHA-256 `7786a31b2986d5fcd8e0d79c6c5745be1522da334853dc885af53005d37aa1d8`.

## Unreal 1.11.0

City districts, faction settlements, streets, highways and built foundations protect the ground beneath them from both round and square excavations. Existing tunnels remain accessible. Miners seek deposits outside protected footprints. Shallow iron and gold outcrops lie about 350 m northeast of Shelter 09.

Hits produce spatial impact sounds and short body reactions for Bugs and Supertechs, including when they fight each other. The survivor gets a suit impact sound, weapon flinch and a light shove. Survivor XP and the next level threshold appear on the HUD. Press K to inspect a connected perk and choose Learn perk to spend points.

The Windows build passed 417 in-game checks. The checks fired both shapes of the borer at protected land, tested the seven build-piece footprints, fired into both wilderness starter seams, and restored a survivor underground in a fresh process. Combat checks loaded both hit sounds and exercised the reactions on both factions and the survivor. A save round trip retained level, XP and learned perks after two level-ups.

Fullscreen play and restart passed 45 checks, including mouse capture after closing the skill web. A copy of a 10-HP checkpoint remained playable after both borer modes reported protected ground. Each story ending passed 38 checks. Highway runs at Bellows Hive and Parallax City recorded hits by both factions.

All 34 packaged files matched the staged Windows game. Windows ZIP SHA-256 `ea53805c0923f84972c22f0a3b0c0bc26ce86903287acceceb60de10680675cf`.

## Unreal 1.10.0 — Highway War

November 26, 2026. Two four-lane highways connect the original human settlements. Exit ramps lead to Bug hives and Supertech cities. Their names appear on roadside signs, the field atlas and the local district readout. A roadside Bug hive has hollow fluted shells and amber-lit nurseries. A Supertech city has open computing halls, forked spires and a suspended processor inside orbital rings. Both have lootable interiors.

The two factions acquire opposing targets and deal damage with their own attacks. Highway companies fight in daylight and darkness, lose troops and bring in replacements. Daytime highway troops focus on the opposing faction. Roaming enemies and raids still emerge after dark. The player gets credit for their own kills and turret kills. Greenline traders and the HELIOS story uplink retain their roles.

Tap Space to take off. A fresh press in the air gives one free double jump. Holding that press engages the jetpack, or the player can release and press again to thrust. Fuel refills on walkable ground.

The packaged Windows build passed 393 game checks. In separate live encounters at Bellows Hive and Parallax City, each faction damaged the other during the day and again at night. The daytime observer kept full health and armor. The movement test escaped a 14.2 m mined shaft, and both story endings, fullscreen mouse capture and save transfer passed on disposable survivors.

The installed executable also completed a six-minute, 2.39 km westbound trip at normal movement speed with an accelerated world clock. The survivor crossed three forest and city boundaries, looted the outlying city, and stayed on pavement at all 24 sampled highway positions.

## Unreal 1.9.0 — Field Atlas

November 25, 2026. The HUD adds a bearing compass and a north-up minimap. An amber house marks Shelter 09 or the nearest outlying Greenline refuge. A blue diamond tracks a personal waypoint. The **M** map shows settlement buildings, homesteads, street names and terrain contours beyond the loaded area. Players can pan, zoom around the cursor, mark a point, select a city or recenter on their survivor. Up to 32 pins and the tracked destination persist with a survivor and transfer through portable saves.

Blackpine Road and Relay Road connect city streets across the forest. Their surfaces follow the terrain, span streaming boundaries and open up when excavated. Town intersections have painted pedestrian crossings. The map uses the same routes as the world.

While a firearm is aimed, light assistance can correct a near miss against a visible hostile by up to 0.7 degrees. Existing gun spread, recoil, reloads and damage still apply. The pause menu includes a per-survivor switch for the assistance.

The shipping build passed 377 in-game checks. Fullscreen input tests placed, selected and deleted pins, panned the map by dragging, zoomed at the pointer, and restored mouse look on closing the atlas. A fresh process restored the saved pin and aim preference. A rifle shot corrected by 0.344 degrees hit an exposed robot. A wall blocked the correction. Both story endings and the wounded-survivor recovery also passed.

An automated six-minute trip in the installed game covered 2.39 km at normal movement speed. All 24 forest road samples landed on rendered pavement. The city marker switched to the outlying refuge, and the survivor looted a building in that city. All 49 trip checks passed. Slot 01 matched its pre-test checksum after the trip.

All 34 files in the Windows ZIP matched the packaged game. Windows ZIP SHA-256 `772e7e2bcf325ac755769bf96c7bbeb47abb293d55f9e90fe30401ff43f0d8bb`. Shipping executable SHA-256 `e2570a54f37d701a9940ff218de9233e0aeaa690e66d70e6f999acbe6a834431`.

## Unreal 1.8.0 — Night Watch

November 24, 2026. The field guide opens with **F2** from the title screen, pause menu or live game. Opening it with the builder equipped goes straight to harvesting and construction. The HUD shows the current piece, material and controls while building, plus the time until dawn or dusk.

The 48-minute clock governs hostile patrols, predators, underground Brood and raids. They emerge at 19:00 and withdraw at 06:00. A raid interrupted by dawn continues the following night. Deer, settlement trading and turtle miners remain available during daylight.

Trees yield eight wood and stay felled when their region reloads. A new borer cut yields two stone. Walls, floors, ramps and roofs cost four wood, four stone or two iron ore. **T** upgrades an existing frame in place. Undermining the last support collapses connected structures, machines and isolated rock. Ordinary props and loose loot disappear when undermined. Mission items and settlement contacts settle on surviving ground.

The installed Windows build passed 357 in-game checks. A fullscreen run harvested a starter birch with **E**, opened the builder guide with **F2**, and returned to mouse look on closing it. Excavated rock, felled trees and upgraded frames survived reloads. Separate sessions verified both story endings, survivor export and import, and recovery from a wounded checkpoint. The existing taskbar shortcut opens this build. Slot 01 matched its pre-test checksum after installation.

An automated 15-minute expedition from the installed game covered 5.94 km at normal movement speed, crossed nine forest and city boundaries, and looted four containers in two outlying cities. The day and weather clock ran at eight times speed. The survivor lived through two full cycles and encountered all five weather types. Two emptied containers stayed empty when their district loaded again. All 47 expedition checks passed.

All 34 files in the Windows ZIP matched the staged game. Windows ZIP SHA-256 `f5dd2ad4ef246748f984fe818aaaad81dea078cac4bbf0bef29aa37b3982ec7c`. Shipping executable SHA-256 `fd2fbb363b352806c9bef261926b8e82c6368eb1cbee340fabc7e310e013cb4b`.

## Unreal 1.7.1 — Sculptor and checkpoint recovery

November 24, 2026. The 1.7.0 death screen was reproduced by continuing a wounded survivor with a HELIOS patrol in range, before equipping the borer. Patrols now keep their attack timers across saves. Returning survivors have eight seconds to move or heal before enemies can attack them. HELIOS rifles fire visible bolts that can miss or strike terrain and walls.

The death menu can recover the last living survivor at Shelter 09 with gear and world edits intact. A focused replay made two round cuts and two square cuts at the original 10 HP, verified that hostiles could attack again, and reopened the survivor's ore and excavations at the shelter. The package also passed 329 native checks, the underground reload and walk, and the fullscreen start and restart checks.

Windows ZIP SHA-256 `700055470c662d1775163187bc8e93cb1e44ab4edcd86eb43272d145ba279d21`. Shipping executable SHA-256 `5a4cc1f24f592a3f2c17cd17e1ec619a3eb712d5d353677c84ac21a7fe142faa`.

## Unreal 1.7.0 — Outposts

November 24, 2026. The Windows game adds relay-powered auto turrets, turtle miners, three armor upgrades and base-value raids. Armored Glass Brood gather before charging. Experienced survivors also draw skimmers. A successful defense reveals a raidable nest. Underground Brood can open a side chamber into a deep mine. Ore grades increase with depth.

The title and pause menus offer ten survivor slots and portable `.etsave` files. Slot 01 retains the original survivor. **F1** opens the in-game console. Death reports include the cause, hydration and remaining bottles. Original 2K normal, color and roughness maps cover soil, bark, concrete and three mine surfaces. New worlds generate seeded ridges and basins.

The packaged game passed 325 native checks, an underground save reload and a physical mine traversal. Fullscreen keyboard input, mouse recapture, Windows export and import, and reopening the imported survivor passed on this PC. The archive's 34 files match the staged Windows package byte for byte.

Both story endings passed on this build. The automated 15-minute signal run traveled 5.94 km at normal movement speed, crossed nine forest and city boundaries, and looted four containers across two outlying cities. The day and weather clock ran at eight times speed and covered all five weather types. The return trip confirmed that previously emptied containers stayed empty after their district reloaded.

The installed fullscreen launcher reopened slot 10 with captured mouse look. A second export, import and cold restart passed from the installed folder. The existing taskbar shortcut resolves to this build, and the original slot 01 file matched its pre-test SHA-256.

Windows ZIP SHA-256 `b3c2ad75de7e70fd2eb58fc08ba72de34c0950e0caab8c24dbec06bc95ba70c8`. Shipping executable SHA-256 `96864ff84ec20022eda6ef6475afaf60756355e5d2b83a80a4ab856f7c9faa39`.

## Unreal 1.6.0 — Fieldworks

November 24, 2026. HELIOS patrols recur outside settlements, and each destroyed robot hull releases two flying shard drones. The Glass Brood hunts in packs of runners and spitters. Its shells can be recycled into building kits. The ten firearms have unlimited reserve ammunition, timed magazine reloads, and six ranks earned through damage dealt with that gun.

Hold Space to fly the rechargeable jetpack. B opens a construction tool with walls, floors, ramps and peaked roofs. Frames snap to one another, provide cover, and persist when a region reloads. R switches the survey borer between round and square excavation. Square cuts have level floors and vertical walls. The main menu supports continuing a survivor and starting a fresh run after confirmation. Restarting backs up the previous slot.

The final Shipping build passed 293 native integration checks. A scripted survivor drilled a 14.1 m shaft and flew out using normal character movement. Two fullscreen launches exercised the main menu, mouse capture, saved frames, square cuts, surviving drones, magazine counts and gun ranks. The restart test forced an autosave during level travel and verified both the new survivor and the backup. A copy of the existing survivor kept its prior equipment, currency, water, story state and mine edits.

Both endings passed on the final package. The westbound signal expedition covered 3.20 km in eight minutes, reached two outlying cities and looted two containers. The eastbound garden expedition on the preceding combat-identical build covered 5.71 km in 15 minutes, crossed eight forest/city boundaries and confirmed that looted containers stayed empty after returning. Both expeditions used normal movement and survival with the day/night clock at eight times speed. Each passed 47 checks.

The Windows archive is `EARTHtotaled-Unreal-Windows-v1.6.0.zip`. Its SHA-256 is `85a1abd4be09657f04fc77038a5730bdc13d423e010f97dea7bc061f84869602`. The Shipping executable SHA-256 is `62cb21e53a9ee4564ef5542f03d315bf1523bcdb2578d7e5a15161b993a3e00c`.

## Unreal 1.5.0 — Faultline, local Windows build

November 23, 2026. The survey borer cuts persistent shafts and tunnels under forests and paved city streets. Natural galleries, visible iron and gold seams, a surface winch and a marked starter mine are available to new and returning survivors. Workbenches refine iron into alloy. SALT-7 buys gold for candybar wrappers. The scene opens with a PS1-style display that can be switched to native in the pause menu.

The installed Shipping executable passed 264 game checks. A survivor walked into the starter chamber and back out with ordinary movement, reaching 10.86 m below the entrance. A separate game process reopened a save 24.8 m underground with 20 terrain edits, both mined deposits and their inventory intact. Both story endings passed 38 checks each. A copy of the existing survivor loaded in normal fullscreen gameplay and autosaved with the same inventory, wrappers, water, quests and unlocked equipment. The original save retained its previous SHA-256.

All 34 files in the Windows ZIP matched the installed package. The existing launcher points at the verified executable. The local archive is `EARTHtotaled-Unreal-Windows-v1.5.0.zip`, SHA-256 `94ce0fbabbc064ab9d36071cedf26f6445f000dccbcafbf3fd724444e2f5c9bf`. Shipping executable SHA-256 is `a86ed3ef988040bf49278c6e6a63157ad6f77c72f401438bb1c1e1bc798a434e`.

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
