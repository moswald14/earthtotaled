"""Original, offline art source. Centimeters, Z up, X forward. No external assets."""
from pathlib import Path
import math, random, json, struct, subprocess, zlib, wave as wavfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource'
OUT.mkdir(exist_ok=True)

from meshkit import Mesh, STATS

house=Mesh(); house.box((0,0,0),(1240,1040,32))
# Walls are segmented around real openings, so collision leaves doors/windows open.
def wall(y, holes):
    xs=sorted(set([-600,600]+[x for lo,hi,bottom,top in holes for x in (lo,hi)])); zs=sorted(set([16,350]+[z for lo,hi,bottom,top in holes for z in (bottom,top)]))
    for a,b in zip(xs,xs[1:]):
        for c,d in zip(zs,zs[1:]):
            x,z=(a+b)/2,(c+d)/2
            if any(lo<x<hi and bottom<z<top for lo,hi,bottom,top in holes): continue
            house.box((x,y,z),(b-a,26,d-c))
wall(-500,[(-76,76,16,255),(-470,-245,125,260),(245,470,125,260)])
wall(500,[(-470,-245,125,260),(180,420,125,260)])
for x in [-600,600]:
    house.box((x,0,83),(26,1000,134)); house.box((x,0,310),(26,1000,80))
    for y,h in [(-370,260),(0,220),(370,260)]: house.box((x,y,200),(26,h,140))
    for y in [-245,245]: house.box((x,y,120),(42,250,14))
# Interior partition with a generous human-sized opening.
house.box((110,250,182),(20,500,332)); house.box((110,-340,182),(20,320,332)); house.box((110,-35,310),(20,290,80))
# Concrete lintels, sills, foundation courses and parapet detail.
for y in [-520,520]:
    house.box((0,y,345),(1260,32,18))
    for x in [-357,357]: house.box((x,y,123),(245,45,15))
for x in [-620,620]: house.box((x,0,345),(32,1060,18))
# Recessed window reveals, trim and individual foundation stones preserve the openings.
house.paint('Steel')
for y in [-536,536]:
    for x in [-470,-245,245,470]: house.box((x,y,195),(6,10,138),bevel=.5)
house.paint('Base')
for x in range(-560,601,160):
    if abs(x)<100: continue
    house.box((x,-517,51),(148,10,60),bevel=2)
house.save('SM_House')
tenement=Mesh()
for floor in range(3):
    tenement.copy_from(house,(0,0,floor*380))
    if floor:
        tenement.box((390,-620,floor*380),(2040,210,32))
        for x in range(-600,1400,170): tenement.beam((x,-710,floor*380+16),(x,-710,floor*380+106),3,8,1)
        tenement.beam((-600,-710,floor*380+106),(1400,-710,floor*380+106),3,8,1)
# Exterior switchback fire escape: 19 cm risers, 50 cm treads, 140 cm clear width.
for floor in range(2):
    for i in range(20):
        y=-475+i*50 if floor==0 else 475-i*50
        top=16+floor*380+(i+1)*19
        tenement.box((780 if floor==0 else 1000,y,top-9),(150,50,18))
    tenement.box((1050,535,(floor+1)*380),(700,160,32))
tenement.box((1240,-40,380),(140,1160,32))
tenement.box((1050,-520,760),(500,160,32))
tenement.box((0,0,1140),(1290,1090,38))
tenement.save('SM_Tenement')
block=Mesh(); block.box((0,0,0),(100,100,100),bevel=0); block.save('SM_Block')
roof=Mesh()
roof.box((0,0,370),(1290,1090,38))
for y in [-530,530]: roof.box((0,y,405),(1290,22,40))
for x in [-635,635]: roof.box((x,0,405),(22,1090,40))
# Vents, rain collector, ladder and external supports.
roof.box((330,280,421),(190,160,60)); roof.box((330,280,460),(210,180,12))
for y in [-42,42]: roof.beam((-625,y,20),(-625,y,430),5,10,1)
for z in range(40,421,28): roof.beam((-630,-42,z),(-630,42,z),3,8,1)
roof.paint('DarkMetal')
for x in range(245,426,18): roof.box((x,280,467),(7,160,5),bevel=.5)
roof.beam((-400,360,390),(-400,360,550),26,40,1)
roof.torus((-400,360,550),26,4,axis='z')
roof.save('SM_Roof')
interior=Mesh()
for x,y in [(-370,300),(350,300)]:
    interior.box((x,y,78),(160,80,12))
    for dx in [-65,65]:
        for dy in [-28,28]: interior.box((x+dx,y+dy,40),(8,8,76))
    interior.box((x-40,y,102),(45,36,40))
