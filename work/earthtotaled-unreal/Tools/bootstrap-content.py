"""Run inside Unreal Editor after compiling: imports the original art and creates the map."""
from pathlib import Path
import re, json
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
art=root/'ArtSource'
assets=unreal.AssetToolsHelpers.get_asset_tools()
editor_assets=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
base='/Game/EARTHtotaled'
tasks=[]
for source in sorted(art.glob('*.obj')):
    task=unreal.AssetImportTask()
    task.filename=str(source); task.destination_path=base+'/Art'; task.destination_name=source.stem
    task.automated=True; task.replace_existing=True; task.save=True
    task.factory=unreal.FbxFactory()
    options=unreal.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type',False)
    options.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property('import_as_skeletal',False)
    options.set_editor_property('import_materials',False)
    options.set_editor_property('import_textures',False)
    data=options.static_mesh_import_data
    data.set_editor_property('combine_meshes',True)
    data.set_editor_property('auto_generate_collision',True)
    data.set_editor_property('generate_lightmap_u_vs',False)
    data.set_editor_property('convert_scene',False)
    data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    data.set_editor_property('build_nanite',not source.stem.startswith('SM_Weapon') and source.stem not in ['SM_Block','SM_Trunk','SM_Grass','SM_Trap','SM_Cache','SM_Core','SM_Wrappers'])
    task.options=options; tasks.append(task)
for source in sorted([*art.glob('*.png'),*art.glob('*.wav')]):
    task=unreal.AssetImportTask(); task.filename=str(source); task.destination_path=base+'/Art'; task.destination_name=source.stem
    task.automated=True; task.replace_existing=True; task.save=True; tasks.append(task)
assets.import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths: raise RuntimeError('Import failed: '+task.filename)

for source in sorted(art.glob('*.obj')):
    mesh=editor_assets.load_asset(base+'/Art/'+source.stem)
    if not isinstance(mesh,unreal.StaticMesh): raise RuntimeError('Missing static mesh: '+source.stem)
    if source.stem in ['SM_Pine','SM_PineB','SM_Birch','SM_Fern','SM_Shrub']:
        nanite=mesh_editor.get_nanite_settings(mesh)
        nanite.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA)
        mesh_editor.set_nanite_settings(mesh,nanite,True)
    body=mesh.get_editor_property('body_setup')
    if body:
        # A convex hull would seal door openings. Use the actual triangles.
        body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    else: raise RuntimeError('Mesh collision setup missing: '+source.stem)
    editor_assets.save_loaded_asset(mesh)

for name in ['ForestDay','ForestNight','CityWind']:
    sound=editor_assets.load_asset(base+'/Art/S_'+name)
    if sound:
        sound.set_editor_property('looping',True)
        editor_assets.save_loaded_asset(sound)

# Keep a full rebuild equivalent to the selective weather/music import.
import runpy
runpy.run_path(str(root/'Tools/import-atmosphere.py'))
runpy.run_path(str(root/'Tools/import-geology.py'))
runpy.run_path(str(root/'Tools/import-frontier.py'))

