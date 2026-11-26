"""Import the five original highway meshes and keep their walkable interiors."""
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
art='/Game/EARTHtotaled/Art'
assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
names=('ParallaxSpire','ComputeHall','BellowsHive','HiveNursery','CrossingSignal')
library=unreal.MaterialEditingLibrary
for label,color,strength in [('CircuitLight',(.14,.69,1.0),2.4),('ResinLight',(1.0,.31,.048),2.1)]:
    path='/Game/EARTHtotaled/Materials'
    mat=assets.load_asset(path+'/M_'+label) if assets.does_asset_exist(path+'/M_'+label) else None
    if mat is None:
        mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+label,path,unreal.Material,unreal.MaterialFactoryNew())
    library.delete_all_material_expressions(mat)
    base=library.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
    base.set_editor_property('constant',unreal.LinearColor(*color,1))
    glow=library.create_material_expression(mat,unreal.MaterialExpressionMultiply)
    glow.set_editor_property('const_b',strength)
    library.connect_material_expressions(base,'',glow,'A')
    library.connect_material_property(base,'',unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    library.recompile_material(mat)
    assets.save_loaded_asset(mat)
tasks=[]
for name in names:
    task=unreal.AssetImportTask()
    task.filename=str(root/'ArtSource'/('SM_'+name+'.obj'))
    task.destination_path=art
    task.destination_name='SM_'+name
    task.automated=task.save=task.replace_existing=True
    opts=unreal.FbxImportUI()
    opts.automated_import_should_detect_type=False
    opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opts.import_materials=opts.import_textures=False
    data=opts.static_mesh_import_data
    data.combine_meshes=True
    data.auto_generate_collision=data.generate_lightmap_u_vs=data.convert_scene=data.build_nanite=False
    data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.factory=unreal.FbxFactory()
    task.options=opts
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name,task in zip(names,tasks):
    if not task.imported_object_paths:
        raise RuntimeError('Missing highway mesh '+name)
    mesh=assets.load_asset(art+'/SM_'+name)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        label=str(slot.material_slot_name).rsplit('_',1)[-1]
        mesh.set_material(i,assets.load_asset('/Game/EARTHtotaled/Materials/M_'+label))
    assets.save_loaded_asset(mesh)
(root/'Saved/warfront-ready.txt').write_text('Imported '+', '.join(names)+'\n')
