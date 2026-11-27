"""Engine-independent content checks. Does not claim an Unreal build or playtest."""
from pathlib import Path
import json, math, struct, zlib, wave
ROOT=Path(__file__).resolve().parents[1]
checks=0
def check(ok,message):
    global checks
    if not ok: raise AssertionError(message)
    checks+=1
design=json.loads((ROOT/'Content/Data/design.json').read_text(encoding='utf-8'))
expansion=json.loads((ROOT/'Content/Data/expansion.json').read_text())
weapons=design['weapons']+expansion['weapons']
check(len(design['weapons'])==6 and len(weapons)==10,'Original six indices retained, ten total weapons')
supplies={s['id'] for s in expansion['items']}
check(len(supplies)==11 and {'goldore','ironore'}<=supplies,'Nine scavenged supplies and two mineable ores')
for r in expansion['recipes']:
    check(all(k in supplies and n>0 for k,n in r['ingredients'].items()),'Recipe has obtainable ingredients: '+r['id'])
for name,entries in expansion['lootTables'].items():
    check(len(entries)>=2 and all(e['id'] in supplies|{'water','buildkit','medkit'} and 0<e['min']<=e['max'] for e in entries),'Valid loot table: '+name)
check(supplies-{'goldore','ironore'}<={e['id'] for entries in expansion['lootTables'].values() for e in entries},'Non-mineral supplies can be scavenged')
check(len(design['skills'])==18,'18 node web')
check(len(design['quests'])==5,'Five community quests')
check(len(design['story'])==6,'Recordings preserved')
ids={p['id'] for p in design['skills']}
check(len(ids)==18,'Unique perk IDs')
known={'survivor'}
while True:
    nxt=known|{p['id'] for p in design['skills'] if any(n in known for n in p['requires'])}
    if nxt==known: break
    known=nxt
check(known==ids,'Entire perk graph is reachable')
check(next(q for q in design['quests'] if q['id']=='circuit')['water']==0,'Friendly robot quest awards no water')
for p in design['skills']: check(all(n in ids for n in p['requires']),'Perk link exists: '+p['id'])
def mesh(path):
    v=[]; f=[]
    for line in path.read_text().splitlines():
        if line.startswith('v '):
            x,y,z=(float(a) for a in line.split()[1:]); v.append((x,-y,z)) # FBX OBJ import reflects Y.
        elif line.startswith('f '): f.append(tuple(int(a.split('/')[0])-1 for a in line.split()[1:]))
    return v,f
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def ray(v,f,start,end):
    d=sub(end,start)
    for ia,ib,ic in f:
        a,b,c=v[ia],v[ib],v[ic]; e1,e2=sub(b,a),sub(c,a); p=cross(d,e2); det=dot(e1,p)
        if abs(det)<1e-9: continue
        tvec=sub(start,a); u=dot(tvec,p)/det
        if u<0 or u>1: continue
        q=cross(tvec,e1); w=dot(d,q)/det; t=dot(e2,q)/det
        if w>=0 and u+w<=1 and 0<=t<=1: return True
    return False
art=ROOT/'ArtSource'
for p in art.glob('*.obj'):
    v,f=mesh(p)
    check(bool(v) and bool(f),'Mesh has geometry: '+p.name)
    check(all(math.isfinite(a) for point in v for a in point),'Finite coordinates: '+p.name)
    check(all(0<=i<len(v) for face in f for i in face),'Valid indices: '+p.name)
    check(all(dot(cross(sub(v[b],v[a]),sub(v[c],v[a])),cross(sub(v[b],v[a]),sub(v[c],v[a])))>1e-10 for a,b,c in f),'Nondegenerate triangles: '+p.name)
    normals=[tuple(float(x) for x in line.split()[1:]) for line in p.read_text().splitlines() if line.startswith('vn ')]
    check(len(normals)==len(v) and all(abs(dot(n,n)-1)<.0001 for n in normals),'Valid unit shading normals: '+p.name)
