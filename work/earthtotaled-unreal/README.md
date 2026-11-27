# EARTHtotaled — The Last Signal / Unreal port

This is EARTHtotaled's native Unreal Engine 5.8 Windows game. The original browser source is preserved in the sibling `earthtotaled` folder. See the [release record](../../docs/RELEASES.md) for the build and playtest evidence, and the [full-release checklist](../../docs/RELEASE-CHECKLIST.md) for remaining verification.

## Reclamation / 1.12

Version 1.12 brings a 52-minute day, 22-minute night, persistent zombie clearances, gated town walls, powered sentries and refillable hill cisterns. Read a settlement board to track the next infestation or the town's water tower. Combat drains your shield first. Use medkits for wounds and hold Ctrl near spilled loot to expand the pickup radius.

The title and pause menus include a Mod Workshop with personal ChatGPT authoring, a browser workflow, clipboard preview and local loot packs. [Modding guide](Content/ModWorkshop/README.md) · [Contribution guide](../../CONTRIBUTING.md)

Bugs have articulated shells and jointed limbs. Supertechs have layered plates, lenses and rotors. The forest floor has individual folded grass blades and broadleaf weeds. Weather changes how much light reaches the surface. Gold, iron and shale have distinct 2048-pixel color, normal and roughness maps. Weathered highway slabs sometimes give way to exposed ground. Intercity roads cut through hills, with sloped embankments blending into the steeper wilderness. Regenerate the meshes with `python Tools/make-reclamation.py` and their surface maps with `node Tools/make-relief.mjs`.

## Survivor progression / 1.11

Version 1.11 adds protected ground, combat hit reactions and a survivor XP display. Press **K** to inspect the skill web and learn connected perks. Every level grants one point. A wilderness cut earns 2 XP, an ore strike earns 12 XP and felling a tree earns 6 XP. The borer protects every depth beneath settlements, roads, homesteads and player structures.

## Highway War / 1.10

H7 Blackpine Highway and H2 Relay Highway have four lanes, marked exits and roadside colonies. Follow the exit boards to the Bug hives or the Supertech tower cities. Both send fighters onto the approaches around the clock. At night the troops can also pursue a survivor. The amber refuge marker still leads to people and water. Click a colony on the field atlas to track it.

Tap **Space** to jump, then press it again in the air for a free double jump. Hold the second press or press once more to thrust. Fuel recharges when your boots touch down.

## Field Atlas / 1.9

The compass and north-up minimap show the nearest city's refuge in amber and your tracked pin in blue. Press **M** for the field atlas. Scroll to zoom at the cursor, drag to pan, and right-click to place a pin. Click a marker to track it. **Delete** removes the selected pin, **C** frames the nearest city and **Home** centers on you. Map pins travel with your survivor through saves and portable files.

Blackpine Road and Relay Road connect neighboring city grids through the forest. The map shows these routes, city streets, settlements, buildings and elevation contours. Hover over a street to read its name. Hold right mouse with a firearm for light aim assist on visible hostiles. The pause menu can switch it off.

## Night Watch / 1.8

Press **F2** for the field guide, or open it from the title or pause menu. Roaming hostile patrols, predators and the Glass Brood emerge at **19:00** and withdraw at **06:00**. Zombies stay in infected towns during daylight as well. Deer and settlement traders stay out during the day. An unfinished raid resumes the next night.

Aim at a tree and press **E** for eight wood. New survivors can harvest birches beside Shelter 09. With **B** equipped, the wheel chooses the piece, **C** selects the material and **R** rotates. Left-click places the preview, right-click salvages, and **T** upgrades an existing frame one tier. A piece costs four wood, four stone, or two iron ore, with 160, 320 and 600 integrity respectively. Every successful borer cut yields two stone. Salvage kits can pay for wooden frames.

Remaining foundations can carry a deck across a gap. Destroying or salvaging the last support collapses connected frames and their machinery. Solid cave roofs can span wilderness tunnels, and a piece of rock breaks away when its last connection to bedrock is cut. Ordinary loose scenery and loot can fall away. Settlers and story items move to the surviving floor of an excavation.

## Outposts / 1.7

Equip the borer with **G** and mine iron and gold. Press **B**, scroll to a machine and click to place it. A relay costs 2 iron and 1 gold ore, a turret costs 3 and 1, and a turtle miner costs 4 and 1. Turrets draw power within 75 m of a relay and track visible hostiles within 34 m. A turtle works a finite deposit every 30 seconds in a loaded region. Press **E** on its shell to empty its hopper.

At 27 outpost value, the Brood can muster a carapace line near the relay. The warning gives you 16 seconds before the charge. Press **E** at the relay to buy stronger armor or trace a nest after a successful defense. Stronger survivors draw flying skimmers. Deep tunnels attract burrowing bulwarks and the ore grade rises every 16 m.

