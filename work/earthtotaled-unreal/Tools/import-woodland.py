from pathlib import Path
import runpy
import unreal

runpy.run_path(str(Path(unreal.Paths.project_dir()).resolve()/'Tools/import-outposts.py'),init_globals={'woodland':True})

# These meshes are placed by the native grass streamer, so the cooker never sees
# a level instance from which to infer the required material permutations.
editor=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
materials={}
for name in ('BladePatch','RoadWeed','CedarFall','AmberCaps','BentBracken','ConeBed'):
    mesh=editor.load_asset('/Game/EARTHtotaled/Art/SM_'+name)
    for slot in mesh.get_editor_property('static_materials'):
        material=slot.material_interface
        if not material: raise RuntimeError('Unbound forest material on '+name)
        materials[material.get_path_name()]=material
for material in materials.values():
    unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(material)
    editor.save_loaded_asset(material)
unreal.log('WOODLAND_STREAMING_MATERIALS_READY '+', '.join(sorted(materials)))
