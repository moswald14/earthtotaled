"""Author the fixed shelter district offline. bootstrap-content bakes it into the map."""
from pathlib import Path
import json, random, math
root=Path(__file__).resolve().parents[1]
actors=[]
buildings=[]
def put(mesh,mat,pos,scale=(1,1,1),yaw=0):
    actors.append(dict(mesh=mesh,material=mat,position=pos,scale=scale,yaw=yaw))
def house(p,roof='Steel'):
    buildings.append((p[0],p[1],850,850))
    for mesh,mat in [('SM_House','Concrete'),('SM_Roof',roof),('SM_Interior','Wood')]: put(mesh,mat,p)
for p in [(0,0,0),(-12000,-4000,0),(11500,8500,0),(-9000,8500,0),(-11500,11500,0),(12500,-9000,0),(-12500,-11500,0)]: house(p)
for row in range(-1,2):
    put('SM_Block','Asphalt',(0,row*7000+2600,7),(320,6,.08))
    put('SM_Block','Asphalt',(row*7000+2600,0,7),(6,320,.08))
    for side in [-1,1]:
        put('SM_Block','Concrete',(0,row*7000+2600+side*370,14),(320,1.2,.2))
        put('SM_Block','Concrete',(row*7000+2600+side*370,0,14),(1.2,320,.2))
    for dash in range(-15,16,2):
        put('SM_Block','Canvas',(dash*1000,row*7000+2600,12),(3,.12,.02))
        put('SM_Block','Canvas',(row*7000+2600,dash*1000,12),(.12,3,.02))
    for col in range(-1,2):
        p=(col*7000+1000,row*7000-1800,0)
        if p[0]**2+p[1]**2>3500**2: house(p,'Rust')
# A clear, deliberate arrival: shelter at the end of a promenade, shops on either side.
put('SM_Block','Concrete',(0,-1400,0),(10,16,.16))
for x in [-950,950]:
    put('SM_Lamp','Steel',(x,-1300,0)); put('SM_Bench','Wood',(x,-1800,0),yaw=90 if x<0 else -90)
    put('SM_CampShelter','Canvas',(x*1.6,-200,0)); put('SM_WaterTank','Greenline',(x*1.5,650,0))
for x in [-2800,2800]:
    buildings.append((x,-800,1050,900))
    put('SM_Shop','Concrete',(x,-800,0)); put('SM_Shelves','Wood',(x-550,-600,16),yaw=90)
    put('SM_Shelves','Wood',(x+450,-450,16))
    put('SM_Sign','Greenline',(x,-1500,0))
put('SM_Canopy','Steel',(5000,-1800,0)); put('SM_Shop','Concrete',(5000,300,0))
buildings.extend([(5000,-1800,1200,1000),(5000,300,1100,900),(-5400,1100,1950,1250)])
for x in [4500,5500]: put('SM_Pump','Rust',(x,-1800,0))
put('SM_Warehouse','Concrete',(-5400,1100,0))
for x in [-6200,-4600]: put('SM_Shelves','Steel',(x,1600,16))
for p in [(-7200,-6500,0),(8200,4000,0),(-2500,6500,0),(6500,11500,0)]:
    # Include the fire escape and its approaches in the protected building footprint.
    buildings.append((p[0]+300,p[1],1250,1100))
    put('SM_Tenement','Concrete',p); put('SM_Interior','Wood',p)
    for z in [380,760]: put('SM_Interior','Wood',(p[0],p[1],z))
put('SM_Tower','Steel',(0,-15500,0))
put('SM_Sign','Greenline',(-700,-700,0))
r=random.Random(6184)
for i in range(12): put('SM_Wreck','Rust',(-2600+i*1250,2950,0),yaw=r.uniform(-25,25))
for row in [-4400,2600,9600]:
    for x in [-14000,-7000,0,7000,14000]: put('SM_Lamp','Steel',(x,row+470,0))
for x,y in [(-2200,550),(-2000,550),(2000,650),(2300,650),(-5000,2100),(-5200,2100),(5400,500)]: put('SM_Barrel','Rust',(x,y,0))
# A mixed pocket grove occupies the bank south of the apartment's entrance.
# Spaced trunks replace the former random trees that could overlap that building.
grove=[]
for i in range(70):
    x,y=r.uniform(6500,9000),r.uniform(-3650,-2800)
    if any(math.hypot(x-px,y-py)<360 for px,py in grove): continue
    if any(math.hypot(x-bx,y-by)<250 for bx,by in [(6800,-3300),(8700,-3300)]): continue
    grove.append((x,y))
    mesh='SM_Birch' if len(grove)%3==0 else 'SM_Pine'
    s=r.uniform(.42,.65)
    put(mesh,'Bark' if mesh=='SM_Birch' else 'Pine',(x,y,0),(s,s,r.uniform(.55,.85)),r.uniform(0,360))
