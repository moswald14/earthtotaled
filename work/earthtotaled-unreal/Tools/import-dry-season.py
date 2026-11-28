from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
models=['FieldKit','Cinderbean','Saltroot','Mendleaf','FieldBed','FieldTank','FieldPump','FieldClinic','RootTank','RootPump','RootClinic','StrandedDock','DryCulvert']
tracks=['Saltline','PumpHouse','NightFurrow']
shared=(root/'Tools'/'import-outposts.py').read_text()
exec(compile(shared,str(root/'Tools'/'import-outposts.py'),'exec'),{'field_models':models})
art='/Game/EARTHtotaled/Art'; folder='/Game/EARTHtotaled/Materials'
tools=unreal.AssetToolsHelpers.get_asset_tools(); editor=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
lib=unreal.MaterialEditingLibrary
tasks=[]
for name in ['T_FieldKit.png','T_CrackedSilt.png']+['S_MusicJazz'+t+'.wav' for t in tracks]:
    task=unreal.AssetImportTask(); task.filename=str(root/'ArtSource'/name);task.destination_path=art
    task.destination_name=Path(name).stem;task.automated=task.save=task.replace_existing=True;tasks.append(task)
tools.import_asset_tasks(tasks)
if any(not t.imported_object_paths for t in tasks): raise RuntimeError('Missing Dry Season artwork or music')
icon=editor.load_asset(art+'/T_FieldKit');icon.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);editor.save_loaded_asset(icon)
for t in tracks:
    wave=editor.load_asset(art+'/S_MusicJazz'+t);wave.set_editor_property('looping',True)
    wave.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT);editor.save_loaded_asset(wave)
for name,color in {'KitTeal':(.025,.115,.12),'Bandage':(.83,.76,.55),'Stitch':(.92,.42,.055),'CrackedSilt':None,'SaltRing':(.64,.59,.45),'Irrigation':(.055,.32,.31),'FlowGlint':(.38,.73,.64)}.items():
    path=folder+'/M_'+name
    m=editor.load_asset(path) if editor.does_asset_exist(path) else tools.create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(m)
    if color is None:
        e=lib.create_material_expression(m,unreal.MaterialExpressionTextureSample); e.texture=editor.load_asset(art+'/T_CrackedSilt')
        lib.connect_material_property(e,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        e=lib.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);e.constant=unreal.LinearColor(*color,1)
        lib.connect_material_property(e,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(m,unreal.MaterialExpressionConstant);rough.r=.17 if name in ('Irrigation','FlowGlint') else .83
    lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    lib.recompile_material(m); editor.save_loaded_asset(m)

# Keep the original ground normal, roughness and texture; paint streambeds through terrain vertex colors.
path=folder+'/M_DryGround'
ground=editor.load_asset(path) if editor.does_asset_exist(path) else editor.duplicate_asset(folder+'/M_Ground',path)
if not ground: raise RuntimeError('Could not build the terrain material')
base=unreal.MaterialProperty.MP_BASE_COLOR
source=lib.get_material_property_input_node(ground,base)
if source.get_class().get_name()!='MaterialExpressionLinearInterpolate':
    vertex=lib.create_material_expression(ground,unreal.MaterialExpressionVertexColor)
    clay=lib.create_material_expression(ground,unreal.MaterialExpressionTextureSample)
    clay.texture=editor.load_asset(art+'/T_CrackedSilt')
    salt=lib.create_material_expression(ground,unreal.MaterialExpressionConstant3Vector)
    salt.constant=unreal.LinearColor(.65,.58,.42,1)
    for tex,out,alpha in [(clay,'RGB','R'),(salt,'','G')]:
        mix=lib.create_material_expression(ground,unreal.MaterialExpressionLinearInterpolate)
        lib.connect_material_expressions(source,'',mix,'A')
        lib.connect_material_expressions(tex,out,mix,'B')
        lib.connect_material_expressions(vertex,alpha,mix,'Alpha')
        source=mix
    lib.connect_material_property(source,'',base)
    lib.recompile_material(ground);editor.save_loaded_asset(ground)
