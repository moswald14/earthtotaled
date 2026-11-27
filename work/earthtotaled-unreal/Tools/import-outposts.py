from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
art = '/Game/EARTHtotaled/Art'
editor = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
assets = unreal.AssetToolsHelpers.get_asset_tools()
models = ['Relay','SentryStand','SentryHead','Turtle','TurtleBit','BroodSkimmer','BroodBulwark']
surfaces = ['Ground','Bark','Concrete','FaultShale','FaultIron','FaultGold']
reclamation = globals().get('reclamation', False)
ringline = globals().get('ringline', False)
quarters = globals().get('quarters', False)
woodland = globals().get('woodland', False)
if woodland:
    models=['WoodlandBear','BearForeleg','BearHindleg','CedarFall','AmberCaps','BentBracken','ConeBed','KitFold','BearSign','HoofReplay']
    surfaces=['BearFur','AmberCap']
if reclamation:
    models = ['BladePatch','RoadWeed','HollowWalker','HillCistern','MoltRunner','MoltSpitter','MoltBulwark','MoltSkimmer','SupertechWarden','SupertechScout']
    surfaces = ['Ground','FaultShale','FaultIron','FaultGold','Carapace','AlloyHull','Pallor','RotCloth']
    models=globals().get('reclamation_models',models)
    surfaces=globals().get('reclamation_surfaces',surfaces)
if ringline:
    models=['Pine','PineB','House','Tenement','RingSled','RingGuide','RingPier','RingStation']
    surfaces=['Ground','Bark','Concrete','Asphalt','Steel','Carapace','AlloyHull']
if quarters:
    models=sorted(p.stem[3:] for p in (root/'ArtSource').glob('SM_Quarter*.obj'))
    if len(models)!=16: raise RuntimeError('Expected all sixteen rail district landmarks')
    surfaces=[]
maps = [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS)]
files = ['SM_'+m+'.obj' for m in models]+['T_Relief'+m+'_'+s+'.png' for m in surfaces for s,_ in maps]
tasks = []
if woodland:
    files.append('S_BearHuff.wav')
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
        if quarters:
            data.build_nanite=True
        elif ringline:
            data.build_nanite = name in ['SM_Pine.obj','SM_PineB.obj','SM_House.obj','SM_Tenement.obj','SM_RingStation.obj','SM_RingSled.obj']
        data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.factory=unreal.FbxFactory()
        task.options=opts
    tasks.append(task)
assets.import_asset_tasks(tasks)
if any(not t.imported_object_paths for t in tasks):
    raise RuntimeError('Incomplete outpost art import')
for name in models:
    mesh=editor.load_asset(art+'/SM_'+name)
    if ringline and name in ['Pine','PineB']:
        subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        settings=subsystem.get_nanite_settings(mesh)
        settings.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA)
        subsystem.set_nanite_settings(mesh,settings,True)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    editor.save_loaded_asset(mesh)
lib=unreal.MaterialEditingLibrary
for name in surfaces:
    folder='/Game/EARTHtotaled/Materials'
    path=folder+'/M_'+name
    material=editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(material)
    if name=='Asphalt':
        # One four-metre tile on both the procedurally laid streets and older stretched road slabs.
        world=lib.create_material_expression(material,unreal.MaterialExpressionWorldPosition)
        plane=lib.create_material_expression(material,unreal.MaterialExpressionComponentMask)
        for channel in ('r','g','b','a'):
            plane.set_editor_property(channel,channel in ('r','g'))
        road_uv=lib.create_material_expression(material,unreal.MaterialExpressionMultiply)
        road_uv.set_editor_property('const_b',1/400.)
        if not (lib.connect_material_expressions(world,'',plane,'') and lib.connect_material_expressions(plane,'',road_uv,'A')):
            raise RuntimeError('Road surface could not project the world position onto the street')
    for suffix,prop in maps:
        tex=editor.load_asset(art+'/T_Relief'+name+'_'+suffix)
        tex.set_editor_property('srgb',suffix=='BaseColor')
        if suffix=='Normal': tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('filter',unreal.TextureFilter.TF_DEFAULT)
        editor.save_loaded_asset(tex)
        sample=lib.create_material_expression(material,unreal.MaterialExpressionTextureSample)
        sample.texture=tex
        if name=='Asphalt':
            if not lib.connect_material_expressions(road_uv,'',sample,''):
                raise RuntimeError('Road surface coordinates did not connect')
        sample.sampler_type={'BaseColor':unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,'Normal':unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,'Roughness':unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}[suffix]
        lib.connect_material_property(sample,'R' if suffix=='Roughness' else 'RGB',prop)
        if suffix=='BaseColor' and name in ['FaultGold','FaultIron']:
            glow=lib.create_material_expression(material,unreal.MaterialExpressionMultiply)
            glow.set_editor_property('const_b',.09)
            lib.connect_material_expressions(sample,'RGB',glow,'A')
            lib.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name in ('AlloyHull','FaultGold','Steel'):
        plate=lib.create_material_expression(material,unreal.MaterialExpressionConstant)
        plate.set_editor_property('r',.43 if name=='FaultGold' else .74)
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
if reclamation or ringline or woodland:
    for name in models:
        mesh=editor.load_asset(art+'/SM_'+name)
        for index,slot in enumerate(mesh.get_editor_property('static_materials')):
            material=editor.load_asset(folder+'/M_'+str(slot.material_slot_name).rsplit('_',1)[-1])
            if material is None: raise RuntimeError(str(slot.material_slot_name))
            mesh.set_material(index,material)
        editor.save_loaded_asset(mesh)
(root/'Saved'/('woodland-ready.txt' if woodland else 'interchange-ready.txt' if quarters else 'ringline-ready.txt' if ringline else 'reclamation-ready.txt' if reclamation else 'outposts-ready.txt')).write_text(f'{len(models)} meshes and {len(surfaces)} PBR surfaces imported.\n')

