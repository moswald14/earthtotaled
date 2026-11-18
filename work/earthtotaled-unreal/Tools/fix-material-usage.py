import unreal
from pathlib import Path
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
for path in assets.list_assets('/Game/EARTHtotaled/Materials',recursive=True):
    material=assets.load_asset(path)
    if isinstance(material,unreal.Material):
        unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_NANITE)
        unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
        unreal.MaterialEditingLibrary.recompile_material(material)
        assets.save_loaded_asset(material)
(Path(unreal.Paths.project_saved_dir())/'material-usage-ready.txt').write_text('Nanite and instanced mesh material permutations saved.\n')
unreal.log('ET_MATERIAL_USAGE_READY')
