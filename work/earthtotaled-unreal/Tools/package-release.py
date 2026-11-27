"""Bundle the verified Windows archive without source debug symbols."""
from pathlib import Path
import argparse, hashlib, json, zipfile
root=Path(__file__).resolve().parents[1]
outputs=root.parents[1]/'outputs'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-root',type=Path,default=outputs/'EARTHtotaled-Unreal-Windows/Windows')
parser.add_argument('--output-dir',type=Path,default=outputs)
options=parser.parse_args()
game=options.game_root.resolve()
outputs=options.output_dir.resolve()
outputs.mkdir(parents=True,exist_ok=True)
assert (game/'EARTHtotaled.exe').is_file(), 'Package the native game first'
setup=game/'EARTHtotaled/Content/UpdateTools/EARTHtotaled-Setup.exe'
assert setup.is_file() and (game/'EARTHtotaled/Content/Data/arrival-line.wav').is_file(), 'Installer or opening dialogue is missing'
kit=game/'EARTHtotaled/Content/ModWorkshop'
for name in ('README.md','starter.earthmod.json','mod.schema.json','AI-BRIEF.txt','Create a mod with ChatGPT.cmd','author.cjs'):
    assert (kit/name).is_file(), f'Missing player author kit file: {name}'
instructions='''EARTHtotaled 1.17.0 / Kit's Trail
Look for folded candy wrappers, bent ferns and repeating hoofprints along the shelter road.
Use E to examine field signs and J to hear the recordings again. Rare woodland bears
hunt at night. Listen for the huff and sidestep the swipe or leave their range.
Find Thrum by Shelter 09 and follow Kit's field journal with J. The first herd opens
the road to your car. Four jobs for the same faction reveal the road to Hollow Field.
E boards and exits, R starts, W accelerates, A/D steer and S brakes. In the plane,
Space climbs and Ctrl descends. Come to a stop on the runway to exit.
The two permanent choices in the vault lead to separate routes into custody.
Both friends stay with you. Herd restorations change Kit's homecoming.


Two loops serve sixteen cities. The North Shuttle connects Kiln Exchange and Windfall
Reach. The South Shuttle connects Signal Heights and Saltmarket. Both run both ways.
Press M then R for the network or L for the nearest station district and its shops.
Click a building to track it. Walk inside, aim at the stock and press E to search.
Jump off the deck and press E at the rear of the platform to take the lift down.
Follow Street lift on the local atlas and press E at the curbside sign to go up. At the yellow edge E waits for the next train.

Double-click EARTHtotaled.exe. Keep this entire folder together.
Install or update from https://github.com/moswald14/earthtotaled/releases/latest
Run EARTHtotaled-Setup.exe and choose a version. Setup installs missing Microsoft
prerequisites, backs up all ten slots and keeps older versions on separate save
copies. Inside the game choose Game updates from the title or pause menu. Pin
EARTHtotaled from the Windows Start menu for a shortcut that follows updates.
You can back up any slot under Survivor saves > Export.
The game opens fullscreen. F11 switches between fullscreen and a window.
To pin it: start the game, right-click its taskbar icon, and choose Pin to taskbar.

LANDFALL
A fraudCORP flight from 1986 crash-lands at SHELTER 09 in 2253. New survivors
receive the Drifter .44 and a survey borer. Press G and fire at each of the four
trench stakes. Ground Bugs cannot cross the cut banks. Settled cities stay clear.
Hostiles have a 1-in-400 chance to drop a unique gun with two perks, rolled stats
and an upgrade path. Collect it with Ctrl. Inspect it in Tab > Unique gun rack.

RECLAMATION
Shelter 09 is safe. Most other towns have zombies. Clear the streets, then use
the settlement board to fund watch shifts, build walls and add powered turrets.
Boards can mark a hill cistern. Draw bottles at its tap. It refills each dawn.
The shield takes incoming hits first and recharges after ten quiet seconds.
Use Q with a medkit to heal. Stand beside fallen loot and hold Ctrl to crouch.
The pickup radius grows to 36 metres in five seconds and needs a clear path.
Mod workshop is on the title and pause menus. Preview the starter or use your
own ChatGPT to create a loot pack. Open Author kit for the complete guide.
For testing use a spare survivor slot and press Ctrl + Shift + F10 during play.

M opens your north-up field atlas and pauses survival. Drag with the left mouse
to pan, and scroll to zoom around the cursor. Right-click to drop a blue pin.
Click a pin to track it or press Delete to remove it. C tracks and frames the
nearest city, Home centers on you, and M or Escape returns to mouse look.
The amber house on the compass and minimap guides you to a settlement.
H7 Blackpine Highway and H2 Relay Highway connect human refuges. Take a signed
exit for a Bug hive or Supertech city. Click a faction landmark on M to track it.
Both factions fight over the exits throughout the day and night.
Light aim assist applies only while aiming a firearm at a visible hostile.
Change Aim assist in the pause menu if you prefer.

Continue survivor loads your existing game. Escape > Save and return to main menu
lets you switch to New survivor. Confirming a restart backs up the last survivor
and begins again at Shelter 09. The same menu is available after death.

Continuing a living checkpoint gives you eight seconds to get to cover or use Q
for a medkit. Supertech patrol shots travel through the world and can be dodged.
After death, Recover at Shelter 09 brings back your last saved survivor and
their gear and tunnels, with medical treatment at the shelter.

Tap Space to jump and press it again in the air for a free double jump.
Hold the second press or press again to thrust. Landing recharges jet fuel.
F2 opens the field guide. Its three tabs cover controls, construction and survival.
Press E at a tree to fell it for 8 wood. Start with the birches beside Shelter 09.
B equips the builder. Scroll for a piece, press C for material, and R to rotate.
Left mouse places a green preview. Right mouse salvages the aimed piece. Press T
to upgrade an existing frame from wood to stone, then metal. Each wooden frame
costs 4 wood or one kit and has 160 integrity. Stone costs 4 stone and has 320.
Metal costs 2 iron ore and has 600. Each successful borer cut gives 2 stone.
Frames and machinery collapse when their last connection to solid ground goes.
Excavated rock breaks away when its last connection to bedrock is cut.
Salvage pieces from the top down if you want to reclaim their materials.
SALT-7 sells six wood frame kits for twenty wrappers. Human supply runs also award kits.
With the survey borer equipped, R switches between round and square tunnel cuts.

From 19:00 until 06:00, roaming Supertech patrols, Bug raiders and predators emerge.
Infested cities and faction front lines stay dangerous by day.
Use daylight to harvest and build. Deer, settlers and Greenline stay in the world.
At dawn roaming hostiles withdraw and raids pause. Highway factions keep fighting.
Each destroyed Supertech hull releases two flying shard drones. Take cover from
their fire. Bug runners close in while spitters attack from a distance.
All ten guns have unlimited reserve ammunition. Magazines still need time to
reload. Dealing damage to hostiles upgrades that particular gun automatically.

G equips or stows your survey borer. Left mouse blasts wilderness earth and cave
walls. Hold right mouse and sweep to locate a workable vein. Your borer recharges after
each pulse. Press V underground with the borer equipped to winch to the surface.
City districts, both faction settlements, roads and built foundations are protected
all the way down. Shallow iron and gold lie about 350 m northeast of Shelter 09.
Tunnels and depleted veins are saved. Existing tunnels are still accessible.
The K skill web shows the next XP threshold. Select a perk to inspect it and
choose Learn perk to spend points. Each survivor level grants one point.
Two iron ore make one alloy sheet at a workbench. SALT-7 pays 12 candybar
wrappers for one gold ore. Escape > Display switches between PS1 and native.

WASD move / mouse look / Shift sprint / tap Space to jump / hold Space to jet
Left mouse fire / right mouse aim / R reload / 1-0 or wheel to select a gun
E interact / H drink / Q medkit / F flashlight
Tab pack and arsenal / K skill web / J journal / Escape pause

Visit Ranger Supply beside the starting shelter for another weapon.
Search clinics, grocery shops, salvage depots, civil defense offices and forest camps.
Find drinking water in houses and relief supplies, or finish quests for people.
Supertech hulls drop candybar wrappers. A human supply run trades 12 scrap for two
water bottles and two building kits. Search armories and complete quests for guns.

The journal follows your survivor through both possible endings. You can keep
exploring afterward and return to HELIOS to reread your chosen epilogue.
Daylight lasts 52 minutes and night lasts 22 minutes. Roofs provide cover in thunderstorms.

Escape opens the display and audio settings. View distance ranges from 350 to
700 m. Render detail ranges from 100% to 200%. The music and weather sounds
have separate volume controls. Moving the mouse turns the view when a menu closes.
Progress saves automatically and when you return to the main menu.

OUTPOSTS
Mine iron and gold with G. Build a relay (2 iron, 1 gold), an auto turret
(3 iron, 1 gold) or a turtle miner (4 iron, 1 gold). Turrets need a relay
within 75 metres and a clear line of fire. Miners drill one local deposit
per 30 seconds while the surrounding region is active. Press E to collect.
Ore grades rise every 16 metres underground. Look out for armored Bugs.
At the relay press E to repair, upgrade suit plating, or trace a Bug nest
after a successful defense. Outposts of value 27 and up draw a warning and
an approaching line of raiders. Stronger survivors encounter flying skimmers.

Survivor saves is available from the main menu and pause. Choose one of ten
slots to play, start a new survivor, export an .etsave file, or import one
from another PC. Pause also offers Copy this run here. Occupied slots require
confirmation before replacement. Slot 01 retains the original survivor.
Press F1 in the world to open the field console. Type help for commands.
Enter runs a command, Up recalls it, and Escape returns to mouse look.
Escape > Display / native shows the new 4K material detail at full resolution.
'''
(game/'READ ME.txt').write_text(instructions,encoding='utf-8')
archive=outputs/'EARTHtotaled-Unreal-Windows-v1.17.0.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
    for p in sorted(game.rglob('*')):
        if p.is_file() and p.suffix.lower() not in {'.pdb','.debug','.map'} and 'Saved' not in p.relative_to(game).parts:
            z.write(p,Path('EARTHtotaled-Unreal')/p.relative_to(game))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert 'EARTHtotaled-Unreal/EARTHtotaled.exe' in z.namelist()
import shutil
installer=outputs/'EARTHtotaled-Setup.exe'
shutil.copy2(setup,installer)
record={'version':'1.17.0-unreal','installer':str(installer),'installer_sha256':hashlib.sha256(installer.read_bytes()).hexdigest(),'engine':'5.8.4','executable':str(game/'EARTHtotaled.exe'),'zip':str(archive),'zip_bytes':archive.stat().st_size,'zip_sha256':hashlib.file_digest(archive.open('rb'),'sha256').hexdigest(),'published':False}
(outputs/'EARTHtotaled-Unreal-release.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
print(json.dumps(record,indent=2))
