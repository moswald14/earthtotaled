"""Author the bear, forest litter and Kit's field clues in centimetres."""
import array
import math
import random
import struct
import wave
import zlib

from meshkit import Mesh, OUT, STATS, unit


def bear():
    body = Mesh().paint('BearFur')
    # Cross sections run from the rump over the high shoulder to the brow.
    sections = [(-102,79,2,3),(-98,84,20,26),(-85,85,40,43),
                (-57,85,49,49),(-24,85,50,49),(6,91,49,54),(29,99,46,59),
                (47,104,40,53),(62,102,33,36),(77,105,30,29),
                (90,106,26,23),(102,101,20,16),(109,99,10,8)]
    def ring(i,j):
        x,z,w,h = sections[i]
        a = j*math.tau/40
        delta = math.sin(a*5+i*.75)*.018
        return (x,w*math.cos(a)*(1+delta),z+h*math.sin(a)*(1+delta))
    for i in range(len(sections)-1):
        for j in range(40):
            corners = [(i,j),(i,j+1),(i+1,j+1),(i+1,j)]
            normals = []
            for k,n in corners:
                a = n*math.tau/40
                left,right = sections[max(0,k-1)],sections[min(len(sections)-1,k+1)]
                slope = ((right[2]-left[2])*math.cos(a)**2+(right[3]-left[3])*math.sin(a)**2
                         +(right[1]-left[1])*math.sin(a))/(right[0]-left[0])
                normals.append(unit((-slope,math.cos(a),math.sin(a))))
            body.face([ring(k,n) for k,n in corners],[(n/40,k/9) for k,n in corners],normals)
    for side in (-1,1):
        body.ellipsoid((74,side*22,128),(8,9,11),20,10)
        body.paint('Rubber').ellipsoid((80,side*23,130),(3,6,6),16,8)
        body.ellipsoid((95,side*21,113),(3.4,3,3),20,10)
        body.paint('BearFur')
    body.ellipsoid((-96,0,89),(14,10,11),16,10)
    body.paint('Hide').ellipsoid((107,0,97),(23,16,12),32,14)
    body.paint('Rubber').ellipsoid((126,0,102),(6,12,6),24,12)
    body.paint('Hide').ellipsoid((109,0,88),(17,13,5),24,10)
    body.save('SM_WoodlandBear')
    for front in (True,False):
        leg = Mesh().paint('BearFur')
        leg.ellipsoid((-3 if front else 1,0,-10),(17 if front else 23,15,31))
        leg.beam((-4,0,-18),(-9 if front else 8,0,-47),14,20,.74)
        leg.ellipsoid((-6 if front else 7,0,-49),(11,10,18),24,12)
        leg.ellipsoid((2 if front else 12,0,-62),(18,12,8),24,12)
        leg.paint('Hide')
        for toe in range(5):
            y=(toe-2)*4
            x=(14 if front else 24)-abs(toe-2)*1.5
            leg.ellipsoid((x,y,-64),(6.5,3.2,4),12,6)
            leg.paint('Pallor').beam((x+3,y,-64),(x+8,y,-67),1.4,16,.12)
            leg.paint('Hide')
        leg.save('SM_BearForeleg' if front else 'SM_BearHindleg')


