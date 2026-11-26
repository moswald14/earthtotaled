"""Original architecture for the highway colonies. Dimensions are in centimetres."""
from math import cos, sin, pi, tau
import json
from meshkit import Mesh, OUT, STATS


def blade(mesh, angle, height):
    """A diamond-section buttress splitting away from the suspended core."""
    def point(radius, side, z):
        return (radius*cos(angle)-side*sin(angle), radius*sin(angle)+side*cos(angle), z)
    levels = [(-300,1190,300,330),(250,1180,265,305),(1500,1070,200,260),
              (2850,1010,270,365),(height-590,1220,155,215),(height,1420,26,54)]
    rings = []
    for z, radius, depth, width in levels:
        rings.append([point(radius+depth,0,z),point(radius,width,z),point(radius-depth,0,z),point(radius,-width,z)])
    for i,(low,high) in enumerate(zip(rings,rings[1:])):
        for j in range(4):
            k=(j+1)%4
            mesh.paint('Steel' if j in (0,3) else 'DarkMetal').face([low[j],low[k],high[k],high[j]])
        if i>=1:
            mesh.paint('CircuitLight').beam(low[2],high[2],12,taper=.85)
    mesh.paint('Steel').face(rings[-1])


def parallax():
    m=Mesh()
    for i in range(6):
        blade(m,i*tau/6,4900+380*(i%2))
    # A floating octahedral processor, intersected by tilted orbital bands.
    eq=[(565*cos(a*pi/4),565*sin(a*pi/4),3150) for a in range(8)]
    for i,p in enumerate(eq):
        q=eq[(i+1)%8]
        m.paint('Glass' if i%2 else 'Steel').face([p,q,(0,0,4050)])
        m.paint('DarkMetal').face([q,p,(0,0,2370)])
        m.paint('CircuitLight').beam(p,(0,0,4050),12,taper=.38)
    for i,tilt in enumerate((.44,-.57)):
        ring=Mesh()
        ring.paint('CircuitLight' if i==0 else 'Gold').torus((0,0,0),930+i*95,21+i*9,'z',72,8)
        def rotate(p): return (p[0],p[1]*cos(tilt)-p[2]*sin(tilt),p[1]*sin(tilt)+p[2]*cos(tilt))
        ring.vertices=[rotate(p) for p in ring.vertices]
        ring.normals=[rotate(n) for n in ring.normals]
        m.copy_from(ring,(0,0,3150))
    for h in (2200,2340,3880):
        m.paint('Steel').torus((0,0,h),1010,21,'z',60,8)
    m.save('SM_ParallaxSpire')

    m=Mesh()
    for side in (-1,1):
        m.paint('DarkMetal').box((side*490,0,520),(280,940,1500),bevel=48)
        for z in range(260,1190,155):
            m.paint('Steel').box((side*490,0,z),(318,985,30),bevel=9)
        m.paint('CircuitLight').box((side*490,-476,630),(27,13,780),bevel=3)
    m.paint('DarkMetal').box((0,0,1390),(1420,1060,260),bevel=45)
    for x in range(-555,556,185):
        m.paint('Steel').box((x,0,1590),(55,965,230-abs(x)*.15),bevel=14)
    m.paint('CircuitLight').box((0,-535,1380),(1150,12,21))
    m.save('SM_ComputeHall')


def bellows():
    m=Mesh()
    # A hollow fluted shell has four ground-level mouths and a breathing oculus.
    n=48
    levels=[(-330,1770),(260,1710),(670,1520),(1110,1390),(1590,1020),(2060,680),(2550,480),(2830,590)]
    def shell(j,i,inner=False):
        z,r=levels[j]; a=tau*i/n
        r=(r-(105 if inner else 0))*(1+.08*cos(8*a+.16*j)+.032*sin(3*a))
        return (r*cos(a)+j*j*4,r*sin(a),z+(.032*z*cos(4*a+.1*j) if j>1 else 0))
    for j in range(len(levels)-1):
        for i in range(n):
            k=(i+1)%n
            if j<=2 and min(i%12,11-i%12)<(3 if j<=1 else 2): continue
            for inside in (False,True):
                points=[shell(j,i,inside),shell(j,k,inside),shell(j+1,k,inside),shell(j+1,i,inside)]
                m.paint('Bark' if inside else 'Rust' if (i+2*j)%12 in (2,3) else 'Hide').face(points[::-1] if inside else points)
            if j==len(levels)-2:
                m.paint('Gold').face([shell(j+1,i),shell(j+1,k),shell(j+1,k,True),shell(j+1,i,True)])
    for i in range(8):
        a=(i+.5)*tau/8
        joints=[((r+85)*cos(a)+j*j*4,(r+85)*sin(a),z) for j,(z,r) in enumerate(levels)]
        for j,(a0,b) in enumerate(zip(joints,joints[1:])):
            m.paint('Rust' if j%3 else 'Gold').beam(a0,b,68-j*5,16,.84)
    for i in range(10):
        a=i*tau/10
        m.paint('ResinLight').ellipsoid((780*cos(a)+125,780*sin(a),1800+90*sin(a*3)),(93,85,135),12,7)
    m.save('SM_BellowsHive')

    m=Mesh()
    for i in range(5):
        a=i*tau/5
        m.paint('Rust').beam((440*cos(a),440*sin(a),-170),(250*cos(a),250*sin(a),650),95,16,.6)
        m.paint('Hide').ellipsoid((255*cos(a),255*sin(a),395),(235,220,480),18,9)
        m.paint('Gold').ellipsoid((403*cos(a),403*sin(a),550),(82,82,155),12,7)
    m.paint('Hide').ellipsoid((0,0,845),(322,312,492),24,12)
    m.paint('ResinLight').ellipsoid((0,0,1170),(138,135,230),18,9)
    m.save('SM_HiveNursery')


def street_furniture():
    m=Mesh()
    m.paint('Steel').beam((0,0,-60),(0,0,535),14,16,.63)
    m.paint('DarkMetal').beam((0,0,527),(0,340,535),11,16,.8)
    m.paint('DarkMetal').box((0,295,496),(123,50,52),bevel=7)
    for i in (-1,0,1):
        m.paint('Signal' if i==-1 else 'Rubber').ellipsoid((i*34,265,495),(13,7,13),12,6)
    m.paint('Steel').box((0,0,105),(43,40,65),bevel=4)
    m.save('SM_CrossingSignal')


if __name__=='__main__':
    parallax()
    bellows()
    street_furniture()
    details=OUT/'mesh-detail.json'
    stats=json.loads(details.read_text())
    stats.update(STATS)
    details.write_text(json.dumps(stats,indent=2))
    path=OUT/'manifest.json'
    manifest=json.loads(path.read_text())
    manifest['meshes']=sorted(p.name for p in OUT.glob('*.obj'))
    path.write_text(json.dumps(manifest,indent=2))
