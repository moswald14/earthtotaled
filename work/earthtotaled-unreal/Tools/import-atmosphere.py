"""Import only the atmosphere assets. Also called by the full content bootstrap."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assets=unreal.AssetToolsHelpers.get_asset_tools()
editor=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
base='/Game/EARTHtotaled'
names=['MusicForest','MusicCity','MusicNight','WeatherRain','WeatherWind','WeatherThunder']
tasks=[]
for source in [*(root/'ArtSource'/('S_'+n+'.wav') for n in names),root/'ArtSource/T_WeatherClouds.png']:
    task=unreal.AssetImportTask(); task.filename=str(source); task.destination_path=base+'/Art'; task.destination_name=source.stem
    task.automated=True; task.replace_existing=True; task.save=True; tasks.append(task)
assets.import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths: raise RuntimeError('Atmosphere import failed: '+task.filename)
for name in names:
    sound=editor.load_asset(base+'/Art/S_'+name)
    sound.set_editor_property('looping',name!='WeatherThunder')
    sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    editor.save_loaded_asset(sound)

lib=unreal.MaterialEditingLibrary
def material(name):
    path=base+'/Materials/M_'+name
    m=editor.load_asset(path) if editor.does_asset_exist(path) else assets.create_asset('M_'+name,base+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(m)
    m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property('two_sided',True)
    return m
def expr(m,kind,**props):
    e=lib.create_material_expression(m,kind)
    for key,value in props.items(): e.set_editor_property(key,value)
    return e
def link(a,b,input='',output=''):
    # UE 5.8 exposes the Clamp value pin as an unnamed first input.
    pins=list(lib.get_material_expression_input_names(b))
    if input=='Input' and input not in pins and pins: input=pins[0]
    if not lib.connect_material_expressions(a,output,b,input):
        raise RuntimeError('Cannot connect '+str(b.get_class().get_name())+' input '+repr(input)+'; available '+repr(pins))
def scalar(m,n,v): return expr(m,unreal.MaterialExpressionScalarParameter,parameter_name=n,default_value=v)
def constant(m,v): return expr(m,unreal.MaterialExpressionConstant,r=v)
def finish(m):
    lib.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    lib.recompile_material(m); editor.save_loaded_asset(m)

m=material('WeatherRain')
color=expr(m,unreal.MaterialExpressionConstant3Vector,constant=unreal.LinearColor(.32,.42,.52,1))
lib.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(constant(m,.32),'',unreal.MaterialProperty.MP_OPACITY)
finish(m)

m=material('WeatherClouds')
uv=expr(m,unreal.MaterialExpressionTextureCoordinate,u_tiling=4.,v_tiling=2.)
pan=expr(m,unreal.MaterialExpressionPanner,speed_x=.0012,speed_y=.0005); link(uv,pan,'Coordinate')
texture=expr(m,unreal.MaterialExpressionTextureSample,texture=editor.load_asset(base+'/Art/T_WeatherClouds'))
link(pan,texture,'UVs')
coverage=scalar(m,'Coverage',.19)
cutoff=expr(m,unreal.MaterialExpressionOneMinus); link(coverage,cutoff,'Input')
cloud=expr(m,unreal.MaterialExpressionSubtract); link(texture,cloud,'A','R'); link(cutoff,cloud,'B')
gain=expr(m,unreal.MaterialExpressionMultiply,const_b=3.); link(cloud,gain,'A')
clamp=expr(m,unreal.MaterialExpressionClamp); link(gain,clamp,'Input')
plain_uv=expr(m,unreal.MaterialExpressionTextureCoordinate)
v=expr(m,unreal.MaterialExpressionComponentMask,r=False,g=True,b=False,a=False); link(plain_uv,v,'Input')
hemisphere=expr(m,unreal.MaterialExpressionSubtract,const_a=.52); link(v,hemisphere,'B')
hgain=expr(m,unreal.MaterialExpressionMultiply,const_b=12.); link(hemisphere,hgain,'A')
hclamp=expr(m,unreal.MaterialExpressionClamp); link(hgain,hclamp,'Input')
opacity=expr(m,unreal.MaterialExpressionMultiply); link(clamp,opacity,'A'); link(hclamp,opacity,'B')
lib.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
tint=expr(m,unreal.MaterialExpressionVectorParameter,parameter_name='CloudTint',default_value=unreal.LinearColor(.22,.27,.31,1))
lib.connect_material_property(tint,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
finish(m)
(root/'Saved/atmosphere-ready.txt').write_text('Three stereo score loops, rain, wind, thunder and cloud/rain materials imported.\n')