def litter():
    rng=random.Random(71397)
    fall=Mesh().paint('Bark')
    for a,b,r,t in [((-71,-7,4),(-26,2,10),7,.88),((-26,2,10),(20,9,8),6.3,.72),
                      ((20,9,8),(68,22,3),4.5,.35),((-24,2,11),(-2,-25,19),4.2,.62),
                      ((-2,-25,19),(15,-38,24),2.8,.25),((18,9,9),(40,-5,21),3.1,.23)]:
        fall.beam(a,b,r,16,t)
    fall.paint('Wood').beam((-72,-7,4),(-65,-6,6),5,16,.95)
    fall.paint('Moss')
    for i in range(8):
        fall.ellipsoid((-56+i*9,-2+i,11+rng.random()*4),(8,4,2.4),12,5)
    fall.save('SM_CedarFall')
    cups=Mesh()
    for x,y,h,r in [(-14,4,13,9),(0,-4,23,14),(12,11,18,11),(18,-9,8,5),(-5,16,9,6)]:
        cups.paint('Wood').beam((x-2,y,0),(x,y,h),r*.23,16,.64)
        cups.paint('Pallor').ellipsoid((x,y,h),(r,r*.9,r*.19),18,6)
        cups.paint('AmberCap')
        for j in range(10):
            for k in range(28):
                vertices=[];normals=[];uv=[]
                for row,col in [(j,k),(j+1,k),(j+1,k+1),(j,k+1)]:
                    angle=col*math.tau/28; arc=row*math.pi*.54/10; broad=math.sin(arc)
                    rim=1+.047*math.sin(3*angle+x)+.023*math.cos(7*angle+y)
                    cx=math.cos(angle)*broad;cy=math.sin(angle)*broad
                    vertices.append((x+r*1.03*cx*rim,y+r*.94*cy*rim,h+r*(.13+.48*math.cos(arc)+.03*cy)))
                    normals.append(unit((cx,cy,math.cos(arc)/.48)));uv.append((.5+.46*cx,.5+.46*cy))
                corners=[0,1,2] if j==0 else [0,1,2,3]
                cups.face([vertices[i] for i in corners],[uv[i] for i in corners],[normals[i] for i in corners])
        cups.paint('Pallor')
        for k in range(24):
            a=k*math.tau/24;c,s=math.cos(a),math.sin(a)
            cups.face([(x+c*r*.2,y+s*r*.2,h-r*.08),
                       (x+c*r*.96,y+s*r*.88,h+r*.06),
                       (x+c*r*.62-1.1*s,y+s*r*.58+1.1*c,h-r*.17)])
    cups.save('SM_AmberCaps')
    fern=Mesh()
    for stem in range(8):
        a=stem*math.tau/8+rng.uniform(-.2,.2)
        ux,uy=math.cos(a),math.sin(a)
        length=rng.uniform(35,66);height=rng.uniform(24,51)
        def at(t):
            return (ux*length*t,uy*length*t,height*math.sin(t*2))
        fern.paint('GreenBlade')
        for j in range(5):
            fern.beam(at(j/5),at((j+1)/5),.85,16,.7)
        for j in range(1,10):
            t=j/11; x,y,z=at(t)
            for sign in (-1,1):
                w=(1-t)**.63*14; tip=(x+ux*6-sign*uy*w,y+uy*6+sign*ux*w,z+2)
                fern.paint('DryBlade' if stem==3 else 'GreenBlade')
                fern.face([(x-ux*2,y-uy*2,z),tip,(x+ux*3,y+uy*3,z+1.8)])
    fern.save('SM_BentBracken')
    cones=Mesh()
    for x,y,h in [(-14,-6,13),(13,4,10),(2,18,8)]:
        cones.paint('Bark').ellipsoid((x,y,h*.53),(h*.34,h*.34,h*.53),14,8)
        cones.paint('Wood')
        for i in range(26):
            t=(i+.5)/26; a=i*2.39996; r=h*.38*math.sin(t*math.pi)**.6; c,s=math.cos(a),math.sin(a)
            z=h*(t+.08); base=(x+r*c,y+r*s,z); tip=(x+(r+2.8)*c,y+(r+2.8)*s,z+2.9)
            cones.face([(base[0]-s*2,base[1]+c*2,z),tip,(base[0]+s*2,base[1]-c*2,z), (x+r*.8*c,y+r*.8*s,z-2)])
    cones.save('SM_ConeBed')


def clues():
    wrapper=Mesh()
    for i in range(4):
        wrapper.paint('Gold' if i%2==0 else 'Rust')
        wrapper.box((i*.32,i*.25,.6+i*.92),(11-i*.45,12-i*.62,.85),i*3,bevel=.25)
    wrapper.paint('Steel').face([(-5,-4,3.6),(5,-3,3.7),(4,4,4.3)])
    wrapper.save('SM_KitFold')
    for paw in (False,True):
        marks=Mesh()
        for step in range(3 if paw else 4):
            x=step*26-36; y=(-1)**step*9
            marks.paint('Ground' if paw else 'Greenline')
            if paw:
                marks.ellipsoid((x,y,1.6),(6.5,8,1.4),16,6)
                for i in range(5):
                    dx=7-abs(i-2)*.85; dy=(i-2)*3.9
                    marks.ellipsoid((x+dx,y+dy,1.9),(2.5,1.8,.9),12,5)
                    marks.beam((x+dx+3,y+dy,2),(x+dx+7,y+dy+.4,1.8),.7,16,.2)
            else:
                for k in (-1,1): marks.ellipsoid((x,y+k*2.5,1.6),(5,1.8,1.2),12,6)
        if paw:
            marks.paint('Rust')
            for x,y in [(-28,-13),(-24,-11),(4,19),(8,21),(32,-4)]:
                marks.ellipsoid((x,y,1.5),(2.8,2.2,.9),12,6)
            marks.paint('Leaf').face([(-34,-14,1),(-24,-19,2),(-29,-21,1)])
        marks.save('SM_BearSign' if paw else 'SM_HoofReplay')


