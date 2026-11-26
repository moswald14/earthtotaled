"""Model colony machinery and the Glass Brood's siege castes in centimetres."""
from meshkit import Mesh, OUT
from math import cos, sin, tau


def relay():
    m = Mesh()
    for i in range(4):
        a = i*tau/4+.78
        foot=(65*cos(a),65*sin(a),-80)
        m.paint('Steel').beam(foot,(25*cos(a),25*sin(a),-15),10)
        m.paint('Rust').box(foot,(30,31,8))
    m.paint('DarkMetal').box((0,0,-5),(79,72,112))
    for h in range(-40,42,12):
        m.paint('Steel').box((0,0,h),(88,78,4))
    m.paint('Greenline').box((42,0,6),(5,38,24))
    m.paint('Steel').beam((0,0,45),(0,0,111),5)
    for h,r in [(55,22),(68,16),(81,11)]:
        m.paint('Gold').torus((0,0,h),r,2,'z')
    m.paint('Signal').ellipsoid((0,0,110),(6,6,8))
    m.save('SM_Relay')


def turret():
    m=Mesh()
    for i in range(3):
        a=i*tau/3
        m.paint('Steel').beam((8*cos(a),8*sin(a),9),(56*cos(a),56*sin(a),-47),8)
        m.paint('Rust').box((56*cos(a),56*sin(a),-47),(28,26,6))
    m.paint('DarkMetal').ellipsoid((0,0,9),(30,30,15))
    m.paint('Gold').torus((0,0,16),24,3,'z')
    m.save('SM_SentryStand')
    head=Mesh()
    head.paint('Steel').ellipsoid((0,0,3),(31,23,18))
    for side in (-1,1):
        head.paint('DarkMetal').beam((8,side*15,3),(76,side*15,3),6,taper=.8)
        for x in (26,36,46):
            head.paint('Rust').torus((x,side*15,3),7,2)
    head.paint('Greenline').ellipsoid((25,0,15),(8,6,6))
    head.save('SM_SentryHead')


def turtle():
    m=Mesh()
    for x in (-35,34):
        for s in (-1,1):
            m.paint('Rust').beam((x,s*20,-4),(x+11,s*42,-30),7)
            m.paint('DarkMetal').box((x+14,s*42,-30),(24,14,7))
    m.paint('DarkMetal').ellipsoid((0,0,-2),(62,40,21))
    for i,x in enumerate((-41,-23,-4,15,34)):
        m.paint('Steel' if i%2 else 'Rust').ellipsoid((x,0,9),(22,38-abs(x)*.2,29-abs(x)*.22))
    m.paint('Steel').ellipsoid((58,0,0),(18,16,14))
    for s in (-1,1): m.paint('Greenline').ellipsoid((70,s*9,3),(4,4,4))
    m.save('SM_Turtle')
    drill=Mesh()
    drill.paint('DarkMetal').beam((0,0,0),(0,0,-29),9,taper=.2)
    for i in range(3):
        a=i*tau/3
        drill.paint('Gold').beam((9*cos(a),9*sin(a),0),(3*cos(a+1),3*sin(a+1),-26),4,taper=.2)
    drill.save('SM_TurtleBit')


def caste(winged):
    m=Mesh()
    m.paint('DarkMetal').ellipsoid((0,0,47),(50,24 if winged else 50,29))
    for i,x in enumerate((-45,-22,0,22,40)):
        m.paint('Rust').ellipsoid((x,0,58),(20,24 if winged else 47-i*2,22))
        m.paint('Gold').beam((x,0,74),(x-14,0,92),5,taper=.1)
    for s in (-1,1):
        m.paint('Greenline').ellipsoid((53,s*12,58),(7,6,5))
        m.paint('Gold').beam((43,s*13,46),(73,s*28,26),8,taper=.07)
        for x in (-33,-4,22):
            k=(x-12,s*(48 if winged else 62),47)
            m.paint('DarkMetal').beam((x,s*16,43),k,8)
            m.paint('Rust').beam(k,(x+2,s*75,winged and 26 or 3),6,taper=.08)
        if winged:
            root=(16,s*19,67)
            edge=[(51,s*100,83),(15,s*151,64),(-16,s*121,49),(-50,s*90,40),(-36,s*37,50)]
            for a,b in zip(edge,edge[1:]):
                m.paint('Canvas').face([root,a,b])
                m.face([b,a,root]); m.paint('Gold').beam(root,a,3,taper=.12)
    m.save('SM_BroodSkimmer' if winged else 'SM_BroodBulwark')


relay()
turret()
turtle()
caste(False)
caste(True)