for z in [40,110,180,250]: interior.box((-545,-50,z),(70,260,9))
for y in [-175,75]: interior.box((-545,y,160),(70,8,320))
interior.box((370,-240,45),(240,140,28)); interior.box((485,-240,77),(14,140,80))
interior.save('SM_Interior')

for name,size in [('SM_Cache',(58,42,34)),('SM_Core',(26,26,65)),('SM_Trap',(48,48,8))]:
    m=Mesh(); m.box((0,0,size[2]/2),size)
    if name=='SM_Cache':
        m.box((0,0,35),(61,45,6))
        for x in [-20,20]: m.box((x,0,20),(4,46,40))
    if name=='SM_Core':
        for z in range(12,62,10): m.box((0,0,z),(36,36,4))
    if name=='SM_Trap':
        for i in range(14):
            a=i*math.tau/14; m.beam((math.cos(a)*22,math.sin(a)*22,7),(math.cos(a)*16,math.sin(a)*16,22),2,6,.15)
    m.save(name)

tree=Mesh(); rng=random.Random(73)
tree.beam((0,0,0),(25,-15,730),28,16,.22)
for i in range(18):
    z=220+i*25; a=i*2.4; x,y=math.cos(a)*rng.uniform(100,220),math.sin(a)*rng.uniform(100,220)
    tree.beam((z*.034,-z*.02,z),(x,y,z+140),9,10,.18)
    tree.beam((x*.65,y*.65,z+90),(x*.8+35,y*.7-40,z+210),4,8,.12)
tree.save('SM_Tree')
trunk=Mesh(); trunk.beam((0,0,0),(0,0,1000),29,10,.23); trunk.save('SM_Trunk')
pine=Mesh(); pine.paint('Bark'); pine.beam((0,0,0),(0,0,1600),29,32,.03); rng=random.Random(138)
for tier in range(16):
    z=260+tier*78; radius=360-tier*20
    for branch in range(13):
        a=branch*math.tau/13+tier*1.83
        end=(math.cos(a)*radius,math.sin(a)*radius,z-30)
        pine.paint("Bark"); pine.beam((0,0,z),end,6,16,.15); pine.paint("Base")
        for q in range(5):
            aa=a+(q-2)*.19; r=radius*(.34+q*.13)
            x,y=math.cos(aa)*r,math.sin(aa)*r
            for face in range(9):
                t=face*math.tau/9
                t2=(face+1)*math.tau/9
                pine.face([(x+math.cos(t)*90,y+math.sin(t)*90,z-55),(x+math.cos(t2)*90,y+math.sin(t2)*90,z-55),(x*.8,y*.8,z+130)])
pine.save('SM_Pine')
wreck=Mesh(); wreck.box((0,0,55),(440,185,65)); wreck.box((-30,0,110),(220,168,55))
for y in [-90,90]:
    for x in [-140,140]: wreck.beam((x,y-12,40),(x,y+12,40),39,16,1)
    for x in [-135,85]: wreck.beam((x,y*.78,110),(x+30,y*.72,175),8,8,1)
    wreck.beam((-105,y*.72,175),(115,y*.72,175),8,8,1)