v,f=mesh(art/'SM_House.obj')
for x in [-32,0,32]:
    for z in [25,92,180,220]: check(not ray(v,f,(x,-700,z),(x,-300,z)),'Doorway has a clear human-sized opening')
check(ray(v,f,(120,-700,100),(120,-300,100)),'Wall beside doorway blocks travel')
check(not ray(v,f,(-350,-700,180),(-350,-300,180)),'Window opening is clear')
check(ray(v,f,(-350,-700,80),(-350,-300,80)),'Wall below window blocks travel')
v,f=mesh(art/'SM_Roof.obj')
check(not ray(v,f,(-400,0,448),(-400,0,630)),'Roof renovation leaves the climbing landing clear')
check(not ray(v,f,(-400,0,550),(0,200,440)),'Rooftop relay remains visible and reachable')
uv=[]; uv_ok=True
for line in (art/'SM_Roof.obj').read_text().splitlines():
    if line.startswith('vt '): uv.append(tuple(float(a) for a in line.split()[1:]))
    elif line.startswith('f '):
        a,b,c=[uv[int(x.split('/')[1])-1] for x in line.split()[1:]]
        uv_ok &= abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))>1e-12
check(uv_ok,'Roof panels have usable texture tangents on every triangle')
v,f=mesh(art/'SM_Tenement.obj')
for floor in range(3):
    check(not ray(v,f,(0,-800,92+floor*380),(0,-300,92+floor*380)),'Every apartment storey has a doorway')
for p in art.glob('*.png'):
    data=p.read_bytes(); check(data[:8]==b'\x89PNG\r\n\x1a\n','Valid texture header: '+p.name)
    pos=8
    while pos<len(data):
        n=struct.unpack('>I',data[pos:pos+4])[0]; kind=data[pos+4:pos+8]; content=data[pos+8:pos+8+n]; crc=struct.unpack('>I',data[pos+8+n:pos+12+n])[0]
        check(zlib.crc32(kind+content)&0xffffffff==crc,'Texture CRC: '+p.name); pos+=n+12
check(len({(art/f'SM_Weapon{i}.obj').read_bytes() for i in range(10)})==10,'Ten distinct weapon meshes')
details=json.loads((art/'mesh-detail.json').read_text())
for name in ('Ground','Concrete','Bark','Asphalt','Steel','Carapace','AlloyHull'):
    for layer in ('BaseColor','Normal','Roughness'):
        texture=(art/('T_Relief'+name+'_'+layer+'.png')).read_bytes()
        width,height=struct.unpack('>II',texture[16:24])
        check((width,height)==(4096,4096),'Ringline 4K '+name+' '+layer)
        chunks=[]
        pos=8
        while pos<len(texture):
            count,kind=struct.unpack_from('>I4s',texture,pos)
            if kind==b'IDAT': chunks.append(texture[pos+8:pos+8+count])
            pos+=count+12
        raw=zlib.decompress(b''.join(chunks))
        stride=width*3+1
        check(texture[24:26]==bytes((8,2)) and len(raw)==stride*height and
              all(raw[y*stride]==0 for y in range(height)),'Authored RGB texels decode: '+name+' '+layer)
        samples=[raw[y*stride+1+x*3:y*stride+4+x*3] for y in range(17,height,79) for x in range(13,width,73)]
        check(len(set(samples))>16 and all(max(pixel[c] for pixel in samples)>20 for c in range(3)),
              'Color and surface relief vary across the tile: '+name+' '+layer)
        if layer=='Normal':
            check(all(pixel[2]>=128 for pixel in samples),'Surface normals face outward: '+name)
for name in ('RingGuide','RingSled','RingPier','RingStation'):
    check(details['SM_'+name]['triangles']>100,'Authored railway geometry '+name)
for name in ['SM_Pine','SM_PineB','SM_Birch','SM_Fern','SM_Shrub','SM_FallenLog','SM_Stump','SM_Rubble','SM_WallRuin','SM_Rebar']:
    check(name in details and details[name]['triangles']>100,'New environmental mesh has geometry: '+name)
