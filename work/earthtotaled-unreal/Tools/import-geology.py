"""Selective Unreal import for the Faultline update."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
base = '/Game/EARTHtotaled'
assets = unreal.AssetToolsHelpers.get_asset_tools()
editor = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
lib = unreal.MaterialEditingLibrary
minerals = ['FaultShale', 'FaultIron', 'FaultGold']
tasks = []
for file in ['SM_SurveyBorer.obj', 'S_BorerPulse.wav', 'S_OreChime.wav', *['T_'+m+'.png' for m in minerals]]:
    task = unreal.AssetImportTask()
    task.filename = str(root/'ArtSource'/file)
    task.destination_path = base+'/Art'
    task.destination_name = Path(file).stem
    task.automated = task.save = task.replace_existing = True
    if file.endswith('.obj'):
        opts = unreal.FbxImportUI()
        opts.automated_import_should_detect_type = False
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        opts.import_materials = opts.import_textures = False
        data = opts.static_mesh_import_data
        data.combine_meshes = True
        data.auto_generate_collision = data.generate_lightmap_u_vs = data.convert_scene = data.build_nanite = False
        data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.factory = unreal.FbxFactory()
        task.options = opts
    tasks.append(task)
assets.import_asset_tasks(tasks)
if any(not t.imported_object_paths for t in tasks):
    raise RuntimeError('Faultline source import failed')

def material(name):
    folder = base+'/Materials'
    path = folder+'/M_'+name
    m = editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name, folder, unreal.Material, unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(m)
    return m

for name in minerals:
    tex = editor.load_asset(base+'/Art/T_'+name)
    tex.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    editor.save_loaded_asset(tex)
    m = material(name)
    sample = lib.create_material_expression(m, unreal.MaterialExpressionTextureSample)
    sample.texture = tex
    lib.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = lib.create_material_expression(m, unreal.MaterialExpressionConstant)
    roughness.r = .92 if name=='FaultShale' else .6
    lib.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    glow = lib.create_material_expression(m, unreal.MaterialExpressionMultiply)
    glow.set_editor_property('const_b', .025 if name=='FaultShale' else .19)
    lib.connect_material_expressions(sample, 'RGB', glow, 'A')
    lib.connect_material_property(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(m)
    editor.save_loaded_asset(m)

# Sample after tonemapping so the native-resolution HUD stays legible.
m = material('FieldDisplay')
m.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
m.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
scene = lib.create_material_expression(m, unreal.MaterialExpressionSceneTexture)
scene.set_editor_property('scene_texture_id', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
shader = lib.create_material_expression(m, unreal.MaterialExpressionCustom)
entry = unreal.CustomInput()
entry.set_editor_property('input_name', 'Scene')
shader.set_editor_property('inputs', [entry])
shader.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
shader.set_editor_property('code', """
float2 view = GetViewportUV(Parameters);
float2 grid = float2(round(270.0 * View.ViewSizeAndInvSize.x / View.ViewSizeAndInvSize.y), 270.0);
float2 pixel = floor(view * grid);
float2 uv = ClampSceneTextureUV(ViewportUVToSceneTextureUV((pixel + 0.5) / grid, 14), 14);
float3 color = saturate(SceneTextureLookup(uv, 14, false).rgb);
int2 p = int2(pixel) & 3;
int d = ((p.x & 1) ^ (p.y & 1))*8 + (p.y & 1)*4 + (((p.x >> 1) ^ (p.y >> 1)) & 1)*2 + (p.y >> 1);
return saturate(floor(color * 31.0 + (d + 0.5)/16.0)/31.0);
""")
if not lib.connect_material_expressions(scene, 'Color', shader, 'Scene'):
    raise RuntimeError('Postprocess scene input disconnected')
lib.connect_material_property(shader, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(m)
editor.save_loaded_asset(m)
(root/'Saved/geology-ready.txt').write_text('Faultline assets imported.\n')
