# EARTHtotaled — The Last Signal / Unreal port

This is EARTHtotaled's native Unreal Engine 5.8 Windows game. The original browser source is preserved in the sibling `earthtotaled` folder. See the [release record](../../docs/RELEASES.md) for the build and playtest evidence, and the [full-release checklist](../../docs/RELEASE-CHECKLIST.md) for remaining verification.

## Build and run on Windows

Requirements: Epic's Unreal Engine 5.8, Visual Studio 2022 17.14+ or Visual Studio 2026 C++ build tools, Windows SDK, and the .NET Framework 4.8 SDK/targeting pack. The installed engine location defaults to `C:\Program Files\Epic Games\UE_5.8`.

From this project folder:

```powershell
node Tools/export-legacy.mjs
python Tools/make-art.py
./Tools/make-voices.ps1
python Tools/make-starting-area.py
python Tools/check-project.py
./Tools/check-terrain.cmd
./Build-Unreal.ps1 -Mode Content
./Build-Unreal.ps1 -Mode Play
./Build-Unreal.ps1 -Mode Package
```

`Content` compiles the editor module, imports original source meshes, textures and sounds, creates the map, and records success only after Unreal finishes. `Package` builds a Windows game in `outputs/EARTHtotaled-Unreal-Windows`. Keep all packaged files together; pin the packaged game executable after opening it. Do not replace the existing executable with an editor binary.

To check a separate candidate build, run `./Tools/verify-packaged.ps1 -GameRoot <candidate-Windows-folder> -QaRoot <temporary-QA-folder>`. It runs the packaged game with a transient survivor and an isolated user directory. `python Tools/package-release.py --game-root <candidate-Windows-folder>` can archive that candidate after verification.

For an on-screen mouse check, launch the Shipping executable with `-ETReview=bench -ETInputAudit -UserDir="<temporary-QA-folder>"`. This opens a workbench in a temporary session that does not load or save the player's survivor. The opt-in `Saved/input-verification.txt` records capture state and mouse axes in Shipping builds, where ordinary diagnostic logging is stripped. Move the mouse without holding a button after returning from menus and compare the recorded camera yaw. The audit is off during normal play and bounded to 4,096 entries.

## Controls

WASD move; mouse look; Shift sprint; Space jump; left mouse fire; right mouse aim; R reload; 1–0 or mouse wheel switch owned weapons; Tab pack/arsenal; E interact; H drink water; Q medkit; F flashlight; K skill web; J journal; Escape pause/resume; F11 fullscreen/windowed.

## Scavenger update / 1.1

Four new guns bring the arsenal to ten: Drifter .44, Ashfall Auto-12, Needler PDW and Ranger Bolt Rifle. Ranger Supply beside the shelter has a guaranteed Drifter. More weapon cases appear in wilderness camps and city depots; civil defense buildings keep a Ranger upstairs. The pack lists owned weapons and supplies, with click-to-equip and click-to-use controls.

Six container families provide different supplies: pantry, medical, tools, electronics, armory and relief. Nine new pack items include crafting materials, bandages, trail rations, suit plates and endurance injectors. Five workbench recipes turn salvaged materials into medicine, ammunition, armor and injectors. Recipes check all ingredients before consuming anything. Container IDs remain stable across unloading and saves.

The starting district is authored offline with 267 placements and baked as real mesh actors into ExclusionZone.umap. It includes shops, apartments, a depot, fuel stop, pocket park, lamps and shelter square. Surrounding terrain and content within the view radius preload before the player spawns. Exploration generates regions in separate terrain, vegetation, building, encounter and detail stages, prioritised by distance to tile bounds. A buffer avoids unloading/reloading at the same boundary. Distant AI sleeps and scenery is distance-culled. View distance is selectable in pause (350/500/700 meters), saved with the survivor. Generation runs on the game thread in stages; the time budget is a soft limit because a single stage may exceed it.

Outlying cities have clinics, groceries, salvage depots, fuel stations, civil defense armories and Greenline relief camps. Original ambient day forest, night forest and city loops blend by place and time. Foley covers footsteps, reloads, pickups, water, crafting, healing and encounters; muzzle light and hit markers provide firing feedback. All 25 sounds are original generated audio, with no third-party recordings.

The game starts in borderless fullscreen. Workbenches and menus pause time and release the cursor. Return to the world restores permanent native mouse capture and viewport focus, with a delayed retry after the menu click has completed. Movement does not require holding a mouse button.

