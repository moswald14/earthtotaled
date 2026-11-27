from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
art = '/Game/EARTHtotaled/Art'
editor = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
assets = unreal.AssetToolsHelpers.get_asset_tools()
models = ['Relay','SentryStand','SentryHead','Turtle','TurtleBit','BroodSkimmer','BroodBulwark']
surfaces = ['Ground','Bark','Concrete','FaultShale','FaultIron','FaultGold']
reclamation = globals().get('reclamation', False)
if reclamation:
    models = ['BladePatch','RoadWeed','HollowWalker','HillCistern','MoltRunner','MoltSpitter','MoltBulwark','MoltSkimmer','SupertechWarden','SupertechScout']
    surfaces = ['Ground','FaultShale','FaultIron','FaultGold','Carapace','AlloyHull','Pallor','RotCloth']
    models=globals().get('reclamation_models',models)
    surfaces=globals().get('reclamation_surfaces',surfaces)
maps = [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS)]
files = ['SM_'+m+'.obj' for m in models]+['T_Relief'+m+'_'+s+'.png' for m in surfaces for s,_ in maps]
tasks = []
for name in files:
    task=unreal.AssetImportTask()
    task.filename=str(root/'ArtSource'/name)
    task.destination_path=art
    task.destination_name=Path(name).stem
    task.automated=task.save=task.replace_existing=True
    if name.endswith('.obj'):
        opts=unreal.FbxImportUI()
        opts.automated_import_should_detect_type=False
        opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
        opts.import_materials=opts.import_textures=False
        data=opts.static_mesh_import_data
        data.combine_meshes=data.auto_generate_collision=True
        data.generate_lightmap_u_vs=data.convert_scene=data.build_nanite=False
        data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.factory=unreal.FbxFactory()
        task.options=opts
    tasks.append(task)
assets.import_asset_tasks(tasks)
if any(not t.imported_object_paths for t in tasks):
    raise RuntimeError('Incomplete outpost art import')
for name in models:
    mesh=editor.load_asset(art+'/SM_'+name)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    editor.save_loaded_asset(mesh)
lib=unreal.MaterialEditingLibrary
for name in surfaces:
    folder='/Game/EARTHtotaled/Materials'
    path=folder+'/M_'+name
    material=editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(material)
    for suffix,prop in maps:
        tex=editor.load_asset(art+'/T_Relief'+name+'_'+suffix)
        tex.set_editor_property('srgb',suffix=='BaseColor')
        if suffix=='Normal': tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('filter',unreal.TextureFilter.TF_DEFAULT)
        editor.save_loaded_asset(tex)
        sample=lib.create_material_expression(material,unreal.MaterialExpressionTextureSample)
        sample.texture=tex
        sample.sampler_type={'BaseColor':unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,'Normal':unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,'Roughness':unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}[suffix]
        lib.connect_material_property(sample,'R' if suffix=='Roughness' else 'RGB',prop)
        if suffix=='BaseColor' and name in ['FaultGold','FaultIron']:
            glow=lib.create_material_expression(material,unreal.MaterialExpressionMultiply)
            glow.set_editor_property('const_b',.09)
            lib.connect_material_expressions(sample,'RGB',glow,'A')
            lib.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name in ('AlloyHull','FaultGold'):
        plate=lib.create_material_expression(material,unreal.MaterialExpressionConstant)
        plate.set_editor_property('r',.74 if name=='AlloyHull' else .43)
        lib.connect_material_property(plate,'',unreal.MaterialProperty.MP_METALLIC)
    lib.recompile_material(material)
    editor.save_loaded_asset(material)
if reclamation:
    for name,color in [('GreenBlade',(.085,.163,.044)),('DryBlade',(.24,.195,.092))]:
        path=folder+'/M_'+name
        m=editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
        m.set_editor_property('two_sided',True)
        lib.delete_all_material_expressions(m)
        node=lib.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property('constant',unreal.LinearColor(*color,1))
        lib.connect_material_property(node,'',unreal.MaterialProperty.MP_BASE_COLOR)
        lib.recompile_material(m)
        editor.save_loaded_asset(m)
    for name in models:
        mesh=editor.load_asset(art+'/SM_'+name)
        for index,slot in enumerate(mesh.get_editor_property('static_materials')):
            material=editor.load_asset(folder+'/M_'+str(slot.material_slot_name).rsplit('_',1)[-1])
            if material is None: raise RuntimeError(str(slot.material_slot_name))
            mesh.set_material(index,material)
        editor.save_loaded_asset(mesh)
(root/'Saved'/('reclamation-ready.txt' if reclamation else 'outposts-ready.txt')).write_text(f'{len(models)} meshes and {len(surfaces)} PBR surfaces imported.\n')