for p in [(6800,-3300,0),(8700,-3300,0)]: put('SM_Bench','Wood',p)

# Baked small detail remains cheap and cannot seal doors, stairs or human reward routes.
def decor(mesh,mat,pos,scale=(1,1,1),yaw=0,far=14000):
    put(mesh,mat,pos,scale,yaw)
    actors[-1].update(decor=True,collision=False,cull_distance=far)

roads=[-4400,2600,9600]
protected=[(0,-1500,1100),(-700,-700,600),(1100,500,550),(0,-15000,600)]
protected.extend((2200+i*1900,2000-i*600,220) for i in range(6))
def clear(x,y,radius=100):
    if any(abs(x-road)<440+radius or abs(y-road)<440+radius for road in roads): return False
    if any(abs(x-bx)<hx+radius and abs(y-by)<hy+radius for bx,by,hx,hy in buildings): return False
    if any(math.hypot(x-px,y-py)<d+radius for px,py,d in protected): return False
    return True

# Ruined courtyard gardens: fractured boundary walls, rubble fans and young trees.
ruin_lots=[(-10500,5600),(11700,-6200),(4500,7100),(-10900,-1800)]
for n,(x,y) in enumerate(ruin_lots):
    if clear(x,y,400):
        decor('SM_WallRuin','Concrete',(x,y,0),(1.1,.9,.85+n*.08),r.uniform(-22,22),35000)
        decor('SM_Rebar','Rust',(x+110,y,110),(.8,.8,.8),r.uniform(0,360),24000)
        decor('SM_Rubble','Concrete',(x+80,y-160,-5),(1.2,1.1,.7),r.uniform(0,360),24000)
    for i in range(20):
        a=r.uniform(0,math.tau); d=r.uniform(350,1400); px,py=x+math.cos(a)*d,y+math.sin(a)*d
        if not clear(px,py,170): continue
        if i%7==0:
            decor('SM_Rubble','Concrete',(px,py,-3),(.65,.6,.55),r.uniform(0,360),24000)
        else:
            s=r.uniform(.4,.8); mesh='SM_Shrub' if i%4==0 else 'SM_Fern'
            decor(mesh,'Grass',(px,py,-2),(s,s,s),r.uniform(0,360))
    px,py=x+650,y+550
    if clear(px,py,300):
        decor('SM_Birch','Bark',(px,py,-3),(.8,.75,.9),r.uniform(0,360),35000)

# Close views of the shelter retain a tended center, with weeds and chipped paving at its edges.
for side in [-1,1]:
    for i in range(10):
        x,y=side*r.uniform(1750,2200),-2300+i*115
        if clear(x,y,100):
            s=r.uniform(.3,.5); decor('SM_Fern','Grass',(x,y,0),(s,s,s),r.uniform(0,360))

for i in range(55):
    x,y=r.uniform(6500,9000),r.uniform(-3750,-2750)
    if not clear(x,y,60) or any(math.hypot(x-bx,y-by)<200 for bx,by in [(6800,-3300),(8700,-3300)]): continue
    s=r.uniform(.4,.75)
    decor('SM_Shrub' if i%8==0 else 'SM_Fern','Grass',(x,y,-3),(s,s,s),r.uniform(0,360))
for x,y in [(7350,-3400),(8500,-3500)]:
    if clear(x,y,170): decor('SM_FallenLog','Bark',(x,y,-5),(.65,.65,.65),r.uniform(-20,20),24000)

# Modest debris accumulates behind each building instead of across its usable threshold.
for i,(x,y,hx,hy) in enumerate(buildings):
    px,py=x-350,y+hy+340
    if clear(px,py,200):
        s=r.uniform(.5,.85); decor('SM_Rubble','Concrete',(px,py,-4),(s,s,s*.7),r.uniform(0,360),24000)
        if clear(px+250,py+150,100): decor('SM_Shrub','Grass',(px+250,py+150,-3),(.45,.45,.6),r.uniform(0,360))

out=root/'Content/Data/starting-area.json'
out.write_text(json.dumps({'version':2,'name':'Shelter 09 / Greenline Square','seed':6184,'actors':actors},indent=2))
print(f'Authored {len(actors)} starting-area placements for baking into ExclusionZone.umap.')
