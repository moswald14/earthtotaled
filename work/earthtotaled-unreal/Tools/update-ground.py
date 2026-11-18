"""Import the forest-floor texture polish without reimporting unrelated assets."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
tasks=[]
for suffix in ['BaseColor','Normal','Roughness']:
    task=unreal.AssetImportTask(); task.filename=str(root/('ArtSource/T_Ground_'+suffix+'.png'))
    task.destination_path='/Game/EARTHtotaled/Art'; task.destination_name='T_Ground_'+suffix
    task.automated=True; task.replace_existing=True; task.save=True; tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
for suffix,task in zip(['BaseColor','Normal','Roughness'],tasks):
    if not task.imported_object_paths: raise RuntimeError('Ground texture import failed: '+suffix)
    texture=assets.load_asset('/Game/EARTHtotaled/Art/T_Ground_'+suffix)
    texture.set_editor_property('srgb',suffix=='BaseColor')
    if suffix=='Normal': texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
    assets.save_loaded_asset(texture)
(root/'Saved/ground-ready.txt').write_text('Irregular forest-floor textures imported.\n')
