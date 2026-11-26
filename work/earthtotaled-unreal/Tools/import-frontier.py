"""Cookable Fieldworks creatures, construction meshes, holograms and audio."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
editor = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
assets = unreal.AssetToolsHelpers.get_asset_tools()
art = '/Game/EARTHtotaled/Art'
models = ['ShardDrone', 'BroodRunner', 'BroodSpitter', 'FieldWall', 'FieldFloor', 'FieldRamp', 'FieldRoof']
sounds = ['JetThrust', 'ShardChirp', 'BroodCall', 'FieldLatch', 'Voice_salt', 'Voice_keeper', 'Voice_supplydone']
tasks = []
for filename in ['SM_'+s+'.obj' for s in models] + ['S_'+s+'.wav' for s in sounds]:
    task = unreal.AssetImportTask()
    task.filename = str(root/'ArtSource'/filename)
    task.destination_path = art
    task.destination_name = Path(filename).stem
    task.automated = task.save = task.replace_existing = True
    if filename.endswith('.obj'):
        opts = unreal.FbxImportUI()
        opts.automated_import_should_detect_type = False
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        opts.import_materials = opts.import_textures = False
        data = opts.static_mesh_import_data
        data.combine_meshes = True
        data.auto_generate_collision = True
        data.generate_lightmap_u_vs = data.convert_scene = data.build_nanite = False
        data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.factory = unreal.FbxFactory()
        task.options = opts
    tasks.append(task)
assets.import_asset_tasks(tasks)
if any(not t.imported_object_paths for t in tasks):
    raise RuntimeError('Fieldworks import failed')
for name in models:
    mesh = editor.load_asset(art+'/SM_'+name)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    editor.save_loaded_asset(mesh)

lib = unreal.MaterialEditingLibrary
folder = '/Game/EARTHtotaled/Materials'
for name, tint in [('BuildReady', (.04, .88, .62)), ('BuildBlocked', (1, .15, .06))]:
    path = folder+'/M_'+name
    m = editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name, folder, unreal.Material, unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(m)
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    color = lib.create_material_expression(m, unreal.MaterialExpressionConstant3Vector)
    color.constant = unreal.LinearColor(*tint, 1)
    lib.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = lib.create_material_expression(m, unreal.MaterialExpressionConstant)
    opacity.r = .32
    lib.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    lib.recompile_material(m)
    editor.save_loaded_asset(m)
(root/'Saved/frontier-ready.txt').write_text('Original Fieldworks meshes, collision, audio and preview materials imported.\n')