wreck.box((5,0,178),(228,146,8)); wreck.box((208,0,83),(18,172,12)); wreck.save('SM_Wreck')
rock=Mesh()
# Sculpted boulder: many coherent rings, rather than four faceted bands.
segments=48; rings=[]
for j in range(17):
    z=-8+j*8; latitude=math.pi*(.10+.84*j/16); radius=95*math.sin(latitude)
    rings.append([(math.cos(i*math.tau/segments)*radius*(1+.12*math.sin(i*math.tau/segments*3+j*.21)),math.sin(i*math.tau/segments)*radius*(1+.08*math.cos(i*math.tau/segments*5-j*.15)),z+5*math.sin(i*math.tau/segments*4+j*.17)) for i in range(segments)])
for a,b in zip(rings,rings[1:]):
    for i in range(segments): j=(i+1)%segments; rock.face([a[i],a[j],b[j],b[i]])
rock.face(list(reversed(rings[0]))); rock.face(rings[-1]); rock.save('SM_Rock')

grass=Mesh(); rng=random.Random(91)
for i in range(23):
    x,y=rng.uniform(-45,45),rng.uniform(-45,45); h=rng.uniform(30,90); a=rng.random()*math.tau; dx,dy=math.cos(a)*2.2,math.sin(a)*2.2
    q=[(x-dx,y-dy,0),(x+dx,y+dy,0),(x+dx*2+15,y+dy*2,h*.65),(x+20,y+7,h)]
    grass.face(q); grass.face(list(reversed(q)))
grass.save('SM_Grass')

for name in ['SM_Robot','SM_Contact']:
    m=Mesh(); robot=name=='SM_Robot'
    m.ellipsoid((0,0,123),(26,33,32),32,16)
    m.box((12,0,132),(29,56,37),bevel=4)
    m.ellipsoid((0,0,92),(22,27,15))
    m.paint('Steel' if robot else 'Skin')
    m.beam((0,0,143),(0,0,162),9,28,1)
    m.ellipsoid((0,0,171),(18,18,23),32,18)
    if not robot:
        m.ellipsoid((18,0,170),(5,7,5),20,10)
        for y in [-18,18]: m.ellipsoid((0,y,173),(6,3,8),20,10)
    m.paint('Rubber'); m.box((14,0,178),(10,33,10),bevel=3)
    m.paint('Signal' if robot else 'DarkMetal')
    for y in [-9,9]: m.ellipsoid((20,y,178),(2.8,4,3),20,10)
    for y in [-20,20]:
        m.paint('Base'); m.ellipsoid((0,y,78),(13,13,27)); m.beam((0,y,95),(5,y,48),10,32,.8)
        m.paint('Steel' if robot else 'Rubber'); m.ellipsoid((7,y,49),(12,12,13),24,12)
        m.paint('Base'); m.beam((5,y,44),(0,y,15),8,28,.9)
        m.paint('Rubber'); m.box((8,y,10),(37,22,18),bevel=5)
        m.paint('Base'); m.ellipsoid((0,y*1.75,142),(14,14,16)); m.beam((0,y*1.8,141),(4,y*1.9,110),10,28,.8)
        m.paint('Steel' if robot else 'Rubber'); m.ellipsoid((4,y*1.9,108),(9,10,9),24,12)
        m.paint('Base'); m.beam((4,y*1.9,104),(20,y*1.8,90),9,28,.7)
        m.paint('Rubber'); m.ellipsoid((22,y*1.8,87),(8,7,10),24,12)
        m.box((-22,y,125),(10,20,32),bevel=2)
    m.paint('Rubber'); m.box((-31,0,125),(20,48,52),bevel=5)
    m.paint('Steel');
    for z in range(110,143,8): m.box((-43,0,z),(3,36,3),bevel=.5)
    m.save(name)
