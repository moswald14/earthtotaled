"""Original empty candybar-wrapper pickup: folded printed foil with torn/crimped ends."""
from pathlib import Path
import math, random, json
from meshkit import Mesh, STATS
ROOT=Path(__file__).resolve().parents[1]

def generate():
    m=Mesh(); r=random.Random(84371)
    for n,(cx,cy,angle) in enumerate([(-10,-7,.35),(10,-6,2.1),(-5,9,-.7),(12,9,1.4)]):
        xs=[-1,-.8,-.55,0,.55,.8,1]; ys=[-1,-.5,0,.5,1]
        points={}
        for i,x in enumerate(xs):
            for j,y in enumerate(ys):
                px=x*12; py=y*5.5
                # Hollow sheets rather than filled candy bars; lifted edges and irregular creases.
                z=2+n*.9+abs(y)**2*2+math.sin(i*2.1+j*.8+n)*1.15
                if abs(x)==1: px+=r.uniform(-1.2,1.2); z+=1.4*(j%2)
                points[i,j]=(cx+px*math.cos(angle)-py*math.sin(angle),cy+px*math.sin(angle)+py*math.cos(angle),z)
        for i in range(len(xs)-1):
            for j in range(len(ys)-1):
                quad=[points[i,j],points[i+1,j],points[i+1,j+1],points[i,j+1]]
                for tri in [quad[:3],[quad[0],quad[2],quad[3]]]:
                    m.paint('Steel' if i in [0,5] else 'Canvas' if j==1 else 'Signal' if n%2==0 else 'Greenline')
                    m.face(tri)
                    m.paint('Steel'); m.face([(x,y,z-.04) for x,y,z in reversed(tri)])
    m.save('SM_Wrappers')
    p=ROOT/'ArtSource/mesh-detail.json'; d=json.loads(p.read_text()); d.update(STATS); p.write_text(json.dumps(d,indent=2))
    p=ROOT/'ArtSource/manifest.json'; d=json.loads(p.read_text()); d['meshes']=sorted(x.name for x in (ROOT/'ArtSource').glob('*.obj')); p.write_text(json.dumps(d,indent=2))
    print('Candybar wrapper pickup:',STATS['SM_Wrappers'])

if __name__=='__main__': generate()
