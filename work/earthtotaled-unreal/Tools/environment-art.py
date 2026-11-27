"""Original branching vegetation and weathered architecture, in centimetres.

Run alone for an environment refresh, or from make-art.py for a complete rebuild.
No external art or opaque foliage cones: leaves and needles are actual small surfaces.
"""
import math, random, json, struct, zlib
from pathlib import Path
from meshkit import Mesh, STATS, unit, cross, sub

OUT=Path(__file__).resolve().parents[1]/'ArtSource'
TAU=math.tau
def add(a,b): return tuple(a[i]+b[i] for i in range(3))
def mul(a,s): return tuple(v*s for v in a)
def mix(a,b,t): return add(mul(a,1-t),mul(b,t))

def tube(m,points,radii,sides=10):
    rings=[]; normals=[]
    for j,p in enumerate(points):
        axis=unit(sub(points[min(j+1,len(points)-1)],points[max(0,j-1)]))
        u=unit(cross(axis,(0,0,1) if abs(axis[2])<.94 else (0,1,0))); v=cross(axis,u)
        ns=[add(mul(u,math.cos(i*TAU/sides)),mul(v,math.sin(i*TAU/sides))) for i in range(sides)]
        rings.append([add(p,mul(n,radii[j])) for n in ns]); normals.append(ns)
    for j in range(len(points)-1):
        for i in range(sides):
            k=(i+1)%sides
            m.face([rings[j][i],rings[j][k],rings[j+1][k],rings[j+1][i]],[(i/sides,j*.7),((i+1)/sides,j*.7),((i+1)/sides,(j+1)*.7),(i/sides,(j+1)*.7)],[normals[j][i],normals[j][k],normals[j+1][k],normals[j+1][i]])
    m.face(list(reversed(rings[0]))); m.face(rings[-1])

def leaf(m,p,d,length,width,twist=0,flat=False):
    d=unit(d); side=unit(cross(d,(0,0,1) if abs(d[2])<.9 else (0,1,0)))
    normal=unit(cross(side,d)); side=add(mul(side,math.cos(twist)),mul(normal,math.sin(twist)))
    tip=add(p,mul(d,length)); mid=add(add(p,mul(d,length*.48)),mul(normal,width*.12))
    left=add(mid,mul(side,width)); right=add(mid,mul(side,-width))
    if flat:
        m.face([p,left,tip],[(0,.5),(.48,0),(1,.5)])
        m.face([p,tip,right],[(0,.5),(1,.5),(.48,1)])
        return
    # A folded four-triangle leaf catches light without a bulky solid canopy.
    m.face([p,left,mid],[(0,.5),(.48,0),(.48,.5)])
    m.face([p,mid,right],[(0,.5),(.48,.5),(.48,1)])
    m.face([left,tip,mid],[(.48,0),(1,.5),(.48,.5)])
    m.face([mid,tip,right],[(.48,.5),(1,.5),(.48,1)])

def pine(seed,name,height=1580):
    r=random.Random(seed); m=Mesh(); m.paint('Bark')
    spine=[(20*math.sin(j*.46),15*math.sin(j*.7),j*height/16) for j in range(17)]
    tube(m,spine,[max(1,32*(1-j/17)**1.1) for j in range(17)],28)
    for a in [0,.8,2.1,3.2,4.7]:
        tube(m,[(0,0,45),(math.cos(a)*55,math.sin(a)*55,12),(math.cos(a)*105,math.sin(a)*105,0)],[18,10,1],8)
    for tier in range(14):
        z=310+tier*(height-390)/14; spread=(height-z)*.255+r.uniform(-18,20)
        for b in range(6):
            if r.random()<.11: continue
            a=b*TAU/6+tier*2.399+r.uniform(-.32,.32); direction=(math.cos(a),math.sin(a),0)
            start=(20*math.sin(z/height*7.36),15*math.sin(z/height*11.2),z)
            end=add(start,add(mul(direction,spread), (0,0,r.uniform(-70,20))))
            mid=add(mix(start,end,.55),(0,0,-22))
            m.paint('Bark'); tube(m,[start,mid,end],[max(2.5,spread*.025),3.2,.7],8)
            for t in range(1,6):
                fraction=.22+t*.13; origin=mix(start,end,fraction)
                for side in [-1,1]:
                    twig_angle=a+side*r.uniform(.45,.88)
                    size=spread*(1-fraction)*r.uniform(.45,.7)+22
                    direction2=(math.cos(twig_angle),math.sin(twig_angle),r.uniform(-.15,.1))
                    twig_end=add(origin,mul(direction2,size))
                    m.paint('Bark'); tube(m,[origin,twig_end],[1.7,.2],5)
                    m.paint('Pine' if (tier+b+t)%3 else 'Leaf')
                    for q in range(8):
                        p=mix(origin,twig_end,.10+q*.115)
                        for s in [-1,1]:
                            aa=twig_angle+s*.55
                            leaf(m,p,(math.cos(aa),math.sin(aa),.15+r.uniform(-.35,.35)),r.uniform(17,30),r.uniform(2.1,3.7),r.uniform(-.6,.6))
    m.save(name)