for name,l,h in [('SM_Deer',120,120),('SM_Boar',100,65),('SM_Wolf',110,85)]:
    m=Mesh(); m.ellipsoid((0,0,h),(l*.55,27 if name=='SM_Boar' else 23,34),36,18)
    m.ellipsoid((-l*.32,0,h-3),(26,26,32),28,14); m.ellipsoid((l*.28,0,h+5),(27,25,35),28,14)
    m.beam((l/2-10,0,h),(l/2+22,0,h+38),19,32,.7)
    m.ellipsoid((l/2+25,0,h+38),(25,16,20),32,16); m.ellipsoid((l/2+46,0,h+32),(20,11,10),28,14)
    for x in [-l*.33,l*.33]:
        for y in [-17,17]:
            knee=(x-7,y,h*.42); m.beam((x,y,h-10),knee,7,24,.65); m.ellipsoid(knee,(6,6,8),20,10)
            m.beam(knee,(x-9,y,7),4.5,24,.7); m.paint('Rubber'); m.ellipsoid((x-5,y,6),(8,6,6),20,10); m.paint('Base')
    m.beam((-l/2,0,h+5),(-l/2-35,0,h-15),9 if name=='SM_Wolf' else 5,24,.1)
    for y in [-12,12]:
        m.beam((l/2+20,y,h+47),(l/2+12,y*2,h+75),8,24,.1)
        m.paint('Rubber'); m.ellipsoid((l/2+34,y*1.15,h+45),(3,2,3),20,10); m.paint('Base')
    m.paint('Rubber'); m.ellipsoid((l/2+63,0,h+34),(5,10,6),24,12); m.paint('Base')
    if name=='SM_Deer':
        m.paint('Wood')
        for y in [-12,12]:
            m.beam((l/2+12,y,h+56),(l/2,y*3,h+110),3,24,.3)
            for z in [75,95]: m.beam((l/2+5,y*2,h+z),(l/2+22,y*3,h+z+12),2,20,.2)
    m.save(name)
for i in range(10):
    m=Mesh(); length=[65,75,80,45,90,85,32,71,42,108][i]
    # Receiver, separate stock/grip materials, smooth machined barrels and visible controls.
    m.box((0,0,0),(length*.6,9,12),bevel=1.2)
    m.paint('Wood' if i in [0,1,6,9] else 'Rubber'); m.box((-length*.36,0,-2),(20 if i!=9 else 30,11,15),bevel=2)
    m.box((-12,0,-10),(7,8,18),bevel=1.4)
    for z in range(-17,-4,3): m.box((-12,-4.1,z),(6,.7,.7),bevel=.2)
    m.paint('DarkMetal'); m.box((0,0,-12),(12,7,17),bevel=.9)
    m.box((0,0,8),(length*.55,5,4),bevel=.4)
    for y in ([-3,3] if i==1 else [0]):
        m.beam((length*.24,y,2),(length*.65,y,2),2.3,48,1)
        m.torus((length*.65,y,2),2.4,.5,segments=40,sides=8)
        m.paint('Rubber'); m.beam((length*.65+.04,y,2),(length*.65+.1,y,2),1.65,40,1); m.paint('DarkMetal')
        for x in range(int(length*.27),int(length*.50),4): m.torus((x,y,2),2.45,.35,segments=32,sides=6)
    m.paint('Base')
    for x in range(-10,int(length*.25),3): m.box((x,0,10.7),(1.1,6,1.3),bevel=.25)
    # Screws, ejection port, charging handle, front sight and a curved trigger guard.
    m.paint('Rubber'); m.box((5,-4.55,1),(12,.4,4),bevel=.3)
    m.paint('Base'); m.beam((5,-4,0),(5,-9,0),1,24,1)
    for x in [-12,12]:
        for y in [-4.8,4.8]: m.beam((x,y*.9,3),(x,y,3),1,24,1)
    m.box((length*.42,0,7),(3,4,8),bevel=.5); m.box((-length*.2,0,12),(3,8,3),bevel=.5)
    m.paint('DarkMetal'); m.torus((-7,0,-11),5,.65,axis='y',segments=32,sides=8)
    m.beam((-7,0,-6),(-5,0,-12),.55,20,1)
    if i in [2,4,9]:
        m.paint('DarkMetal'); m.beam((-14,0,17),(14,0,17),4.5 if i!=9 else 5.8,48,1)
        for x in [-14,-7,7,14]: m.torus((x,0,17),4.8 if i!=9 else 6.1,.8,segments=40,sides=10)
        m.paint('Glass'); m.beam((-14.1,0,17),(-14.2,0,17),3.8 if i!=9 else 5,40,1)
        m.paint('Base'); m.box((-6,0,12),(6,7,6),bevel=.8)
    if i==5:
        m.paint('Rubber'); m.box((-2,10,-9),(20,13,18),bevel=2)
        m.paint('Gold')
        for x in range(-10,14,4): m.beam((x,3,1),(x,12,1),1.2,24,1)
    if i==6:
        m.paint('Base'); m.beam((-6,0,0),(5,0,0),6.5,48,1)
        for a in range(6):
            y,z=math.cos(a*math.tau/6)*4.8,math.sin(a*math.tau/6)*4.8
            m.paint('Rubber'); m.beam((5.03,y,z),(5.08,y,z),1.1,20,1)
        m.paint('Wood'); m.box((-10,0,-18),(9,8,22),-12,bevel=2)
    if i==7:
        m.paint('DarkMetal'); m.beam((2,-9,-9),(2,9,-9),10,48,1); m.torus((2,9,-9),8,1,axis='y')
    if i==8:
        m.paint('Rubber'); m.box((4,0,12),(22,12,5),bevel=1); m.box((10,0,-10),(17,5,24),-10,bevel=1)
    if i==9:
        m.paint('Base'); m.beam((-4,0,3),(-4,13,4),1.8,32,1); m.ellipsoid((-4,14,4),(3,3,3),24,12)
    m.save('SM_Weapon'+str(i))

