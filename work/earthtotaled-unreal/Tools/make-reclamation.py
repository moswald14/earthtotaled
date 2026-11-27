"""Build original Reclamation meshes in centimetres for Unreal's material slots."""
from math import sin, cos, tau
from random import Random
from meshkit import Mesh


def limb(m, joints, radii, coat, rings=10):
    for a,b,r,s in zip(joints,joints[1:],radii,radii[1:]):
        m.paint(coat).beam(a,b,r,rings,s/r)


def shell(kind, length, wings=False):
    m=Mesh()
    scale=length/110
    for i,(x,z,w,h) in enumerate([(-31,54,24,21),(-7,64,29,24),(18,69,27,23),(40,60,18,16)]):
        m.paint('Carapace').ellipsoid((x,0,z),(27,w,h),24,12)
        for side in [-1,1]:
            m.paint('Rust').beam((x-19,side*w*.5,z+10),(x-24,side*(w+7),z+1),3.4,9,.15)
            m.paint('ResinLight').ellipsoid((x+4,side*(w*.96),z+4),(4.6,1.5,2.4),10,5)
    for side in [-1,1]:
        for i,(x,reach) in enumerate([(-28,-27),(-6,-14),(17,23)]):
            points=[(x,side*16,56),(x+reach*.24,side*39,69-i*4),(x+reach,side*(51+i*4),31),(x+reach+13,side*(60+i*3),2)]
            limb(m,points,[7,5.8,3.3,.6],'Carapace')
            m.paint('Hide').ellipsoid(points[1],(7,7,7),12,6)
        m.paint('DarkMetal').ellipsoid((49,side*13,63),(7,4.5,6),16,9)
        for i in range(5):
            a=i*tau/5
            m.paint('ResinLight').ellipsoid((50+3*cos(a),side*16.2,63+2.6*sin(a)),(1,1,1.2),8,4)
        limb(m,[(47,side*9,55),(58,side*22,48),(68,side*17,53)],[4.8,2.7,.35],'Gold')
        limb(m,[(40,side*8,72),(55,side*17,92),(71,side*30,96)],[1.9,1.1,.2],'DarkMetal',7)
        if wings:
            for x,ex,ey in [(0,-88,92),(16,-49,104)]:
                a=(x,side*14,76); b=(ex,side*ey,83); c=(ex-16,side*60,78); d=(x-24,side*23,77)
                m.paint('Glass').face([a,b,c,d]); m.face([d,c,b,a])
                for p in [b,c,d]:m.paint('Carapace').beam(a,p,1.6,7,.22)
    if kind=='Spitter':
        for x in (-43,-30,-17):m.paint('ResinLight').ellipsoid((x,0,78),(11,16,12),16,8)
    if kind=='Bulwark':
        for x in (-30,-8,14):
            m.paint('Carapace').ellipsoid((x,0,83),(24,35,16),24,12)
            for side in [-1,1]:m.paint('Gold').beam((x,side*29,82),(x-14,side*39,99),5,10,.09)
    m.vertices=[(x*scale,y*scale,z*scale) for x,y,z in m.vertices]; m.save('SM_Molt'+kind)


def machines():
    m=Mesh()
    m.paint('AlloyHull').ellipsoid((0,0,133),(30,36,29),32,16)
    m.paint('DarkMetal').ellipsoid((-1,0,107),(18,21,16),20,10)
    for z in (108,115,122): m.paint('Rubber').torus((0,0,z),21,2,'z',24,6)
    for y in (-1,1):
        limb(m,[(0,y*17,108),(-13,y*22,73),(13,y*23,34),(-3,y*24,9)],[10,7,5,3],'AlloyHull')
        m.paint('Rubber').ellipsoid((-13,y*22,73),(9,10,9),16,8)
        m.paint('AlloyHull').box((9,y*24,6),(35,17,10),bevel=3)
        limb(m,[(-5,y*32,145),(-14,y*50,117),(15,y*51,103)],[11,7,5],'DarkMetal')
        m.paint('AlloyHull').ellipsoid((-4,y*36,147),(16,15,13),22,12)
        m.paint('Steel').beam((8,y*50,104),(44,y*50,104),6.5,20,.8)
        m.paint('CircuitLight').ellipsoid((44,y*50,104),(1.5,3,3),12,7)
        for x in range(-21,10,7):m.paint('DarkMetal').box((x,y*33,138),(3,5,18),bevel=.8)
        for z in range(128,149,7):m.paint('Steel').beam((-24,y*18,z),(12,y*23,z+6),1.4,7,.8)
        limb(m,[(-12,y*37,144),(-24,y*45,132),(-14,y*48,121)],[2,2,1.5],'Rubber')
    m.paint('AlloyHull').ellipsoid((6,0,167),(18,20,14),28,14)
    m.paint('DarkMetal').box((21,0,168),(5,33,10),bevel=2)
    for y in (-10,0,10):m.paint('CircuitLight').ellipsoid((24,y,168),(2,3.1,3.1),12,7)
    m.paint('Steel').beam((-9,-13,174),(-18,-16,208),1.8,9,.38)
    m.save('SM_SupertechWarden')
    m=Mesh();m.paint('AlloyHull').ellipsoid((0,0,0),(25,19,12),24,12)
    for x in (-17,14):
        for y in (-22,22):
            m.paint('AlloyHull').torus((x,y,0),13,2.8,'z',24,6)
            m.paint('DarkMetal').box((x,y,0),(22,3,2),yaw=x+y,bevel=.5)
    m.paint('DarkMetal').ellipsoid((17,0,-5),(12,10,9),18,10)
    m.paint('CircuitLight').ellipsoid((27,0,-5),(2.2,5,4),14,8);m.save('SM_SupertechScout')