Open **Survivor saves** from the title or pause menu to manage ten independent worlds. Slot 01 holds the original survivor. **Export file** writes a portable `.etsave` to a folder you choose. **Import file** restores it to the selected slot. Portable files support up to 32 MB. Replacing a survivor backs up its previous contents. In the field, press **F1** and enter `help` for local diagnostics and admin commands. Escape closes the console and returns to mouse look.

The new terrain uses seeded drainage and ridge shapes. A returning survivor retains the original ground around saved excavations and structures. Soil, bark, concrete, shale, iron and gold use 2K color, roughness and normal maps. The pause menu includes the native-resolution display.

Run `node Tools/make-relief.mjs` and `python Tools/make-outposts.py` to regenerate the original art. Unreal imports it with `Tools/import-outposts.py`. `Tools/verify-outposts.ps1 -GameRoot <Windows folder> -QaRoot <new test folder>` exercises the packaged console, file dialogs, slot switching and a cold restart.

## Fieldworks / 1.6

Use the double jump and hold **Space** to thrust. Land to recharge. **B** opens construction. Choose a wall, floor, ramp or roof and aim at a solid footing or a frame to snap pieces together. Search armory crates, craft fittings, trade with SALT-7, or bring scrap to human settlements to replenish wooden frame kits.

With the borer equipped, **R** selects round or square excavation. Square cuts share level, grid-aligned faces. All ten guns have unlimited reserve ammunition and timed magazine reloads. Each gun advances independently at 150, 450, 1,000, 2,000 and 4,000 hostile damage, gaining 15% base damage per rank.

Roaming HELIOS patrols break into flying shard drones. The Glass Brood has melee runners and ranged spitters. Escape returns to the main menu through **Save and return to main menu**. **New survivor** asks for confirmation and saves the previous slot under the same name with the suffix `-previous`.

Run `Tools/verify-fieldworks.ps1` with a packaged Windows game folder and a fresh QA folder to exercise the fullscreen menu, jetpack, excavation, saved construction, continuing a game, and confirming a restart across two launches.

## Faultline / 1.5

Read the survey sign at the old shelter mine. From Shelter 09, follow bearing 067 for about 350 m to reach the first wilderness gold and iron outcrops. **G** equips or stows the borer, **left mouse** excavates, and **hold right mouse** surveys for ore. Each round pulse cuts a roughly 5.7 m cavity. Aim into a wall for a horizontal drift or angle down for a ramp. **V** winches an underground survivor to supported ground. Gold shows as yellow seams and iron as rust-red bands. SALT-7 buys one gold ore for 12 wrappers. A workbench refines two iron ore into an alloy sheet.

In open wilderness, the borer reveals seeded underground galleries and leaves floors and ceilings over tunnels. Settlements, roads and buildings protect the rock below. Tunnels and depleted deposits travel with the survivor save. The PS1 display uses a 270-line dithered scene with a native-resolution HUD. Escape opens the display switch for the full-resolution view.

`Tools/verify-mining.ps1 -GameRoot <Windows folder> -QaRoot <test folder>` exercises the packaged borer, collision, protected streets, trading, refining, streaming and deep mining. It then reopens a dedicated underground save in a second game process. To regenerate the new art, run `python Tools/make-geology.py` followed by `Tools/import-geology.py` in Unreal Editor.

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

For story and expedition checks, run `./Tools/verify-journey.ps1 -GameRoot <candidate-Windows-folder> -QaRoot <temporary-QA-folder> -Ending garden -Seconds 900`, then repeat with `-Ending signal` and a separate QA folder. The story scenario interacts with actual spawned cores, settlements and HELIOS through clear, in-range collision traces. It stages the player beside story sites, so it is not a manual playthrough between those sites. The expedition then uses normal character movement, collision, AI, ammunition and water while entering city houses, scavenging and returning through streamed regions. The expedition advances the day and weather clock at 13 times normal speed. Movement and survival run at normal speed.

`-StoryOnly` skips the expedition. `-Resume -StoryOnly` loads that branch's test save in a new process and checks the saved epilogue and consumed story objects. Test saves use `EARTHtotaled-QA-garden` or `EARTHtotaled-QA-signal` inside the selected QA directory; the normal survivor slot is never read or written by these scenarios. Reports and five-second travel/memory samples are written under `Saved`. The runner keeps its window hidden and sound muted: these are runtime checks, not a listening test or a foreground frame-rate guarantee.

## Controls

Use **WASD** to move, the mouse to look, **Shift** to sprint and **Space** to jump. Left-click to fire, right-click to aim and press **R** to reload. Choose an owned weapon with **1–0** or the mouse wheel. **Tab** opens the pack and arsenal.

Use **E** to interact. Hold **Ctrl** near spilled loot to crouch and collect it. **H** drinks water, **Q** uses a medkit and **F** toggles the flashlight. Open the skill web with **K**, the journal with **J** and the pause menu with **Esc**. **F11** switches between fullscreen and a window.

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

A full day and night takes 74 real minutes. The sun, moon, sky and fog change as time passes. The flashlight is unlimited. Menus pause the clock. World time, position, inventory, quests and emptied caches are saved automatically and on exit.

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
