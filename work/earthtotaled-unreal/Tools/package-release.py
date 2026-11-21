"""Bundle the verified Windows archive without source debug symbols."""
from pathlib import Path
import argparse, hashlib, json, zipfile
root=Path(__file__).resolve().parents[1]
outputs=root.parents[1]/'outputs'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-root',type=Path,default=outputs/'EARTHtotaled-Unreal-Windows/Windows')
game=parser.parse_args().game_root.resolve()
assert (game/'EARTHtotaled.exe').is_file(), 'Package the native game first'
instructions='''EARTHtotaled / Weather and Music 1.4.1 / Unreal Windows edition

Double-click EARTHtotaled.exe. Keep this entire folder together.
The game opens fullscreen. F11 switches between fullscreen and a window.
To pin it: start the game, right-click its taskbar icon, and choose Pin to taskbar.

1.4.1 is the rebuilt fullscreen Windows release. F11 toggles windowed mode.
Mouse look needs no held button; closing menus returns control to the game.

New in 1.4: clear skies, overcast, mist, rain and thunderstorms.
Weather changes gradually as the saved world clock advances. Take cover under roofs
to escape the rain and muffle wind. Storms bring occasional lightning and thunder.
Three original stereo music themes crossfade between daytime forest, city and night.
Dialogue lowers the music. Escape > Music and Weather sound adjust or mute each.
These volume choices are saved. Existing survivors and wrapper balances continue.

WASD move / mouse look / Shift sprint / Space jump
Left mouse fire / right mouse aim / R reload / 1-0 or mouse wheel: weapons
E interact / H drink / Q medkit / F flashlight
Tab pack and arsenal / K skill web / J journal / Escape pause

New: ten guns, nine pack items, six kinds of supplies and five workbench recipes.
Find the Drifter .44 at Ranger Supply beside the starting shelter.
Search clinics, grocery shops, salvage depots, civil defense offices and forest camps.
Reloads, footsteps, scavenging, tools, wildlife and day/night environments have sounds.
The starting district is built into the map and fully loaded before play.
Exploration loads nearby terrain and buildings in stages and unloads distant areas.
Escape > View distance switches between 350m, 500m and 700m.
Escape > Render detail switches between native resolution (100%), sharper (125%)
and supersampled (150%). Higher percentages trade frame rate for a sharper image.
New: detailed curved weapons, smoother people and wildlife, beveled architecture,
denser trees and rocks, 2K main surface textures, and much finer nearby terrain.

Wilds and Ruins: branching pines and birches, ferns, shrubs, fallen timber and stumps.
Weathered masonry, broken glazing, partial pitched roofs and exposed trusses.
Rubble, broken walls, rebar and overgrown city yards; 435 starting-area placements.
Twenty spoken lines for settlements, Greenline trade, quests and story archives.
Voices are synthetic, with speaker subtitles and a radio treatment for SALT-7.

Water is essential. Loot houses and sealed relief supplies, or help human settlements.
Robots drop candybar wrappers, never drinking water. Human supply runs trade 12 scrap for 2 water.
Forest, homes, wildlife, traps and more city districts continue in every direction.
A full day and night takes 48 minutes. Menus pause time and restore mouse look on return.

Progress saves automatically. Use Escape > Save and exit before quitting.
Your existing Unreal save continues. Browser-version saves are kept separately.
Original procedural art. Human and animal models are simple rigid models.
'''
(game/'READ ME.txt').write_text(instructions,encoding='utf-8')
archive=outputs/'EARTHtotaled-Unreal-Windows-v1.4.1.zip'
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
    for p in sorted(game.rglob('*')):
        if p.is_file() and p.suffix.lower() not in {'.pdb','.debug','.map'} and 'Saved' not in p.relative_to(game).parts:
            z.write(p,Path('EARTHtotaled-Unreal')/p.relative_to(game))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert 'EARTHtotaled-Unreal/EARTHtotaled.exe' in z.namelist()
record={'version':'1.4.1-unreal','engine':'5.8.4','executable':str(game/'EARTHtotaled.exe'),'zip':str(archive),'zip_bytes':archive.stat().st_size,'zip_sha256':hashlib.file_digest(archive.open('rb'),'sha256').hexdigest()}
(outputs/'EARTHtotaled-Unreal-release.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
print(json.dumps(record,indent=2))