def vegetation():
    rng=Random(80172)
    m=Mesh()
    for i in range(78):
        a=rng.random()*tau; radius=rng.random()**.55*104
        x,y=radius*cos(a),radius*sin(a); bend=rng.uniform(8,24); h=rng.uniform(14,51)
        a+=rng.uniform(-1,1); dx,dy=cos(a),sin(a); width=rng.uniform(1,2.4)
        m.paint('DryBlade' if i%6==0 else 'GreenBlade')
        def row(t):
            cx=x+dx*bend*t*t; cy=y+dy*bend*t*t; z=t*h; w=width*(1-t*.78)
            return [(cx-dy*w,cy+dx*w,z),(cx+dx*.8,cy+dy*.8,z+1),(cx+dy*w,cy-dx*w,z)]
        low=row(0);mid=row(.54);tip=(x+dx*bend,y+dy*bend,h)
        for j in (0,1):
            m.face([low[j],low[j+1],mid[j+1],mid[j]]);m.face([mid[j],mid[j+1],tip])
    m.save('SM_BladePatch')
    m=Mesh()
    for stem in range(5):
        x,y=rng.uniform(-33,33),rng.uniform(-33,33);h=rng.uniform(40,104)
        limb(m,[(x,y,-2),(x+3,y+2,h*.5),(x+12,y+4,h)],[1.4,1,.4],'GreenBlade',5)
        for i in range(8):
            z=h*(.18+i*.072);a=i*2.40+stem;dx,dy=cos(a),sin(a);l=17-i*1.1
            b=(x+5,y,z);tip=(x+5+dx*l,y+dy*l,z+8)
            m.paint('GreenBlade').face([b,(x+5+dx*l*.55-dy*4,y+dy*l*.55+dx*4,z+1),tip,(x+5+dx*l*.55+dy*4,y+dy*l*.55-dx*4,z+2)])
        for i in range(5):
            a=i*tau/5;p=(x+12+8*cos(a),y+4+8*sin(a),h+3)
            m.paint('DryBlade').beam((x+12,y+4,h-6),p,.6,5,.5);m.ellipsoid(p,(1.8,1.8,3),6,4)
    m.save('SM_RoadWeed')


def tower():
    m=Mesh()
    for a in [i*tau/4+tau/8 for i in range(4)]:
        b=a+tau/4
        q=lambda t,r,z:(r*cos(t),r*sin(t),z)
        m.paint('Rust').beam(q(a,292,-215),q(a,215,1185),14,12,.85)
        for z in (220,630):
            m.paint('Steel').beam(q(a,265,z),q(b,235,z+395),5,8)
            m.beam(q(b,265,z),q(a,235,z+395),5,8)
    m.paint('Rust').beam((0,0,1115),(0,0,1270),75,52,4.4)
    m.paint('AlloyHull').beam((0,0,1270),(0,0,1590),330,64,1)
    for z in [1280,1420,1575]:
        m.paint('Rust').torus((0,0,z),330,6,'z',64,6)
    m.paint('Steel').beam((0,0,1590),(0,0,1707),339,64,.075)
    for y in [-24,24]:m.paint('Steel').beam((322,y,0),(322,y,1604),3.4,8)
    for z in range(27,1598,34):m.paint('Steel').beam((322,-24,z),(322,24,z),2.5,8)
    m.paint('Greenline').beam((0,0,1120),(0,0,55),14,12,1)
    m.paint('Steel').beam((0,0,60),(142,0,60),13,12,.85)
    m.save('SM_HillCistern')


