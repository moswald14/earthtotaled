"""Modular original field equipment, crop canopies, salt flats and HUD artwork."""
import math
import random
from PIL import Image, ImageDraw
from meshkit import Mesh, OUT

def pole(m, a, b, radius, mat):
    m.paint(mat).beam(a,b,radius,16,1)

m=Mesh()
m.paint('KitTeal').box((0,0,18),(76,47,36),bevel=5)
m.paint('DarkMetal').box((0,0,34),(78,48,4))
m.paint('KitTeal').box((0,0,39),(77,48,8),bevel=3)
m.paint('Bandage').box((0,0,44),(64,10,2),yaw=29)
m.paint('Stitch').box((13,8,45.8),(3,6,1.6),yaw=29)
for x in (-23,23): m.paint('Stitch').box((x,-24,32),(4,3,9))
m.save('SM_FieldKit')
im=Image.new('RGBA',(256,256)); d=ImageDraw.Draw(im)
d.rounded_rectangle((86,37,170,91),radius=17,outline='#72a59b',width=12)
d.rounded_rectangle((25,63,231,208),radius=24,fill='#184d4d',outline='#71a59b',width=7)
d.polygon([(46,156),(185,85),(204,115),(65,186)],fill='#eee6c9')
for x,y in ((148,126),(165,117)): d.line((x-3,y-6,x+3,y+5),fill='#d9933b',width=7)
im.save(OUT/'T_FieldKit.png')

for crop in ('Cinderbean','Saltroot','Mendleaf'):
    m=Mesh(); r=random.Random(crop)
    for row in (-1,0,1):
        for col in (-1,0,1):
            x,y=row*55+r.uniform(-6,6),col*56+r.uniform(-6,6)
            ht={'Cinderbean':65,'Saltroot':30,'Mendleaf':48}[crop]*r.uniform(.8,1.1)
            pole(m,(x,y,0),(x+3,y,ht),1.7,'Moss')
            for i in range(6):
                ang=i*2.4+row; dx,dy=math.cos(ang),math.sin(ang)
                z=ht*(.25+.10*i); span=(32 if crop=='Saltroot' else 22)-i*.8
                base=(x+2,y,z); tip=(x+dx*span,y+dy*span,z+8)
                center=(x+dx*span*.54,y+dy*span*.54,z+9)
                m.paint('GreenBlade' if crop!='Mendleaf' else 'Greenline')
                for side in (-1,1):
                    edge=(center[0]+side*dy*6,center[1]-side*dx*6,center[2]-4)
                    for pts in ((base,edge,center),(center,edge,tip)):
                        m.face(pts); m.face(list(reversed(pts)))
            if crop=='Cinderbean':
                for k in (-1,1):m.paint('Rust').ellipsoid((x+k*7,y,ht*.68),(3,4,9),12,6)
            elif crop=='Saltroot':m.paint('Pallor').ellipsoid((x,y,5),(9,9,12),12,6)
            else:m.paint('Bandage').ellipsoid((x+3,y,ht),(5,5,3),12,5)
    m.save('SM_'+crop)
for n in ('FieldBed','FieldTank','FieldPump','FieldClinic','RootTank','RootPump','RootClinic'):
    m=Mesh(); bio=n.startswith('Root')
    if n=='FieldBed':
        m.paint('Ground').box((0,0,9),(209,206,18))
        for i in (-1,1):
            m.paint('Wood').box((i*108,0,13),(8,222,26)); m.paint('Wood').box((0,i*106,13),(210,8,26))
        for x in range(-75,76,30):pole(m,(x,-96,16),(x,96,16),6,'Ground')
    elif n.endswith('Tank'):
        if bio:
            m.paint('Carapace').ellipsoid((0,0,66),(73,64,66))
            for z,rr in ((48,68),(77,70),(101,61)):m.paint('Greenline').torus((0,0,z),rr,4,'z')
        else:
            pole(m,(0,0,10),(0,0,132),66,'Steel')
            for z in (14,49,91,130):m.paint('Rust').torus((0,0,z),66,3,'z')
        pole(m,(0,0,17),(84,0,17),8,'DarkMetal')
    elif n.endswith('Pump'):
        m.paint('Carapace' if bio else 'Concrete').box((0,0,9),(100,85,18))
        for x in (-22,22):
            pole(m,(x,0,14),(x,0,91),14,'Greenline' if bio else 'Steel')
            m.paint('ResinLight' if bio else 'Gold').torus((x,0,59),14,3,'z')
        pole(m,(-22,0,90),(22,0,90),13,'Carapace' if bio else 'Rust')
        pole(m,(23,0,37),(67,0,37),8,'DarkMetal')
    else:
        m.paint('Carapace' if bio else 'KitTeal').box((0,0,36),(124,75,72),bevel=7)
        m.paint('Bandage').box((0,0,73),(82,12,4),yaw=25)
        for x in (-37,37):
            m.paint('ResinLight' if bio else 'Glass').ellipsoid((x,20,92),(10,10,18))
        m.paint('Stitch').box((24,-39,54),(20,3,5))
    m.save('SM_'+n)
m=Mesh()
for y in (-60,60):
    for x in (-115,100):pole(m,(x,y,-58),(x,y,40),8,'Wood')
for i in range(10):
    if i!=7:m.paint('Wood').box((-118+i*26,0,34+(i%3)*1.8),(24,146-i*1.6,9),yaw=(i%3)-1)
m.save('SM_StrandedDock')
m=Mesh()
for x in (-64,0,64):
    m.paint('Concrete').torus((x,0,51),49,11,'x')
for seg in range(18):
    a,b=seg*math.tau/18,(seg+1)*math.tau/18
    def pt(x,t):return (x,52*math.cos(t),51+52*math.sin(t))
    pts=[pt(-66,a),pt(66,a),pt(66,b),pt(-66,b)];m.paint('Rust').face(pts);m.face(pts[::-1])
m.save('SM_DryCulvert')
# Tileable Voronoi cracks and salt-flecked clay.
from PIL import ImageFilter
r=random.Random(2253); size=256
centers=[(x*32+r.randrange(8,25),y*32+r.randrange(8,25)) for y in range(8) for x in range(8)]
tile=Image.new('RGB',(size,size)); px=tile.load()
for y in range(size):
    for x in range(size):
        ds=[]
        for j in (-1,0,1):
            for i in (-1,0,1):
                a,b=centers[((y//32+j)%8)*8+(x//32+i)%8]
                dx=(x-a+128)%256-128;dy=(y-b+128)%256-128;ds.append(math.hypot(dx,dy))
        ds.sort(); v=min(1,(ds[1]-ds[0])*.9); t=r.uniform(-9,9)
        px[x,y]=tuple(int(c+a*v+t) for c,a in ((54,91),(43,74),(34,60)))
tile.save(OUT/'T_CrackedSilt.png')
