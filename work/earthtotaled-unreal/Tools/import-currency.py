"""Import the wrapper pickup and revised spoken dialogue without rebuilding the scenery."""
from pathlib import Path
import unreal, json
root=Path(unreal.Paths.project_dir()).resolve()
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/SM_Wrappers.obj'); task.destination_path='/Game/EARTHtotaled/Art'; task.destination_name='SM_Wrappers'
task.automated=True; task.replace_existing=True; task.save=True; task.factory=unreal.FbxFactory()
options=unreal.FbxImportUI()
options.set_editor_property('automated_import_should_detect_type',False)
options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
options.set_editor_property('import_materials',False); options.set_editor_property('import_textures',False)
data=options.static_mesh_import_data
data.set_editor_property('combine_meshes',True); data.set_editor_property('auto_generate_collision',True)
data.set_editor_property('generate_lightmap_u_vs',False); data.set_editor_property('convert_scene',False)
data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
data.set_editor_property('build_nanite',False); task.options=options
tasks=[task]
for source in sorted((root/'ArtSource').glob('S_Voice_*.wav')):
    voice=unreal.AssetImportTask(); voice.filename=str(source); voice.destination_path='/Game/EARTHtotaled/Art'; voice.destination_name=source.stem
    voice.automated=True; voice.replace_existing=True; voice.save=True; tasks.append(voice)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for imported in tasks:
    if not imported.imported_object_paths: raise RuntimeError('Currency import failed: '+imported.filename)
mesh=assets.load_asset('/Game/EARTHtotaled/Art/SM_Wrappers')
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
slots=mesh.get_editor_property('static_materials')
for i,slot in enumerate(slots):
    name=str(slot.material_slot_name)
    mat=next((x for x in ['Canvas','Signal','Greenline','Steel'] if name==x or name.endswith('_'+x)),'Steel')
    mesh.set_material(i,assets.load_asset('/Game/EARTHtotaled/Materials/M_'+mat))
assets.save_loaded_asset(mesh)
p=root/'Saved/graphics-import.json'; d=json.loads(p.read_text()); d['SM_Wrappers']={'slots':[str(x.material_slot_name) for x in slots],'vertices':mesh.get_num_vertices(0),'triangles':mesh.get_num_triangles(0)}; p.write_text(json.dumps(d,indent=2))
(root/'Saved/currency-ready.txt').write_text('Candybar wrappers and revised dialogue imported.\n')