def walker():
    m=Mesh()
    rng=Random(219)
    def jacket(rings):
        loops=[]
        for i,(x,z,rx,ry) in enumerate(rings):
            loop=[]
            for j in range(36):
                a=j*tau/36
                crease=1+.037*sin(7*a)+.024*cos(11*a+i*.6)
                hem=rng.uniform(-7,2) if i==0 else 0
                loop.append((x+rx*cos(a)*crease,ry*sin(a)*crease,z+hem))
            loops.append(loop)
        m.paint('RotCloth')
        for i in range(len(loops)-1):
            for j in range(36):
                k=(j+1)%36
                m.face([loops[i][j],loops[i][k],loops[i+1][k],loops[i+1][j]],
                       [(j/36,i/5),((j+1)/36,i/5),((j+1)/36,(i+1)/5),(j/36,(i+1)/5)])
    jacket([(-2,84,13,18),(-2,98,14,18),(0,112,14,20),(1,128,15,23),(5,139,12,22),(7,145,7,11)])
    m.paint('Rubber').ellipsoid((-1,0,93),(14.1,19,3.5),30,6)
    m.paint('Steel').box((13,0,93),(2,6,4),bevel=.6)
    for z in (99,106,113,120,127,134):
        x=13+(z-100)*.09
        m.paint('DarkMetal').ellipsoid((x+1,1,z),(1.6,1.2,1),8,5)
    m.paint('Canvas').box((-4,-19,95),(12,5,11),bevel=1)
    for s in [-1,1]:
        knee=(7+s*5,s*11,48+s*2);ankle=(-4-s*5,s*13,13)
        limb(m,[(-2,s*10,91),knee,ankle],[10.8,7.8,6.1],'RotCloth',18)
        m.paint('Pallor').ellipsoid((knee[0]+6.4,knee[1],knee[2]),(2.2,4,3.4),12,8)
        m.paint('Rubber').box((ankle[0]+5,s*13,4),(26,13,6),bevel=2)
        m.paint('DarkMetal').ellipsoid((ankle[0]+7,s*13,9),(13.3,6.7,6.2),20,9)
        for j in range(4):m.paint('Canvas').beam((ankle[0]+j*2,s*13-3,14),(ankle[0]+j*2+2,s*13+3,15),.6,6,.7)
        shoulder=(5,s*22,136);elbow=(11+s*7,s*32,111-s*4);hand=(34+s*8,s*24,100+s*9)
        limb(m,[shoulder,elbow],[8.8,6.0],'RotCloth',16)
        m.paint('RotCloth').ellipsoid(shoulder,(9,7.8,8),20,9)
        limb(m,[elbow,hand],[5.8,3.2],'Pallor',14)
        m.paint('Pallor').ellipsoid(hand,(6.5,4.8,3.6),16,8)
        for j in range(4):
            x,y,z=hand;y+=s*(j*1.7-2.4)
            limb(m,[(x+3,y,z-1),(x+8-j*.3,y+s*.6,z-2.3),(x+9-j*.4,y+s*1.4,z-5.7)],[1.1,.9,.45],'Pallor',6)
        m.paint('RotCloth').face([(15,s*3,128),(12,s*17,140),(7,s*8,146)])
    m.paint('Pallor').beam((7,0,139),(14,0,151),5.8,18,.88)
    m.paint('Pallor').ellipsoid((13,0,158),(10.9,8.8,12.6),32,16)
    m.paint('Pallor').ellipsoid((17.7,.3,151.3),(7.2,7,5.8),24,12)
    for s in (-1,1):
        m.paint('Pallor').ellipsoid((12,s*8.6,158),(2.5,1.9,4.1),14,8)
        m.paint('DarkMetal').ellipsoid((22,s*3.8,160.1+s*.5),(1.55,3.1,1.65),16,8)
        m.paint('Plaster').ellipsoid((23.2,s*4,160+s*.5),(.8,1.55,.82),14,7)
        m.paint('Pallor').beam((22.1,s*.8,161.3),(21.5,s*7.1,162.1+s*.7),1.5,10,.64)
        m.paint('Pallor').ellipsoid((21.0,s*6,156.7),(2.0,2.6,2.1),16,8)
    limb(m,[(22,0,161),(24.8,.2,155.9),(25.4,0,155.2)],[1.25,1.8,.65],'Pallor',12)
    m.paint('DarkMetal').ellipsoid((24,.4,150.2),(1.1,4,1.75),18,8)
    for j in range(5):m.paint('Plaster').box((24.7,-2.2+j*1.04,151),(.65,.8,1),bevel=.15)
    for i in range(50):
        a=rng.uniform(0,tau); t=rng.uniform(.1,1.12) if cos(a)<.25 else rng.uniform(.1,.60)
        p=(13+10.9*cos(a)*sin(t),8.8*sin(a)*sin(t),158+12.6*cos(t))
        m.paint('Bark').beam(p,(p[0]-rng.uniform(.5,3),p[1]+rng.uniform(-.8,.8),p[2]+rng.uniform(1,3)),.65,6,.16)
    m.save('SM_HollowWalker')


if __name__=='__main__':
    vegetation(); tower(); walker(); machines()
    for kind,length in [('Runner',100),('Spitter',113),('Bulwark',158),('Skimmer',91)]: shell(kind,length,kind=='Skimmer')