def birch(seed,name,bare=False):
    r=random.Random(seed); m=Mesh(); m.paint('BirchBark' if not bare else 'Bark')
    spine=[(28*math.sin(j*.38),j*5,j*115) for j in range(11)]
    tube(m,spine,[max(2,24-j*2.1) for j in range(11)],14)
    for j in range(13):
        z=410+j*48; a=j*2.399+r.uniform(-.4,.4); length=r.uniform(160,290)*(1-(z-410)/1100)
        p=(28*math.sin(z/115*.38),z/23,z)
        end=add(p,(math.cos(a)*length,math.sin(a)*length,r.uniform(180,310)))
        mid=add(mix(p,end,.5),(0,0,-40)); tube(m,[p,mid,end],[8,4,1],9)
        for k in range(6):
            t=.26+k*.13; origin=mix(p,end,t); aa=a+(-1 if k%2 else 1)*r.uniform(.65,1.3)
            tip=add(origin,(math.cos(aa)*r.uniform(70,130),math.sin(aa)*r.uniform(70,130),r.uniform(25,90)))
            m.paint('BirchBark' if not bare else 'Bark'); tube(m,[origin,mix(origin,tip,.5),tip],[2.5,1.3,.2],6)
            if not bare:
                for q in range(11):
                    pos=mix(origin,tip,.1+q*.085)
                    m.paint('Leaf' if q%4 else 'Moss')
                    for s in [-1,1]:
                        da=aa+s*r.uniform(.65,1.25)
                        leaf(m,pos,(math.cos(da),math.sin(da),r.uniform(-.4,.4)),r.uniform(17,26),r.uniform(6,9),r.uniform(-1,1))
                m.paint('BirchBark')
    m.save(name)

def understory():
    r=random.Random(73); m=Mesh()
    for i in range(11):
        a=i*2.399; length=r.uniform(65,100)
        points=[(math.cos(a)*length*t,math.sin(a)*length*t,12+length*1.05*math.sin(t*2.1)) for t in [0,.25,.5,.75,1]]
        m.paint('Moss'); tube(m,points,[1.2,1,.8,.5,.12],5)
        m.paint('Leaf')
        for j in range(1,16):
            t=j/17; p=(math.cos(a)*length*t,math.sin(a)*length*t,12+length*1.05*math.sin(t*2.1))
            for s in [-1,1]:
                aa=a+s*1.1; leaf(m,p,(math.cos(aa),math.sin(aa),.18),25*math.sin(math.pi*t)**.6,3.2*(1-t)+1.0,s*.2)
    m.save('SM_Fern')
    m=Mesh()
    for i in range(15):
        a=i*2.399; end=(math.cos(a)*r.uniform(50,110),math.sin(a)*r.uniform(50,110),r.uniform(90,175))
        m.paint('Bark'); tube(m,[(0,0,0),mul(end,.55),end],[3.5,1.8,.2],7)
        for j in range(8):
            p=mul(end,.3+j*.09)
            m.paint('Leaf' if i%3 else 'Moss')
            for s in [-1,1]:
                aa=a+s*1.1; leaf(m,p,(math.cos(aa),math.sin(aa),.1),r.uniform(23,38),r.uniform(7,13),r.uniform(-.8,.8))
    m.save('SM_Shrub')
    m=Mesh(); m.paint('Bark')
    tube(m,[(-250,0,28),(-150,3,37),(0,-5,39),(150,4,34),(250,7,28)],[32,35,31,28,25],16)
    for x,a in [(-120,.3),(10,2.5),(160,1.2)]:
        tube(m,[(x,0,40),(x+30,math.cos(a)*45,40+math.sin(a)*45)],[11,3],8)
    m.paint('Wood'); tube(m,[(-252,0,28),(-254,0,28)],[26,26],18)
    m.paint('Moss')
    for x in range(-200,220,40): m.ellipsoid((x,-4,63),(28,17,5),12,6)
    m.save('SM_FallenLog')
    m=Mesh(); m.paint('Bark'); tube(m,[(0,0,0),(4,1,45),(2,3,84)],[47,34,30],18)
    for i in range(8):
        a=i*TAU/8; tube(m,[(2,3,82),(math.cos(a)*r.uniform(22,32),math.sin(a)*28,r.uniform(90,115))],[7,1],6)
    m.paint('Wood'); tube(m,[(2,3,82),(2,3,84)],[24,24],18); m.save('SM_Stump')

