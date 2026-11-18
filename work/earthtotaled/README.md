# EARTHtotaled — The Last Signal / Wilderness Update (v4)

A standalone single-player survivor FPS, built with Three.js r160 and plain JavaScript.
All visuals and sound are generated locally. No network services are used.

## Edit and build

The main game is `dist/game.js`. Weapon models are in `dist/arsenal.js`, city detail
is in `dist/environment.js`, and the outer districts, interiors, and route metadata
are in `dist/expansion.js`. The original progression graph and its styles are in
`dist/skill-web.js` and `dist/skill-web.css`. Other presentation files are
`dist/style.css` and `dist/index.html`. Three.js and its MIT license are vendored
in `dist/vendor/`.

With Node.js installed, run `npm run build`. No installation step is needed.
The build produces `standalone/EARTHtotaled.html`, which includes everything.
An alternate output directory can be passed to `node build.mjs OUTPUT_DIRECTORY`.

Open the standalone HTML in Chrome or Edge to play. For development, serve
`dist/` with any local static server. A WebGL-capable browser is required.

## itch.io release

Run `npm run build:itch` with Node.js. No dependency installation is needed.
This rebuilds the current game, runs its gameplay and bundle checks, and creates
`outputs/itch-io/EARTHtotaled-itch-v4.1.0.zip` relative to this workspace.
Use `node build-itch.mjs OUTPUT_DIRECTORY` to choose another destination.
The archive has `index.html` at its root and includes the Three.js license.
Upload this release ZIP as an HTML game; the source ZIP is for editing only.
The release folder contains listing copy, upload guidance and verification notes.

To check the local embedded player, serve this source folder and open
`itch-preview.html`. It embeds the packaged entry point from `itch-build/`.
Local checks do not establish that the game has been uploaded or tested on itch.io.

## Endless wilderness (v4.1)

There is no outer fence or movement boundary. Seeded 1,440-meter regions generate
as the survivor travels. Distant regions unload; returning recreates their terrain,
22 houses, supplies, wildlife and traps with stable IDs. Loot, disarmed traps and
harvested animals remain exhausted. Nearby scenery and the terrain-plan cache
are bounded, although saved exploration records grow with the journey.

Adjacent terrain regions share global height samples, so terrain seams match.
The wilderness map follows the player; the city view stays fixed on New Carthage.
The story can still reach its ending, after which exploration continues.
Water survival remains active and sealed household supplies continue generating.

Existing saves retain their world seed and city progress. Distant player and
carcass positions restore without being clamped to the former map boundary.

## Expanded city and traversal

The playable city bounds now span 352 by 352 meters, roughly four times the
original 173-by-173-meter footprint. The old city connects to the western
Riverside district and the eastern and southeastern industrial districts.

Nine buildings marked OPEN have physical doorways and furnished interiors:
Riverside clinic, commons hall, hardware store, canal laundry, East Relay,
civic archive, Cistern pump house, Cistern market, and the near-bunker pharmacy.
The expansion defines 20 floor, roof, mezzanine, and deck support surfaces,
seven marked climbing routes, and 12 indoor or elevated supply lockers.

Climbing is route based. Unlock Climbing Grip, approach a marked ladder, and
press E for an animated automatic ascent. Press E at the marked upper landing
to descend. Over the Edge unlocks the advanced loading-deck scramble route.
There is no free climbing on arbitrary walls. Climbing cannot begin below
15% hydration. Walking off a roof uses gravity and can cause fall damage.

## Lifeline skill web

K opens an original 18-node constellation of connected perks across Mobility,
Combat, Survival, and Salvage. A new or migrated survivor starts with two skill
points and the central Survivor node. Climbing Grip costs one point; Over the
Edge costs another after it. A skill must connect through at least one purchased
neighbor listed in its requirements, and its cost is paid once.

Combat, completed quests, salvage, and first-time building discovery earn XP.
Leveling grants more skill points. Perks change weapon damage, maximum health,
water consumption, reload and movement speed, climbing speed, salvage yield,
medkit healing, and fall damage. The skill web supports mouse and keyboard use.

## People, settlements, and water

Ada keeps Riverside Commons west of the old city. Her medicine quest sends the
player inside the nearby clinic. Malik keeps Cistern Market in the southeast;
his radio quest sends the player up the East Relay service ladder to recover a
rooftop module. These join the existing Nora, Ivo, and Greenline quests.

Each new settlement gives two free water bottles every 120 seconds of active
surface time. Their cooldowns are independent and persist in saves. Talk to the
keeper or interact with the settlement water reserve. Menus and conversations
pause surface time. The visibly contaminated river is not drinking water.

Nora gives a one-time welcome ration and rewards the transit filter quest. Ivo
rewards the botanical pump-fuse quest; afterward, his emergency refills require
an empty bottle inventory and hydration at or below 40%, with a two-minute
in-game cooldown. Water also comes from marked civilian relief drops, interior
supply lockers, and the three core quest rewards. Hostile robots never drop water.

Hydration falls during active gameplay. H drinks one bottle (+40 hydration) at
60% hydration or below. Sprinting increases consumption; below 15%, sprinting
is disabled, walking slows, and climbing cannot begin. At zero hydration, health
drains directly regardless of armor or hit invulnerability. A full base meter
lasts about nine minutes on Survivor while walking; perks can extend it.

## Combat and story

Six weapons unlock through crates, human quests, and Greenline. Scrap buys
workbench upgrades. Hostile robots drop gold only, never water or ammunition.
Trade gold with SALT-7 for ammunition, suit repairs, or the BASTION heavy weapon.
The friendly Greenline quest grants a gun, gold, and defense support, not water.
Friendly actors do not enter the hostile enemy collection.

Recover the transit, botanical, and observatory cores in that order, clearing
nearby guardians before taking each one. Reach the northern uplink and select
one of two endings. Exploration and survival continue afterward. J opens story
records and community quests; Tab opens the expanded survey map.

## Saving

Save schema 3 retains the original localStorage key and migrates schema 1 and 2
saves while preserving weapons, ammo, upgrades, story, and survival progress.
It adds skills, XP, visited interiors, and settlement ration cooldowns.
Roof and upper-floor saves retain elevation. Saving during an animated climb
uses the safe starting anchor, so reloading does not leave the survivor suspended
partway up a wall. Uncollected robot gold also persists with defeated robots.
Saves are browser-local; clearing browser storage can remove progress.

## Verification

`npm test` builds the game and runs the current regression suite. It exercises
real game logic with minimal DOM stand-ins; these checks do not verify rendering
or browser input. The suite covers navigation and collision, combat and loot,
hydration, quest rewards, saves, progression, and the emitted standalone script.

Browser observations during this update confirmed buying skills and ascending
the pharmacy roof through its marked ladder. Earlier browser checks covered the
core controls, weapon pickup, map, pause/continue, survival HUD, human water
rewards, and Greenline dialogue. Embedded-preview pointer lock was unavailable,
so drag-to-look and arrow-key fallback remain available. These observations do
not claim that every expansion route has been manually played end to end.

Fullscreen and mouse look: use FULLSCREEN on the title screen, HUD, or pause menu, or press Alt+Enter. Leaving a workbench or menu attempts to restore mouse capture. If the browser requires another click, use CLICK TO RESUME MOUSE LOOK; failed attempts can be retried. Drag-look and arrow turning remain available.

Browser mouse capture requests Pointer Lock with unadjustedMovement enabled.
Unsupported raw-input options fall back to regular pointer lock. Permission
denials remain retryable by clicking the capture button. Desktop input uses
the native Windows mouse bridge described in desktop/README.md.