tower=Mesh()
for x in [-150,150]:
    for y in [-150,150]: tower.beam((x,y,0),(x*.25,y*.25,3200),12,12,.5)
for z in range(100,3200,220):
    r=150*(1-z/4300)
    for y in [-r,r]: tower.beam((-r,y,z),(r,y,z),5,8,1); tower.beam((-r,y,z),(r,y,z+220),4,8,1)
tower.beam((0,0,3000),(0,0,3900),8,12,.3); tower.save('SM_Tower')

def png(path,w,h,data):
    def chunk(kind,content): return struct.pack('>I',len(content))+kind+content+struct.pack('>I',zlib.crc32(kind+content)&0xffffffff)
    scan=b''.join(b'\0'+bytes(data[y*w*3:(y+1)*w*3]) for y in range(h))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(scan,6))+chunk(b'IEND',b''))

palette={'Ground':(67,62,44),'Concrete':(125,121,106),'Steel':(80,91,88),'Rust':(106,63,38),'Wood':(95,77,49),'Bark':(51,47,37),'Canvas':(112,112,77),'Hide':(99,87,65),'Grass':(75,85,42),'Gold':(172,126,43),'Signal':(125,78,182),'Greenline':(69,169,109),'Asphalt':(43,44,39),'Pine':(32,49,29),'DarkMetal':(29,35,38),'Rubber':(22,25,24),'Glass':(35,79,91),'Skin':(150,112,84)}
for index,(name,color) in enumerate(palette.items()):
    N=2048 if name in ["Ground","Concrete","Steel","Rust","Wood","Bark","DarkMetal","Rubber"] else 1024
    rng=random.Random(index+849); heights=[]; period=N/512
    for y in range(N):
        for x in range(N):
            grain=rng.random()*.10
            wave=math.sin(x*math.tau/N*5+math.sin(y*math.tau/N*3))*math.sin(y*math.tau/N*7)*.1
            if name=='Ground': wave=wave*.2+(rng.random()-.5)*.33
            if name in ['Wood','Bark']: wave+=math.sin(x/period*.21+math.sin(y/period*.025)*3)*.16
            if name=='Concrete': wave+=math.sin(x/period*.057+math.sin(y/period*.04)*2)*.07
            if name in ['Steel','DarkMetal']: wave=wave*.25+math.sin(y/period*5)*.02
            if name=='Rubber': wave=math.sin(x/period*2)*math.sin(y/period*2)*.06
            if name=='Rust': wave+=max(0,math.sin(x/period*.08)*math.cos(y/period*.05))*.25
            heights.append(.5+wave+grain)
    base=bytearray(); rough=bytearray(); normal=bytearray()
    for y in range(N):
        for x in range(N):
            h=heights[y*N+x]; variation=.7+h*.5
            base.extend(max(0,min(255,int(c*variation))) for c in color)
            r=int((.32 if name in ['Steel','Gold','DarkMetal'] else .12 if name=='Glass' else .83)*255+(h-.5)*25); rough.extend([r]*3)
            dx=(heights[y*N+(x+1)%N]-heights[y*N+(x-1)%N])*.5
            dy=(heights[((y+1)%N)*N+x]-heights[((y-1)%N)*N+x])*.5
            normal.extend([int(128-dx*127),int(128-dy*127),254])
    for suffix,data in [('BaseColor',base),('Roughness',rough),('Normal',normal)]: png(OUT/f'T_{name}_{suffix}.png',N,N,data)