def facade(m,half,back,height,seed):
    r=random.Random(seed)
    # Uneven exposed brick patches at the base and window corners, not extra solid walls.
    for y in [-back-16,back+16]:
        for side in [-1,1]:
            for row in range(7):
                for col in range(7-row//2):
                    x=side*(half-28-col*31+(row%2)*13)
                    if abs(x)<90: continue
                    m.paint('Brick'); m.box((x,y,26+row*12),(29,6,10),yaw=r.uniform(-.5,.5),bevel=1.5)
        m.paint('Concrete')
        m.box((0,y,height+3),(half*2+36,48,16),bevel=3)
    m.paint('DarkMetal')
    for x in [-half-18,half+18]:
        tube(m,[(x,back-80,18),(x,back-80,height-25),(x-10,back-80,height)],[6,6,6],12)
        for z in [70,height-50]: m.box((x,back-80,z),(20,20,4),bevel=1)
        # Brackets, couplings and the worn ribs of a cast-iron downpipe.
        for z in range(48,int(height)-20,62):
            m.paint('Rust').torus((x,back-80,z),6.6,1.6,'z',20,6)
        for z in (78,height-64):
            m.paint('Steel').box((x,back-80,z),(26,22,5),bevel=1)
            for offset in (-8,8):
                m.paint('Rust').beam((x+offset,back-87,z+2),(x+offset,back-87,z+5),2.5,8,1)
    for side in (-1,1):
        for row in range(3):
            # A fractured outer course exposes the brick returns at the corner.
            for j in range(5-row):
                m.paint('Brick').box((side*(half+11),back-46-j*36+(row%2)*13,53+row*15),
                                     (11,32,13),yaw=r.uniform(-2,2),bevel=2.2)

def house():
    m=Mesh(); m.paint('Concrete'); m.box((0,0,0),(1240,1040,32))
    def wall(y,holes):
        xs=sorted(set([-600,600]+[x for a,b,c,d in holes for x in (a,b)])); zs=sorted(set([16,350]+[z for a,b,c,d in holes for z in (c,d)]))
        for a,b in zip(xs,xs[1:]):
            for c,d in zip(zs,zs[1:]):
                x,z=(a+b)/2,(c+d)/2
                if any(lo<x<hi and bottom<z<top for lo,hi,bottom,top in holes): continue
                m.paint('Plaster'); m.box((x,y,z),(b-a,26,d-c),bevel=2.7)
    wall(-500,[(-76,76,16,255),(-470,-245,125,260),(245,470,125,260)])
    wall(500,[(-470,-245,125,260),(180,420,125,260)])
    for x in [-600,600]:
        m.box((x,0,83),(26,1000,134)); m.box((x,0,310),(26,1000,80))
        for y,h in [(-370,260),(0,220),(370,260)]: m.box((x,y,200),(26,h,140))
        m.paint('Concrete')
        for y in [-245,245]: m.box((x,y,120),(42,250,14),bevel=3)
        m.paint('Plaster')
    m.box((110,250,182),(20,500,332)); m.box((110,-340,182),(20,320,332)); m.box((110,-35,310),(20,290,80))
    facade(m,600,500,350,4)
    for y in [-523,523]:
        for x in [-357,357]:
            m.paint('Concrete'); m.box((x,y,120),(245,48,14),bevel=3)
            m.paint('Wood')
            for dx in [-111,111]: m.box((x+dx,y,195),(7,12,138),bevel=1)
            # Splintered upper frame, with the window centre kept clear for shooting.
            m.box((x-25,y,265),(175,12,7),yaw=2,bevel=1)
            m.paint('Glass'); m.face([(x-105,y,258),(x-55,y,258),(x-105,y,225)])
    m.paint('DarkMetal'); m.box((-84,-519,140),(10,16,248),bevel=2); m.box((84,-519,140),(10,16,248),bevel=2)
    m.save('SM_House'); return m

def architecture(tenement_only=False):
    h=house(); m=Mesh()
    for floor in range(3):
        m.copy_from(h,(0,0,floor*380))
        if floor:
            m.paint('Concrete'); m.box((390,-620,floor*380),(2040,210,32))
            m.paint('Rust')
            for x in range(-600,1400,170): tube(m,[(x,-710,floor*380+16),(x,-710,floor*380+106)],[3,3],8)
            tube(m,[(-600,-710,floor*380+106),(1400,-710,floor*380+106)],[3,3],8)
    for floor in range(2):
        for i in range(20):
            y=-475+i*50 if floor==0 else 475-i*50; top=16+floor*380+(i+1)*19
            m.paint('Rust'); m.box((780 if floor==0 else 1000,y,top-9),(150,50,18))
        m.box((1050,535,(floor+1)*380),(700,160,32))
    m.box((1240,-40,380),(140,1160,32)); m.box((1050,-520,760),(500,160,32))
    m.paint('Concrete'); m.box((0,0,1140),(1290,1090,38))
    # A broken skyline and exposed vertical reinforcement.
    r=random.Random(97)
    for x in range(-590,600,90):
        z=r.uniform(50,120); m.paint('Brick'); m.box((x,515,1158+z/2),(85,28,z),bevel=5)
        if x%3==0:
            m.paint('Rust'); tube(m,[(x,515,1155),(x+8,519,1340)],[2,1.5],6)
    for z in [500,880]:
        m.paint('DarkMetal'); m.box((615,220,z),(65,120,65),bevel=5)
        for y in range(175,270,14): m.box((651,y,z),(4,5,50),bevel=.5)
    m.save('SM_Tenement')
    if tenement_only: return
    roof=Mesh(); roof.paint('Wood'); roof.box((0,0,370),(1290,1090,24))
    # Surviving front roof section leaves the ladder landing and relay route open.
    for y in range(-480,-119,120):
        tube(roof,[(-595,y,390),(0,y,580),(595,y,390)],[7,7,7],8)
    r=random.Random(386); roof.paint('Rust')
    for y in range(-500,-199,100):
        for side in [-1,1]:
            if r.random()<.17: continue
            # Individual pitched sheets with corrugation and missing sections.
            for x in range(0,600,40):
                p=[(side*x,y,580-x*.32),(side*(x+20),y,580-(x+20)*.32+3),(side*(x+40),y,580-(x+40)*.32)]
                for a,b in zip(p,p[1:]):
                    quad=[a,b,(b[0],y+94,b[2]),(a[0],y+94,a[2])]
                    roof.face(quad if side>0 else list(reversed(quad)))
    roof.paint('Brick'); roof.box((330,380,500),(85,100,260),bevel=4)
    roof.paint('DarkMetal'); roof.box((330,380,638),(100,115,13),bevel=3)
    for y in [-42,42]: tube(roof,[(-645,y,20),(-645,y,430)],[5,5],10)
    for z in range(40,421,28): tube(roof,[(-649,-42,z),(-649,42,z)],[3,3],8)
    roof.save('SM_Roof')
    shop=Mesh(); shop.paint('Concrete'); shop.box((0,0,0),(1440,1040,32))
    shop.paint('Plaster')
    for x in [-700,700]: shop.box((x,0,210),(28,1000,420),bevel=3)
    shop.box((0,500,210),(1400,28,420),bevel=3)
    for x in [-450,450]:
        shop.box((x,-500,65),(500,28,100)); shop.box((x,-500,370),(500,28,100))
    for x in [-700,-200,200,700]: shop.box((x,-500,210),(28,28,420))
    shop.paint('Concrete'); shop.box((0,0,430),(1480,1080,28)); facade(shop,700,500,420,91)
    shop.paint('Canvas')
    for x in range(-720,721,80):
        end=-760+random.Random(x).uniform(-30,15)
        shop.face([(x,-500,345),(x+76,-500,345),(x+76,end,285),(x,end+8,278)])
    shop.paint('Wood'); shop.box((0,-530,385),(1300,25,55),bevel=3)
    shop.paint('Rust')
    for x in [-650,650]: tube(shop,[(x,-500,290),(x,-750,280)],[4,4],8)
    shop.save('SM_Shop')
    warehouse=Mesh(); warehouse.paint('Concrete'); warehouse.box((0,0,0),(2640,1840,32))
    warehouse.paint('Brick')
    for x in [-1300,1300]: warehouse.box((x,0,340),(35,1800,680),bevel=4)
    warehouse.box((0,900,340),(2600,35,680),bevel=4)
    for x in [-810,810]: warehouse.box((x,-900,340),(980,35,680),bevel=4)
    warehouse.box((0,-900,610),(640,35,140))
    warehouse.paint('Concrete')
    for x in [-1325,1325]:
        for y in range(-870,880,340): warehouse.box((x,y,350),(50,50,700),bevel=4)
    warehouse.paint('Rust')
    for x in range(-1250,1300,250):
        tube(warehouse,[(x,-900,680),(x,0,850),(x,900,680)],[9,9,9],8)
        tube(warehouse,[(x,-850,690),(x,850,690)],[5,5],8)
    r=random.Random(64)
    for x in range(-1300,1300,100):
        for side in [-1,1]:
            if r.random()<.2: continue
            warehouse.face([(x,0,850),(x+96,0,850),(x+96,side*930,674),(x,side*930,674)] if side>0 else [(x,side*930,674),(x+96,side*930,674),(x+96,0,850),(x,0,850)])
    warehouse.save('SM_Warehouse')

def ruins():
    r=random.Random(818); m=Mesh()
    for i in range(46):
        x,y=r.uniform(-160,160),r.uniform(-105,105); z=8+30*max(0,1-math.hypot(x/190,y/150))
        m.paint('Brick' if i%3 else 'Concrete'); m.box((x,y,z),(r.uniform(18,65),r.uniform(15,46),r.uniform(12,38)),yaw=r.uniform(0,360),bevel=4)
    m.paint('Rust')
    for i in range(5):
        x,y=r.uniform(-100,100),r.uniform(-70,70); tube(m,[(x-60,y,18),(x,y,35),(x+60,y+20,25)],[1.5,1.5,1.5],6)
    m.save('SM_Rubble')
    m=Mesh()
    for row in range(17):
        for col in range(17):
            x=-280+col*34+(row%2)*17
            if abs(x)<90 and row>3 or row>11+3*math.sin(col*.7): continue
            m.paint('Brick'); m.box((x,0,row*14+7),(32,32,12),yaw=r.uniform(-1.6,1.6),bevel=2)
    m.paint('Concrete'); m.box((-290,0,100),(45,60,200),bevel=5)
    m.paint('Rust')
    for x in [-302,-281]: tube(m,[(x,0,120),(x+6,0,260),(x+21,0,275)],[2,2,1.5],6)
    m.save('SM_WallRuin')
    m=Mesh(); m.paint('Rust')
    for x in [-35,0,35]: tube(m,[(x,0,0),(x,0,120),(x+50,15,190)],[2,2,2],6)
    for z in [30,65,100]: tube(m,[(-42,0,z),(42,0,z)],[1.5,1.5],6)
    m.save('SM_Rebar')

def texture(name,color,n=1024):
    # Coherent large weathering, pits, mortar and fine grain, all tileable.
    rng=random.Random(name); height=[]; colors=[]
    noise_levels=[]
    if name=='Ground':
        for cells,weight in [(4,.45),(13,.30),(47,.16),(137,.09)]:
            grid=[[rng.uniform(-1,1) for x in range(cells)] for y in range(cells)]
            indices=[int(x/n*cells)%cells for x in range(n)]
            fractions=[(x/n*cells)%1 for x in range(n)]
            fractions=[t*t*(3-2*t) for t in fractions]
            noise_levels.append((cells,weight,grid,indices,fractions))
    for y in range(n):
        for x in range(n):
            u=x/n; v=y/n; noise=rng.random()-.5
            blot=math.sin(TAU*u*3+math.sin(TAU*v*2))*math.sin(TAU*v*5+math.cos(TAU*u*2))
            h=.5+blot*.1+noise*.08; tint=1+blot*.17+noise*.11; c=color
            if name=='Brick':
                row=int(v*22); xx=(u*6+(row%2)*.5)%1; yy=(v*22)%1
                mortar=xx<.045 or yy<.11
                if mortar: c=(96,91,80); h=.25+noise*.04; tint=1+noise*.15
                else: tint+=math.sin(int(u*6+(row%2)*.5)*43+row*73)*.13; h+=.06
            elif name=='Plaster':
                crack=abs(math.sin(TAU*u*4+math.sin(TAU*v*3)*1.7))
                if crack<.012: h-=.2; tint*=.48
                if blot<-.6: c=(110,106,91); tint*=.8
            elif name=='BirchBark':
                scar=math.sin(TAU*v*38+math.sin(TAU*u*4)*.8)
                if scar>.93 and math.sin(TAU*u*7+v*38)>.0: c=(39,40,34); h-=.1
            elif name in ['Leaf','Moss']:
                vein=abs(v-.5)<.02 or abs(math.sin((u*8+abs(v-.5)*5)*math.pi))<.1
                if vein: tint*=1.13; h+=.02
            elif name=='Ground':
                field=0
                for cells,weight,grid,indices,fractions in noise_levels:
                    ix,iy=indices[x],indices[y]; fx,fy=fractions[x],fractions[y]
                    a=grid[iy][ix]*(1-fx)+grid[iy][(ix+1)%cells]*fx
                    b=grid[(iy+1)%cells][ix]*(1-fx)+grid[(iy+1)%cells][(ix+1)%cells]*fx
                    field+=(a*(1-fy)+b*fy)*weight
                moss=max(0,min(1,(field+.08)*1.7))
                c=tuple(color[k]*(1-moss)+(46,59,34)[k]*moss for k in range(3))
                tint=1+field*.35+noise*.16; h=.5+field*.13+noise*.045
            colors.extend(max(0,min(255,int(a*tint))) for a in c); height.append(h)
    normal=bytearray(); rough=bytearray()
    for y in range(n):
        for x in range(n):
            dx=(height[y*n+(x+1)%n]-height[y*n+(x-1)%n])*.6
            dy=(height[((y+1)%n)*n+x]-height[((y-1)%n)*n+x])*.6
            normal.extend([max(0,min(255,int(128-dx*127))),max(0,min(255,int(128-dy*127))),254]); rough.extend([224]*3)
    def png(suffix,data):
        def chunk(t,b): return struct.pack('>I',len(b))+t+b+struct.pack('>I',zlib.crc32(t+b)&0xffffffff)
        scan=b''.join(b'\0'+bytes(data[y*n*3:(y+1)*n*3]) for y in range(n))
        (OUT/f'T_{name}_{suffix}.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',n,n,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(scan,6))+chunk(b'IEND',b''))
    for suffix,data in [('BaseColor',colors),('Normal',normal),('Roughness',rough)]: png(suffix,data)