materials=['Ground','Concrete','Steel','Rust','Wood','Bark','Canvas','Hide','Grass','Gold','Signal','Greenline','Asphalt','Pine','DarkMetal','Rubber','Glass','Skin','Leaf','Moss','BirchBark','Brick','Plaster']
for name in materials:
    path=base+'/Materials/M_'+name
    material=editor_assets.load_asset(path) if editor_assets.does_asset_exist(path) else None
    if not material: material=assets.create_asset('M_'+name,base+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    else: unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property('two_sided',name in ['Leaf','Pine','Moss','Grass','Rust','Canvas','Glass'])
    if name in ['Leaf','Pine','Moss','Grass']:
        material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    for suffix,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS),('Normal',unreal.MaterialProperty.MP_NORMAL)]:
        texture=editor_assets.load_asset(base+'/Art/T_'+name+'_'+suffix)
        if not texture: raise RuntimeError('Texture missing: '+name+' / '+suffix)
        if suffix=='Normal': texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property('srgb',suffix=='BaseColor')
        editor_assets.save_loaded_asset(texture)
        expr=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionTextureSample)
        expr.set_editor_property('texture',texture)
        if suffix=='Normal': expr.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif suffix=='Roughness': expr.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(expr,'R' if suffix=='Roughness' else 'RGB',prop)
        if suffix=='BaseColor' and name in ['Signal','Greenline']:
            unreal.MaterialEditingLibrary.connect_material_property(expr,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if suffix=='BaseColor' and name in ['Leaf','Pine','Moss','Grass']:
            unreal.MaterialEditingLibrary.connect_material_property(expr,'RGB',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    metallic=unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant)
    metallic.set_editor_property('r',.85 if name in ['Steel','Gold','DarkMetal'] else .35 if name=='Rust' else 0)
    unreal.MaterialEditingLibrary.connect_material_property(metallic,'',unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_NANITE)
    unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(material)
    editor_assets.save_loaded_asset(material)

# Named OBJ slots retain the different surfaces of a weapon, character or building.
def slot_material(slot,base_name):
    name=str(slot.material_slot_name)
    for material in materials:
        if name==material or name.endswith('_'+material): return material
    return base_name

import_report={}
for source in sorted(art.glob('*.obj')):
    mesh=editor_assets.load_asset(base+'/Art/'+source.stem)
    slots=mesh.get_editor_property('static_materials')
    for index,slot in enumerate(slots): mesh.set_material(index,editor_assets.load_asset(base+'/Materials/M_'+slot_material(slot,'Steel')))
    editor_assets.save_loaded_asset(mesh)
    import_report[source.stem]={'slots':[str(x.material_slot_name) for x in slots],'vertices':mesh.get_num_vertices(0),'triangles':mesh.get_num_triangles(0)}
(root/'Saved/graphics-import.json').write_text(json.dumps(import_report,indent=2))

level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path=base+'/Maps/ExclusionZone'
if not editor_assets.does_asset_exist(map_path):
    if not level.new_level(map_path): raise RuntimeError('Could not create map')
else: level.load_level(map_path)
# Replace only our generated district. The starting architecture is cooked into the map,
# rather than being randomly assembled while the player is already looking at it.
actor_editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actor_editor.get_all_level_actors():
    if 'ET_BakedStart' in [str(t) for t in actor.tags]: actor_editor.destroy_actor(actor)
layout=json.loads((root/'Content/Data/starting-area.json').read_text())
for i,entry in enumerate(layout['actors']):
    actor=actor_editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*entry['position']),unreal.Rotator(0,entry['yaw'],0))
    actor.tags=['ET_BakedStart']; actor.set_actor_label('Shelter09_'+str(i)+'_'+entry['mesh'])
    component=actor.static_mesh_component
    component.set_static_mesh(editor_assets.load_asset(base+'/Art/'+entry['mesh']))
    for index,slot in enumerate(component.static_mesh.get_editor_property('static_materials')):
        component.set_material(index,editor_assets.load_asset(base+'/Materials/M_'+slot_material(slot,entry['material'])))
    component.set_collision_profile_name('BlockAll' if entry.get('collision',True) else 'NoCollision')
    cull=entry.get('cull_distance',50000)
    component.set_cull_distance(cull)
    if entry.get('decor'):
        component.component_tags=['ET_DetailNear' if cull<=14000 else 'ET_DetailMid' if cull<=24000 else 'ET_DetailFar']
    component.set_editor_property('cast_shadow',entry['mesh'] not in ['SM_Block','SM_Fern','SM_Shrub','SM_Grass'])
    actor.set_actor_scale3d(unreal.Vector(*entry['scale']))
if not level.save_current_level(): raise RuntimeError('Could not save map')
config=root/'Config/DefaultEngine.ini'
text=config.read_text(encoding='utf-8')
for key in ['GameDefaultMap','EditorStartupMap']:
    text=re.sub(r'(?m)^'+key+r'=.*$',key+'='+map_path,text)
config.write_text(text,encoding='utf-8')
runpy.run_path(str(root/'Tools/import-outposts.py'))
runpy.run_path(str(root/'Tools/import-warfront.py'))
editor_assets.save_directory(base,only_if_is_dirty=False,recursive=True)
(root/'Saved').mkdir(exist_ok=True)
(root/'Saved/content-ready.txt').write_text('Original art imported and map saved by Unreal Editor.\n')
unreal.log('EARTHTOTALED_CONTENT_READY')