for i in range(10):
    rng=random.Random(i+189); rate=44100; samples=bytearray()
    for n in range(int(rate*.42)):
        t=n/rate; noise=rng.uniform(-1,1); envelope=math.exp(-t*(16 if i==1 else 23))
        value=(noise*.62+math.sin(t*math.tau*(74+i*18))*math.exp(-t*14)*.38)*envelope
        samples.extend(struct.pack('<h',int(max(-1,min(1,value))*.8*32767)))
    with wavfile.open(str(OUT/f'S_Fire{i}.wav'),'wb') as f: f.setnchannels(1); f.setsampwidth(2); f.setframerate(rate); f.writeframes(samples)
# Buildings have full-size, unobstructed entrances; props are separate for varied layouts.
shop=Mesh(); shop.box((0,0,0),(1440,1040,32))
for x in [-700,700]: shop.box((x,0,210),(28,1000,420))
shop.box((0,500,210),(1400,28,420))
for x in [-450,450]:
    shop.box((x,-500,65),(500,28,100)); shop.box((x,-500,370),(500,28,100))
for x in [-700,-200,200,700]: shop.box((x,-500,210),(28,28,420))
shop.box((0,0,430),(1480,1080,28)); shop.box((0,-650,350),(1500,300,24)); shop.save('SM_Shop')
warehouse=Mesh(); warehouse.box((0,0,0),(2640,1840,32))
for x in [-1300,1300]: warehouse.box((x,0,340),(35,1800,680))
warehouse.box((0,900,340),(2600,35,680))
for x in [-810,810]: warehouse.box((x,-900,340),(980,35,680))
warehouse.box((0,-900,610),(640,35,140))
for x in [-1200,0,1200]: warehouse.beam((x,-900,680),(x,0,850),12,8,1); warehouse.beam((x,0,850),(x,900,680),12,8,1)
warehouse.face([(-1340,-940,680),(1340,-940,680),(1340,0,850),(-1340,0,850)])
warehouse.face([(-1340,0,850),(1340,0,850),(1340,940,680),(-1340,940,680)])
warehouse.save('SM_Warehouse')
canopy=Mesh()
for x in [-900,900]:
    for y in [-500,500]: canopy.box((x,y,250),(32,32,500))