def generate(textures=True):
    pine(138,'SM_Pine'); pine(238,'SM_PineB',1370)
    birch(861,'SM_Birch'); birch(73,'SM_Tree',True)
    understory(); architecture(); ruins()
    if textures:
        for name,color,size in [('Leaf',(57,86,35),1024),('Moss',(48,65,29),1024),('BirchBark',(173,169,144),1024),('Brick',(128,68,45),2048),('Plaster',(157,150,128),2048),('Ground',(68,62,43),2048)]:
            texture(name,color,size); print('Weathered texture:',name,flush=True)
    detail=OUT/'mesh-detail.json'; prior=json.loads(detail.read_text()) if detail.exists() else {}; prior.update(STATS); detail.write_text(json.dumps(prior,indent=2))
    manifest=OUT/'manifest.json'; data=json.loads(manifest.read_text()) if manifest.exists() else {}
    data.update({'units':'centimeters','axis':'Z up / X forward','license':'Original EARTHtotaled project art. No third-party assets.','meshes':sorted(p.name for p in OUT.glob('*.obj')),'textures':sorted(p.name for p in OUT.glob('*.png')),'sounds':sorted(p.name for p in OUT.glob('*.wav'))}); manifest.write_text(json.dumps(data,indent=2))
    print('Natural environment meshes:',json.dumps({k:v['triangles'] for k,v in STATS.items()}),flush=True)

if __name__=='__main__':
    import sys
    if '--ground-only' in sys.argv: texture('Ground',(68,62,43),2048)
    else: generate('--meshes-only' not in sys.argv)
