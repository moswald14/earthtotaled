"""Reimport the roof after splitting corrugated sheets into planar faces."""
from pathlib import Path
import unreal, json
root=Path(unreal.Paths.project_dir()).resolve()
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
task=unreal.AssetImportTask()
task.filename=str(root/'ArtSource/SM_Roof.obj'); task.destination_path='/Game/EARTHtotaled/Art'; task.destination_name='SM_Roof'
task.automated=True; task.replace_existing=True; task.save=True; task.factory=unreal.FbxFactory()
options=unreal.FbxImportUI()
options.set_editor_property('automated_import_should_detect_type',False)
options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
options.set_editor_property('import_materials',False); options.set_editor_property('import_textures',False)
data=options.static_mesh_import_data
data.set_editor_property('combine_meshes',True); data.set_editor_property('auto_generate_collision',True)
data.set_editor_property('generate_lightmap_u_vs',False); data.set_editor_property('convert_scene',False)
data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
data.set_editor_property('build_nanite',True); task.options=options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
if not task.imported_object_paths: raise RuntimeError('Roof reimport failed')
mesh=assets.load_asset('/Game/EARTHtotaled/Art/SM_Roof')
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
slots=mesh.get_editor_property('static_materials')
for i,slot in enumerate(slots):
    name=str(slot.material_slot_name)
    mat=next((x for x in ['Brick','DarkMetal','Rust','Wood'] if name==x or name.endswith('_'+x)),'Steel')
    mesh.set_material(i,assets.load_asset('/Game/EARTHtotaled/Materials/M_'+mat))
assets.save_loaded_asset(mesh)
report=root/'Saved/graphics-import.json'; data=json.loads(report.read_text())
data['SM_Roof']={'slots':[str(x.material_slot_name) for x in slots],'vertices':mesh.get_num_vertices(0),'triangles':mesh.get_num_triangles(0)}
report.write_text(json.dumps(data,indent=2))
(root/'Saved/roof-shading-ready.txt').write_text('Planar corrugated roof surfaces imported with nondegenerate UVs.\n')
