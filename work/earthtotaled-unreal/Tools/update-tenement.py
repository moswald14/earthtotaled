import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/SM_Tenement.obj')
task.destination_path='/Game/EARTHtotaled/Art'; task.destination_name='SM_Tenement'
task.automated=True; task.replace_existing=True; task.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
mesh=assets.load_asset('/Game/EARTHtotaled/Art/SM_Tenement')
assert mesh
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
assets.save_loaded_asset(mesh)
(root/'Saved/stairs-ready.txt').write_text('Separate stair flights and clear balcony approach imported.\n')
