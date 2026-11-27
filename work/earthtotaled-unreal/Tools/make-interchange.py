"""Station landmarks in centimetres. The same meshes appear in the street and from the train."""
import json
import math
from meshkit import Mesh, OUT, STATS


def frame(m, width, height, material='Steel'):
    for side in (-1, 1):
        x = side * width
        m.paint('Concrete').box((x, 0, 60), (160, 250, 120), bevel=14)
        for y in (-65, 65):
            m.paint(material).beam((x, y, 80), (x * .65, y, height), 24, taper=.7)
    m.paint(material).beam((-width * .65, 0, height), (width * .65, 0, height), 60, taper=1)


def stack(m, x, y, h, material):
    m.paint(material).beam((x, y, 0), (x, y, h), 152, 40, .68)
    for n in range(1, 9):
        z = h * n / 9
        m.paint('DarkMetal').torus((x, y, z), 152 * (1 - .32 * n / 9), 9, 'z')
    m.paint('Rust').torus((x, y, h), 108, 17, 'z')


def landmark(kind):
    m = Mesh()
    if kind == 'Beacon':
        frame(m, 290, 1470, 'Greenline')
        for z in (1110, 1290, 1500):
            m.paint('CircuitLight').torus((0, 0, z), 176, 23, 'z')
        m.paint('Greenline').beam((0,0,1480),(0,0,2150),22)
    elif kind == 'QuayCrane':
        frame(m, 330, 2150)
        for z in range(120, 2080, 270):
            m.paint('Rust').beam((-210,0,z),(210,0,z+265),21)
        m.paint('Rust').beam((-710,0,2360),(1020,0,2720),60)
        for x in (-630, 320, 850):
            m.paint('Steel').beam((0,0,2920),(x,0,2530+x*.18),13)
        m.paint('DarkMetal').beam((0,0,2010),(0,0,2920),63)
        for x in (740, 820):
            m.paint('Steel').beam((x,0,2670),(x,0,1180),7)
        m.paint('Rust').box((780,0,1150),(215,100,85))
    elif kind in ('Glasshouse','CanopyWalk'):
        for x in (-540, -270, 0, 270, 540):
            for side in (-1, 1):
                y = side*350
                m.paint('Greenline' if kind=='Glasshouse' else 'Wood').beam((x,y,0),(x,y,700),16)
                m.beam((x,y,700),(x,0,1100),20)
        if kind=='Glasshouse':
            for side in (-1,1):
                m.paint('Glass').face([(-540,350*side,700),(540,350*side,700),(540,0,1100),(-540,0,1100)])
            for side in (-1,1):
                m.paint('Brick').box((0,side*350,115),(1130,65,230))
        else:
            for x in range(-570,571,45):
                m.paint('Wood').box((x,0,680),(42,450,24))
            for y in (-230,230):
                m.paint('Rust').beam((-585,y,790),(585,y,790),12,taper=1)
    elif kind=='Kilns':
        for x,h in [(-420,2000),(0,2700),(420,1550)]:
            stack(m,x,0,h,'Brick')
            m.paint('Brick').ellipsoid((x,0,210),(260,260,300))
    elif kind=='Aerial':
        frame(m,290,1770)
        for z in range(370,2700,320):
            m.paint('Steel').beam((0,0,z-340),(0,0,z+20),30)
            m.paint('Signal').beam((-520+z*.12,0,z),(520-z*.12,0,z),16,taper=1)
        m.paint('CircuitLight').ellipsoid((0,0,2680),(35,35,68))
    elif kind in ('Vats','FilterBeds'):
        for i,(x,y) in enumerate(((-420,0),(50,180),(480,-60))):
            h = (1430+190*i) if kind=='Vats' else (660+140*i)
            mat = 'Rust' if kind=='Vats' else 'Greenline'
            m.paint(mat).beam((x,y,100),(x,y,h),190,40,1)
            m.ellipsoid((x,y,h),(190,190,100))
            for z in range(140,h,285):
                m.paint('Steel').torus((x,y,z),194,11,'z')
            m.paint('Steel').beam((x,y,h+75),(x,y,h+180),20)
        m.paint('Steel').beam((-420,0,360),(480,0,360),45,taper=1)
    elif kind=='SawWheel':
        frame(m,410,660,'Wood')
        m.paint('Steel').torus((0,0,775),590,36,'y',64)
        for i in range(24):
            a=i*math.tau/24
            x,z=math.cos(a),math.sin(a)
            m.paint('Rust').face([(x*585,0,775+z*585),(x*710,0,775+z*710),(math.cos(a+.17)*585,0,775+math.sin(a+.17)*585)])
            if i%3==0: m.paint('Wood').beam((0,0,775),(x*570,0,775+z*570),28)
    elif kind=='Arch':
        for y in (-170,170):
            for i in range(16):
                a,b=i*math.pi/16,(i+1)*math.pi/16
                m.paint('Concrete').beam((800*math.cos(a),y,480+1120*math.sin(a)),(800*math.cos(b),y,480+1120*math.sin(b)),63,taper=1)
            for x in (-800,800):
                m.paint('Brick').box((x,y,230),(195,210,480))
        for x in range(-750,751,150):
            m.paint('Steel').box((x,0,520),(144,455,39))
    elif kind=='Turbine':
        m.paint('Concrete').beam((0,0,0),(0,0,1720),104,36,.4)
        for n in range(3):
            a=n*math.tau/3+.34
            p=lambda r,t:(math.cos(a)*r-math.sin(a)*t,-80,1710+math.sin(a)*r+math.cos(a)*t)
            m.paint('Plaster').face([p(55,-35),p(320,-90),p(950,40),p(290,75)])
        m.paint('Rust').ellipsoid((0,-85,1710),(100,90,100))
    elif kind=='Dish':
        frame(m,280,1120)
        for i in range(24):
            a,b=i*math.tau/24,(i+1)*math.tau/24
            p=lambda r,t:(r*math.cos(t),-.0006*r*r,1190+r*math.sin(t))
            m.paint('Steel' if i%3 else 'Greenline').face([p(70,a),p(690,a),p(690,b),p(70,b)])
            if i%6==0: m.paint('Rust').beam(p(690,a),(0,-605,1190),12)
        m.paint('CircuitLight').ellipsoid((0,-605,1190),(45,60,45))
    elif kind=='Headframe':
        frame(m,440,2100,'Rust')
        for y in (-150,150):
            m.paint('Steel').torus((0,y,2150),250,26,'y')
            m.beam((-560,y,0),(0,y,2180),30)
            m.beam((0,y,2320),(800,y,530),12)
        m.paint('DarkMetal').box((550,0,360),(500,420,720))
    elif kind=='Clock':
        m.paint('Brick').box((0,0,875),(420,400,1750),bevel=19)
        for y in (-211,211):
            m.paint('Canvas').beam((0,y,1440),(0,y*1.04,1440),176,48,1)
            m.paint('Steel').torus((0,y*1.07,1440),175,16,'y')
            m.paint('DarkMetal').beam((0,y*1.10,1440),(0,y*1.10,1570),12)
            m.beam((0,y*1.10,1440),(98,y*1.10,1370),11)
        m.paint('Greenline').beam((0,0,1710),(0,0,2090),292,16,.04)
    elif kind=='Orrery':
        frame(m,260,760)
        for axis,r in [('z',420),('x',470),('y',525)]:
            m.paint('Gold').torus((0,0,1000),r,18,axis,64)
        m.paint('CircuitLight').ellipsoid((0,0,1000),(145,145,145))
    elif kind=='SaltSilos':
        for x,h in [(-390,1220),(0,1720),(390,1180)]:
            m.paint('Plaster').beam((x,0,150),(x,0,h),188,40,1)
            m.paint('Steel').beam((x,0,h),(x,0,h+240),200,32,.06)
            for z in range(320,h,330):
                m.paint('Rust').torus((x,0,z),192,8,'z')
    m.save('SM_Quarter'+kind)


def generate_landmarks():
    for name in ('Beacon','QuayCrane','Glasshouse','Kilns','Aerial','Vats','SawWheel','Arch','Turbine','Dish','Headframe','Clock','FilterBeds','CanopyWalk','Orrery','SaltSilos'):
        landmark(name)


if __name__=='__main__':
    generate_landmarks()
    for file,key in [('mesh-detail.json',None),('manifest.json','meshes')]:
        target=OUT/file
        data=json.loads(target.read_text())
        if key: data[key]=sorted(p.name for p in OUT.glob('*.obj'))
        else: data.update(STATS)
        target.write_text(json.dumps(data,indent=2))