canopy.box((0,0,510),(2000,1400,50)); canopy.save('SM_Canopy')
pump=Mesh(); pump.box((0,0,65),(65,50,130)); pump.box((0,0,145),(85,60,35)); pump.box((0,-32,148),(58,6,20))
pump.beam((45,0,130),(65,0,30),5,10,1); pump.save('SM_Pump')
tank=Mesh(); tank.beam((0,0,70),(0,0,400),150,24,1)
for x,y in [(-100,-100),(-100,100),(100,-100),(100,100)]: tank.box((x,y,35),(16,16,70))
tank.beam((0,0,400),(0,0,445),24,12,1); tank.save('SM_WaterTank')
barrel=Mesh(); barrel.beam((0,0,0),(0,0,105),42,20,1)
for z in [8,45,95]: barrel.beam((0,0,z),(0,0,z+6),45,20,1)
barrel.save('SM_Barrel')
shelves=Mesh()
for z in [25,95,165,235]: shelves.box((0,0,z),(220,60,10))
for x in [-105,105]:
    for y in [-25,25]: shelves.box((x,y,130),(8,8,260))
for x,z in [(-70,50),(50,120),(-20,195)]: shelves.box((x,0,z),(45,40,40))
shelves.save('SM_Shelves')
bench=Mesh(); bench.box((0,0,47),(190,55,12)); bench.box((0,28,77),(190,8,55))
for x in [-65,65]: bench.box((x,0,24),(14,45,42))
bench.save('SM_Bench')
lamp=Mesh(); lamp.beam((0,0,0),(0,0,570),10,12,.65); lamp.beam((0,0,570),(0,-170,570),7,10,1); lamp.box((0,-175,555),(75,110,16)); lamp.save('SM_Lamp')
sign=Mesh()
for x in [-180,180]: sign.beam((x,0,0),(x,0,400),8,10,1)
sign.box((0,0,330),(440,24,130)); sign.save('SM_Sign')
tent=Mesh()
for x in [-250,250]:
    for y in [-250,250]: tent.beam((x,y,0),(x,y,320 if y<0 else 200),7,10,1)
tent.face([(-300,-300,335),(300,-300,335),(300,300,190),(-300,300,190)])
tent.face([(-300,300,190),(300,300,190),(300,-300,335),(-300,-300,335)])
tent.box((0,230,60),(550,20,120)); tent.save('SM_CampShelter')
for name,size in [('SM_Toolbox',(64,30,32)),('SM_MedCase',(48,36,22)),('SM_PantryCase',(50,45,50)),('SM_WeaponCase',(105,40,22)),('SM_ElectronicsCase',(40,38,42))]:
    m=Mesh(); w,d,h=size; m.box((0,0,h/2),size); m.box((0,0,h+2),(w+3,d+3,5))
    m.box((0,0,h+8),(w*.35,8,8))
    if name=='SM_MedCase': m.box((0,-d/2-2,h*.55),(w*.52,4,5)); m.box((0,-d/2-3,h*.55),(5,4,h*.7))
    else:
        for x in [-w*.3,w*.3]: m.box((x,-d/2-2,h/2),(5,4,h))
    m.save(name)

# Original foley and looping soundscapes, generated offline. No downloaded samples.
def sound(name,duration,fn):
    rate=22050; rng=random.Random(name); data=bytearray(); length=int(rate*duration)
    for n in range(length):
        t=n/rate; edge=min(1,t/.025,(duration-t)/.025)
        value=max(-.98,min(.98,fn(t,rng)*edge))
        data.extend(struct.pack('<h',int(value*32767)))
    with wavfile.open(str(OUT/('S_'+name+'.wav')),'wb') as f: f.setnchannels(1); f.setsampwidth(2); f.setframerate(rate); f.writeframes(data)