for name in ['SM_Pine','SM_PineB']:
    verts=[]; foliage=[]; mat=''; longest=0
    for line in (art/(name+'.obj')).read_text().splitlines():
        if line.startswith('v '): verts.append(tuple(float(a) for a in line.split()[1:]))
        elif line.startswith('usemtl '): mat=line.split()[1]
        elif line.startswith('f ') and mat in ['Leaf','Pine']:
            tri=[verts[int(x.split('/')[0])-1] for x in line.split()[1:]]
            foliage.append(tri); longest=max(longest,max(math.dist(a,b) for a in tri for b in tri))
    check(len(foliage)>1000 and longest<60,'Canopy uses individual needle surfaces, not metre-wide solid cones: '+name)
for i in range(10): check(details[f'SM_Weapon{i}']['triangles']>=4000 and len(details[f'SM_Weapon{i}']['materials'])>=3,'Detailed weapon with separate surface materials')
for name in ['Concrete','Steel','Ground','Wood']:
    data=(art/('T_'+name+'_BaseColor.png')).read_bytes()
    check(struct.unpack('>II',data[16:24])==(2048,2048),'2048px main surface texture: '+name)
dialogue=json.loads((ROOT/'Content/Data/dialogue.json').read_text(encoding='utf-8-sig'))
check(len(dialogue['lines'])==20,'Twenty spoken character and archive lines')
check(len({line['id'] for line in dialogue['lines']})==20,'Unique voice identifiers')
for line in dialogue['lines']:
    check(bool(line['speaker']) and bool(line['text']) and (art/('S_Voice_'+line['id']+'.wav')).is_file(),'Spoken line has matching subtitles and audio: '+line['id'])
for cue in ['BorerPulse','OreChime','JetThrust','ShardChirp','BroodCall','FieldLatch']:
    check((art/('S_'+cue+'.wav')).is_file(),'Gameplay sound is present: '+cue)
for model in ['ShardDrone','BroodRunner','BroodSpitter','FieldWall','FieldFloor','FieldRamp','FieldRoof']:
    check((art/('SM_'+model+'.obj')).is_file(),'Fieldworks mesh is present: '+model)
for p in art.glob('*.wav'):
    with wave.open(str(p),'rb') as w:
        samples=struct.unpack('<'+'h'*(w.getnframes()*w.getnchannels()),w.readframes(w.getnframes()))
        check(w.getnchannels() in (1,2) and w.getsampwidth()==2 and any(samples),'Playable non-silent PCM: '+p.name)
        check(max(abs(v) for v in samples)<32767,'No clipped samples: '+p.name)
for name,width,front in [('SM_Shop',170,-500),('SM_Warehouse',290,-900)]:
    v,f=mesh(art/(name+'.obj'))
    for x in [-width,0,width]:
        for z in [30,110,210]: check(not ray(v,f,(x,front-200,z),(x,front+200,z)),'Clear entrance: '+name)
layout=json.loads((ROOT/'Content/Data/starting-area.json').read_text())
check(len(layout['actors'])>=250,'Authored starting district has detailed placements')
check(sum(bool(a.get('decor')) for a in layout['actors'])>=150,'Starting area contains forest-floor and ruin details')
for a in layout['actors']:
    if a.get('decor'): check(a.get('collision')==False and a.get('cull_distance',999999)<=35000,'Small baked decoration has bounded distance and does not block travel')
for a in layout['actors']:
    check((art/(a['mesh']+'.obj')).is_file() and all(math.isfinite(v) for v in a['position']+list(a['scale'])),'Baked placement references valid art')
project=json.loads((ROOT/'EARTHtotaled.uproject').read_text())
check(project['EngineAssociation']=='5.8','Stable 5.8 target')
check(not (ROOT/'Binaries/Win64/EARTHtotaled.exe').exists() or (ROOT/'Saved/content-ready.txt').exists(),'No executable presented without imported content')
print(f'{checks} content checks passed. Unreal compilation, rendering and playtesting require separate engine checks.')