def write_png(path,size,components,pixels):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+pixels[y*size*components:(y+1)*size*components] for y in range(size))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,2 if components==3 else 0,0,0,0))
                     +chunk(b'IDAT',zlib.compress(rows,7))+chunk(b'IEND',b''))


def fur():
    size=1024; rng=random.Random(227)
    heights=bytearray([68])*(size*size)
    for _ in range(125000):
        x=rng.randrange(size); y=rng.randrange(size); length=rng.randint(7,39)
        flow=math.sin(math.tau*x/size)*.35+math.sin(math.tau*y/size*2)*.25+rng.uniform(-.14,.14)
        strength=rng.randint(18,115)
        for j in range(length):
            at=((y+j)%size)*size+round(x+flow*j+.0025*j*j)%size
            heights[at]=min(245,round(heights[at]*.62+43+strength*(1-j/length)))
    color=bytearray();normal=bytearray();rough=bytearray()
    for y in range(size):
        for x in range(size):
            i=y*size+x; z=heights[i]
            color.extend((int(24+z*.42),int(17+z*.29),int(13+z*.205)))
            dx=(heights[y*size+(x-1)%size]-heights[y*size+(x+1)%size])*.009
            dy=(heights[((y-1)%size)*size+x]-heights[((y+1)%size)*size+x])*.009
            n=unit((dx,dy,1));normal.extend(round(127.5+v*127.5) for v in n)
            rough.append(int(234-z*.22))
    for name,n,data in [('BaseColor',3,color),('Normal',3,normal),('Roughness',1,rough)]:
        write_png(OUT/('T_ReliefBearFur_'+name+'.png'),size,n,data)


def huff():
    rate=22050; rng=random.Random(932); phase=0; noise=0; samples=array.array('h')
    for i in range(int(rate*1.1)):
        t=i/rate; phase+=math.tau*(61+19*math.sin(t*4.7))/rate
        noise=.87*noise+.13*rng.uniform(-1,1)
        breath=math.exp(-((t-.24)/.15)**2)+.67*math.exp(-((t-.68)/.23)**2)
        voice=.43*math.sin(phase)+.17*math.sin(phase*2)+.08*math.sin(phase*3.01)
        samples.append(int(26000*breath*(voice+noise)*(1-math.exp(-40*t))))
    with wave.open(str(OUT/'S_BearHuff.wav'),'wb') as f:
        f.setparams((1,2,rate,0,'NONE',''));f.writeframes(samples.tobytes())


def cap_surface():
    size=512;rng=random.Random(949)
    freckle=bytearray(size*size)
    for _ in range(1750):
        x,y=rng.randrange(size),rng.randrange(size);radius=rng.uniform(.5,3.5);shade=rng.randrange(32,148)
        for by in range(-3,4):
            for bx in range(-4,5):
                d=(bx/(1.3*radius))**2+(by/radius)**2
                if d<1:
                    at=((y+by)%size)*size+(x+bx)%size
                    freckle[at]=max(freckle[at],int((1-d)*shade))
    color=bytearray();normal=bytearray();rough=bytearray()
    for y in range(size):
        for x in range(size):
            at=y*size+x;f=freckle[at];a=math.tau*x/size;b=math.tau*y/size
            m=math.sin(5*a+1.8*math.cos(3*b))*.45+math.cos(9*b-2*math.sin(4*a))*.35+math.sin(21*a+18*b)*.2
            grain=rng.uniform(-7,7)
            color.extend((int(152+24*m+grain+.52*f),int(86+22*m+grain+.64*f),int(37+13*m+grain+.55*f)))
            dx=(freckle[y*size+(x-1)%size]-freckle[y*size+(x+1)%size])*.007
            dy=(freckle[((y-1)%size)*size+x]-freckle[((y+1)%size)*size+x])*.007
            normal.extend(round(127.5+127.5*v) for v in unit((dx,dy,1)))
            rough.append(int(181+17*m+.23*f))
    for name,n,data in [('BaseColor',3,color),('Normal',3,normal),('Roughness',1,rough)]:
        write_png(OUT/('T_ReliefAmberCap_'+name+'.png'),size,n,data)


if __name__=='__main__':
    bear();litter();clues();fur();cap_surface();huff()
    for name,stats in STATS.items(): print(name,stats['triangles'])