## Detail update / 1.2

Rendering now defaults to full native resolution (formerly 85%). Pause > Render detail offers 100%, 125% and 150%, saved per survivor. On a 1920x1080 display these request 1920x1080, 2400x1350 and 2880x1620 scene rendering. The display remains fullscreen at the monitor's resolution. Dynamic resolution is disabled and anisotropic filtering is 16x; higher render percentages cost performance.

All ten guns now have 4,064–10,104 source triangles, beveled receivers, rounded barrels, controls, rails, trigger guards and separate surface materials. People and wildlife have rounded bodies and articulated-looking parts, but remain rigid models. Buildings have beveled edges and window trim, trees have denser branching, and boulders have detailed contours. Explicit shading normals and corrected planar UV projection replace the old hard faces and stretched floor UVs. Main surfaces use 2048px maps, other surfaces 1024px. Named material slots preserve metal, wood, rubber, skin and lenses during both runtime spawning and baked-map import.

Terrain near the player uses 160x160 cells (51,200 triangles per tile, 16x the original); farther tiles use 80x80 (12,800 triangles, 4x). Detail follows the player with a retention band. All tile edges share a common boundary curve to prevent cracks. Height sampling is spread across frames, and the existing surface stays in place while a replacement is prepared. Nanite manages suitable static-mesh detail at distance; runtime terrain remains a conventional procedural mesh.

`-ETReview=forest -ETPerfTest` opens a disposable rendered test, samples a stationary view for 25 seconds after a five-second warmup, writes `Saved/graphics-performance.json`, and pauses. This is a narrow frame-timing sample, not a long exploration benchmark. Review sessions have a visible warning and never overwrite the survivor save.

## Weather and Music / 1.4

Clear skies, overcast, mist, rain and thunderstorms follow the saved survival clock. Fronts change about every five real minutes with 45-second blends, and continue indefinitely. Clouds move overhead; sunlight, sky colour and haze respond to each front. Rain uses 256 reusable instances near the survivor, with roof collision checks. Taking cover muffles weather sounds. Storms produce occasional single lightning fades and delayed thunder. Rain does not change water rewards or refill sealed bottles.

Three original stereo score loops play continuously: **Embers in the Canopy** in daytime forest, **Concrete Echoes** in daytime city districts, and **The Last Frequency** at night. Travel crossfades the score; spoken dialogue lowers it. Each composition is 64 seconds and loops without a silent gap. Rain and wind loops and an eight-second thunder roll are synthesized original sounds. No third-party songs or samples are used.

Escape opens separate **Music** and **Weather sound** controls, including mute. Choices are saved; old saves start at 50% music and 75% weather sound. Pausing stops survival, fronts, rain movement and thunder timers while the music continues quietly. The game now contains 51 sounds, including the existing voices, weapons and ambience.

`python Tools/make-atmosphere.py` generates the score, weather audio and cloud texture; `python Tools/check-atmosphere.py` verifies audio format, duration, stereo width, levels and loop boundaries. Run `Tools/import-atmosphere.py` inside Unreal Editor for a selective import. Full art/content builds also include these assets. Temporary review sessions accept `-ETWeather=clear|overcast|mist|rain|storm`; normal saves ignore this override. Review sessions record actual audio channel playback in `Saved/atmosphere-playback.txt` after eight seconds.

## Candybar-wrapper currency / 1.3.1

Candybar wrappers are the currency. Hostile robots drop folded printed wrappers, and quests award wrappers. Greenline accepts them for ammunition, repairs and weapons. The HUD, shops, quest rewards, skill web and spoken replies use the new currency. Existing balances, prices and saved drops carry over one-for-one; the internal `Gold` field and `gold` drop key remain only for save compatibility. Water rules are unchanged.

Regenerate the pickup with `python Tools/make-wrappers.py`, regenerate spoken replies with `Tools/make-voices.ps1`, then import them with `Tools/import-currency.py` in Unreal Editor. A full art rebuild also includes the wrapper mesh.

## Wilds and Ruins / 1.3

Pines now have curved trunks, irregular branches and individual needle surfaces rather than solid stacked cones. Two pine shapes mix with birches and dead trees. Ferns, shrubs, fallen logs, stumps and patches of forest-floor colour break up the ground. Small clutter is instanced, non-blocking and limited to shorter viewing distances.