def click(t,r,pulses): return sum((r.uniform(-1,1)*.65+math.sin(t*math.tau*160)*.25)*math.exp(-(t-p)*55) for p in pulses if t>=p)
sound('Reload',1.8,lambda t,r:click(t,r,[.08,.3,1.12,1.48]))
sound('Empty',.2,lambda t,r:click(t,r,[.01])*.8)
sound('Switch',.25,lambda t,r:click(t,r,[.02,.12])*.7)
sound('Pickup',.48,lambda t,r:(math.sin(t*math.tau*(660 if t<.16 else 990))*.26+r.uniform(-.06,.06))*math.exp(-t*6))
sound('Drink',1.2,lambda t,r:sum(math.sin(t*math.tau*(170+math.sin(t*24)*80))*.3*math.exp(-(t-p)*15) for p in [.08,.3,.55,.78] if t>=p))
sound('Craft',1.1,lambda t,r:click(t,r,[.05,.26,.48,.72])*.65+math.sin(t*math.tau*1320)*.12*math.exp(-t*3))
sound('Heal',.7,lambda t,r:r.uniform(-1,1)*.22*math.sin(t*math.pi/.7)**2)
sound('StepForest',.25,lambda t,r:(r.uniform(-1,1)*.5+math.sin(t*math.tau*85)*.3)*math.exp(-t*18))
sound('StepCity',.3,lambda t,r:(r.uniform(-1,1)*.28+math.sin(t*math.tau*120)*.5)*math.exp(-t*24))
sound('Hit',.18,lambda t,r:click(t,r,[.01])*.8)
sound('Robot',.6,lambda t,r:(math.sin(t*math.tau*(240+40*math.sin(t*35)))*.22+r.uniform(-1,1)*.2)*math.exp(-t*6))
sound('Animal',.8,lambda t,r:(math.sin(t*math.tau*(130+math.sin(t*17)*45))*.3+r.uniform(-1,1)*.13)*math.sin(t*math.pi/.8)**2)
sound('ForestDay',12,lambda t,r:r.uniform(-1,1)*.025*(1+.4*math.sin(t*math.tau/12))+math.sin(t*math.tau*1800+math.sin(t*23)*14)*.055*max(0,math.sin(t*math.tau/3))**24)
sound('ForestNight',12,lambda t,r:r.uniform(-1,1)*.018+math.sin(t*math.tau*3100)*.035*max(0,math.sin(t*19))**4)
sound('CityWind',12,lambda t,r:r.uniform(-1,1)*.04*(1+.6*math.sin(t*math.tau/12))+math.sin(t*math.tau*55)*.045+math.sin(t*math.tau*430)*.018*max(0,math.sin(t*math.tau/6))**20)
import runpy
runpy.run_path(str(Path(__file__).with_name('environment-art.py')))['generate']()
runpy.run_path(str(Path(__file__).with_name('make-wrappers.py')))['generate']()
runpy.run_path(str(Path(__file__).with_name('make-atmosphere.py')))['generate']()
runpy.run_path(str(Path(__file__).with_name('make-geology.py')),run_name='__main__')
runpy.run_path(str(Path(__file__).with_name('make-frontier.py')),run_name='__main__')
runpy.run_path(str(Path(__file__).with_name('make-outposts.py')),run_name='__main__')
runpy.run_path(str(Path(__file__).with_name('make-reclamation.py')),run_name='__main__')
runpy.run_path(str(Path(__file__).with_name('make-ringline.py')))['generate_railway']()
runpy.run_path(str(Path(__file__).with_name('make-interchange.py')))['generate_landmarks']()
subprocess.run(['node',str(Path(__file__).with_name('make-relief.mjs'))],check=True)
(OUT/'mesh-detail.json').write_text(json.dumps(STATS,indent=2))
(OUT/'manifest.json').write_text(json.dumps({'units':'centimeters','axis':'Z up / X forward','license':'Original EARTHtotaled project art. No third-party assets.','meshes':sorted(p.name for p in OUT.glob('*.obj')),'textures':sorted(p.name for p in OUT.glob('*.png')),'sounds':sorted(p.name for p in OUT.glob('*.wav'))},indent=2))
print(f'Created {len(list(OUT.glob("*.obj")))} meshes and {len(list(OUT.glob("*.png")))} PBR textures.')
