"""Original mesh authoring primitives, with bevels, smooth normals and material slots."""
import math
from pathlib import Path
OUT=Path(__file__).resolve().parents[1]/'ArtSource'
STATS={}
def unit(v):
    d=math.sqrt(sum(x*x for x in v)); return tuple(x/d for x in v) if d>1e-12 else (0,0,1)
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
class Mesh:
    def __init__(self):
        self.vertices=[]; self.uv=[]; self.normals=[]; self.faces=[]; self.face_materials=[]; self.material='Base'
    def paint(self,name): self.material=name; return self
    def face(self,points,uv=None,normals=None):
        normal=unit(cross(sub(points[1],points[0]),sub(points[2],points[0])))
        axis=max(range(3),key=lambda i:abs(normal[i])); axes=[i for i in range(3) if i!=axis]
        start=len(self.vertices)+1; self.vertices.extend(points)
        self.uv.extend(uv or [(p[axes[0]]/200,p[axes[1]]/200) for p in points]); self.normals.extend(normals or [normal]*len(points))
        for i in range(1,len(points)-1): self.faces.append((start,start+i,start+i+1)); self.face_materials.append(self.material)
    def box(self,center,size,yaw=0,bevel=None):
        e=[v*.5 for v in size]; r=min(min(size)*.12,1.4) if bevel is None else min(bevel,min(e)*.8)
        c=[v-r for v in e]; angle=math.radians(yaw)
        def transform(p): return (center[0]+p[0]*math.cos(angle)-p[1]*math.sin(angle),center[1]+p[0]*math.sin(angle)+p[1]*math.cos(angle),center[2]+p[2])
        def face(points):
            n=cross(sub(points[1],points[0]),sub(points[2],points[0])); mid=[sum(p[i] for p in points)/len(points) for i in range(3)]
            if sum(n[i]*mid[i] for i in range(3))<0: points=list(reversed(points))
            self.face([transform(p) for p in points])
        for k in range(3):
            i,j=[x for x in range(3) if x!=k]
            for sign in [-1,1]:
                pts=[]
                for a,b in [(-1,-1),(1,-1),(1,1),(-1,1)]:
                    p=[0,0,0]; p[k]=sign*e[k]; p[i]=a*c[i]; p[j]=b*c[j]; pts.append(p)
                face(pts)
        if not r: return
        for k in range(3):
            i,j=[x for x in range(3) if x!=k]
            for a in [-1,1]:
                for b in [-1,1]:
                    pts=[]
                    for end,side in [(-1,0),(1,0),(1,1),(-1,1)]:
                        p=[0,0,0]; p[k]=end*c[k]; p[i]=a*(e[i] if side==0 else c[i]); p[j]=b*(c[j] if side==0 else e[j]); pts.append(p)
                    face(pts)
        for x in [-1,1]:
            for y in [-1,1]:
                for z in [-1,1]:
                    signs=[x,y,z]; pts=[]
                    for k in range(3): pts.append([signs[i]*(e[i] if i==k else c[i]) for i in range(3)])
                    face(pts)
    def beam(self,a,b,radius,segments=24,taper=.8):
        segments=max(segments,16); axis=unit(sub(b,a)); length=math.dist(a,b)
        ref=(0,0,1) if abs(axis[2])<.95 else (0,1,0); u=unit(cross(axis,ref)); v=cross(axis,u)
        def radial(i): return tuple(math.cos(i*math.tau/segments)*u[k]+math.sin(i*math.tau/segments)*v[k] for k in range(3))
        rings=[[tuple(origin[k]+r*radial(i)[k] for k in range(3)) for i in range(segments)] for origin,r in [(a,radius),(b,radius*taper)]]
        normals=[unit(tuple(radial(i)[k]+axis[k]*radius*(1-taper)/length for k in range(3))) for i in range(segments)]
        for i in range(segments):
            j=(i+1)%segments; self.face([rings[0][i],rings[0][j],rings[1][j],rings[1][i]],[(i/segments,0),((i+1)/segments,0),((i+1)/segments,length/100),(i/segments,length/100)],[normals[i],normals[j],normals[j],normals[i]])
        self.face(list(reversed(rings[0]))); self.face(rings[1])
    def ellipsoid(self,center,radii,segments=28,rings=14):
        def p(j,i):
            a=math.pi*j/rings; b=math.tau*i/segments; n=(math.sin(a)*math.cos(b),math.sin(a)*math.sin(b),math.cos(a))
            return tuple(center[k]+radii[k]*n[k] for k in range(3)),unit(tuple(n[k]/radii[k] for k in range(3)))
        for j in range(rings):
            for i in range(segments):
                indices=[(j,i),(j+1,i),(j+1,i+1),(j,i+1)]
                if j==0: indices=indices[:3]
                if j==rings-1: indices=[indices[0],indices[1],indices[3]]
                points,normals=zip(*(p(a,b) for a,b in indices)); self.face(points,[(b/segments,a/rings) for a,b in indices],normals)
    def torus(self,center,major,minor,axis='x',segments=40,sides=10):
        def p(i,j):
            a=i*math.tau/segments; b=j*math.tau/sides; rr=major+minor*math.cos(b)
            pos=(minor*math.sin(b),rr*math.cos(a),rr*math.sin(a)); n=(math.sin(b),math.cos(b)*math.cos(a),math.cos(b)*math.sin(a))
            order=(0,1,2) if axis=='x' else (1,0,2) if axis=='y' else (1,2,0)
            return tuple(center[k]+pos[order[k]] for k in range(3)),tuple(n[order[k]] for k in range(3))
        for i in range(segments):
            for j in range(sides):
                points,normals=zip(*(p(a,b) for a,b in [(i,j),(i+1,j),(i+1,j+1),(i,j+1)]))
                if sum(cross(sub(points[1],points[0]),sub(points[2],points[0]))[k]*normals[0][k] for k in range(3))<0: points=points[::-1]; normals=normals[::-1]
                self.face(points,normals=normals)
    def copy_from(self,other,offset=(0,0,0)):
        base=len(self.vertices); self.vertices.extend(tuple(p[k]+offset[k] for k in range(3)) for p in other.vertices)
        self.uv.extend(other.uv); self.normals.extend(other.normals); self.faces.extend(tuple(i+base for i in face) for face in other.faces); self.face_materials.extend(other.face_materials)
    def save(self,name):
        materials=sorted(set(self.face_materials),key=lambda x:(x!='Base',x))
        with (OUT/(name+'.obj')).open('w',encoding='ascii') as f:
            f.write('# Original EARTHtotaled detailed mesh, centimeters, reflected Y for Unreal OBJ\nmtllib '+name+'.mtl\n')
            for x,y,z in self.vertices: f.write('v %.6f %.6f %.6f\n'%(x,-y,z))
            for uv in self.uv: f.write('vt %.6f %.6f\n'%uv)
            for x,y,z in self.normals: f.write('vn %.7f %.7f %.7f\n'%(x,-y,z))
            for material in materials:
                f.write('usemtl '+material+'\n')
                for face,mat in zip(self.faces,self.face_materials):
                    if mat==material: f.write('f '+' '.join(f'{i}/{i}/{i}' for i in reversed(face))+'\n')
        (OUT/(name+'.mtl')).write_text(''.join('newmtl '+m+'\nKd 0.5 0.5 0.5\n' for m in materials))
        STATS[name]={'triangles':len(self.faces),'vertices':len(self.vertices),'materials':materials}