Houses and apartments have weathered plaster, exposed brick, chipped trim, rain pipes and broken glazing. Houses retain partial pitched roofs with exposed rafters and missing sheets; ladder landings and rooftop quest routes remain clear. Apartments have broken parapets and exposed reinforcement. Shops have torn fabric awnings, and warehouses expose roof trusses through missing panels. Rubble, broken walls, rebar and vegetation reclaim city yards and pavement edges. The starting district now has 435 baked placements.

Twenty spoken lines cover settlement greetings, water and supply rewards, quest replies, SALT-7 trade and four story archive moments. They use an installed Windows synthetic voice, with radio treatment for SALT-7, not recorded actors. Speaker-labeled subtitles work during paused conversations; one audio channel prevents overlapping replies. `Tools/make-voices.ps1` and `Tools/finish-voices.py` reproduce the 20 clips from `Content/Data/dialogue.json`. Existing 25 weapon/foley/ambient clips remain, for 45 sounds in total. `-ETReview=voice` opens a disposable SALT-7 conversation.

`python Tools/environment-art.py` regenerates the changed environment meshes and textures without rebuilding the weapons. Add `--meshes-only` to keep textures unchanged. The full make-art generator also runs this environment pass. Foliage uses two-sided lighting and Nanite area preservation, following Epic's [foliage guidance](https://dev.epicgames.com/documentation/unreal-engine/working-with-naniteenabled-content?lang=en-US).

## Endless expeditions

A full day and night takes 48 real minutes. Sun, moon, sky and fog change as time passes; the flashlight is unlimited. Menus pause the clock. World time, position, inventory, quests and emptied caches are saved automatically and on exit.

The starting shelter lies in Exclusion City. Walk in any direction to reach pine forest, charred trees, wildlife, traps and abandoned homes. Beyond the forest, more ruined districts appear roughly every 1.6 km. Their apartments have three accessible storeys with exterior stairs. New regions keep generating while distant regions unload; there is no fence or designed end of the map.

Humans offer a repeatable supply run: bring 12 scrap for two water bottles and 18 rounds. Emergency rations remain available when supplies run out. The same container stays emptied after leaving and returning; keep travelling for fresh loot. Robots drop candybar wrappers, never water.

## What is carried forward

The design export preserves the actual six weapon specifications, eighteen original perk nodes and connections, five community quests, four chapters and six recordings. Mara Venn, Eli, HELIOS, Nora, Ivo, Ada, Malik and SALT-7 retain their roles. The three cores lead to a final irrigation or warning choice; exploration continues afterward.

Runtime source includes hydration, armor, ammunition, scavenging, workbench repairs, quest rewards, community water, Greenline trade/support, climbing, seeded terrain streaming, enterable houses, wildlife, traps, and a native SaveGame. Robots yield candybar wrappers and never drinking water. Water can come from human rewards, sealed relief supplies and house caches.

Unreal saves use a separate native slot. The 1.1 update continues existing native saves and retains original weapon indices. New inventory fields default empty. Browser/WebView saves remain separate and are **not automatically imported**.

## Graphics

The project configures Lumen lighting/reflections, virtual shadow maps, DX12/SM6 and temporal super resolution. Original meshes include open doorways, windows, partitions, furniture, roof details, ladders, trees, rocks, grass, weapons, robots and animal stand-ins. Original PBR texture sets have base color, normal and roughness maps. No third-party or marketplace assets are included.

Runtime procedural terrain uses ordinary mesh rendering and collision; it does **not** automatically gain Nanite or mesh distance fields. The original character and animal models are simple rigid models rather than finished skeletal animation. This is a playable independent game with procedural art, not photorealistic production art.

## Checks

`check-project.py` checks design integrity, perk reachability, mesh indices and triangles, actual geometric doorway/window openings, PNG checksums and unique weapons. The native standalone terrain check compiles the same terrain functions the game uses and checks seams, negative coordinates, deterministic reloads and distant terrain. Neither check substitutes for Unreal compilation, editor import, a rendered playtest or packaged-game testing.

Run `Tools/verify-packaged.ps1`, or launch a development or packaged game with `-ETSelfTest -unattended -nosound`, to run the native integration checks. Graphics must be enabled: Unreal discards render mesh data in a cooked `-nullrhi` session, which cannot validate triangle counts. The report is written to `Saved/native-verification.txt`. These use a transient save and never overwrite the player's slot. Optional `-ETReview=bench`, `forest`, `city`, or `night` launches a disposable visual review. Development builds support `-ETInputAudit` for raw mouse and capture diagnostics.
