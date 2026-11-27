"""Author the open freight sled, modular guideway and walk-up stations in centimetres."""
import json
import runpy
from pathlib import Path
from meshkit import Mesh, OUT, STATS


def sled():
    m = Mesh()
    m.paint('DarkMetal').box((0, 0, -48), (2370, 602, 78), bevel=23)
    for x in range(-1000, 1001, 250):
        m.paint('Steel').box((x, 0, -53), (42, 626, 58), bevel=6)
    for side in (-1, 1):
        y = side * 303
        m.paint('Rust').box((0, y, -34), (2380, 18, 45), bevel=5)
        for x in range(-1080, 1081, 135):
            m.paint('Canvas').box((x, y, -6), (78, 19, 7), bevel=2)
            m.paint('Steel').beam((x, y, -35), (x, y+side*12, -35), 5, 16, 1)
        for x in (-890, -660, 660, 890):
            m.paint('Rubber').beam((x, side*109-15, -94), (x, side*109+15, -94), 30, 32, 1)
            m.paint('Steel').torus((x, side*116, -94), 30, 5, 'y', 32, 8)
            m.paint('DarkMetal').box((x, side*163, -77), (110, 58, 40), bevel=8)
        for x in (-1000, 1000):
            m.paint('CircuitLight').box((x, y+side*6, -27), (108, 9, 9), bevel=3)
    for ix in range(8):
        for iy in range(4):
            x, y = (ix-3.5)*295, (iy-1.5)*148
            m.paint('Steel').box((x, y, -5), (293, 146, 10), bevel=2)
            for corner in (-1, 1):
                m.paint('DarkMetal').beam((x+corner*131, y+59, -.5), (x+corner*131, y+59, .8), 2.8, 8, 1)
    m.save('SM_RingSled')


def guideway():
    m = Mesh()
    m.paint('DarkMetal').box((0, 0, -177), (2200, 82, 110), bevel=5)
    for y in (-110, 110):
        m.paint('Rust').box((0, y, -143), (2200, 32, 40), bevel=3)
        m.paint('Steel').box((0, y, -128), (2200, 49, 12), bevel=3)
    for x in range(-990, 1000, 220):
        m.paint('Concrete').box((x, 0, -167), (39, 313, 38), bevel=4)
        for y in (-137, 137):
            m.paint('Steel').box((x, y, -143), (23, 22, 12), bevel=2)
    m.save('SM_RingGuide')
    m = Mesh()
    m.paint('Concrete').box((0, 0, -562), (116, 144, 874), bevel=12)
    m.paint('Concrete').box((0, 0, -117), (160, 390, 32), bevel=10)
    for y in (-120, 120):
        m.paint('Rust').beam((0, 0, -250), (0, y, -125), 24, 12, .9)
    m.save('SM_RingPier')


def station():
    m = Mesh()
    m.paint('Concrete').box((0, 610, -26), (2410, 462, 52), bevel=5)
    for x in range(-1125, 1150, 150):
        m.paint('Canvas').box((x, 401, 1), (112, 30, 3), bevel=1)
    for x in (-1000, 1000):
        for y in (585, 1360):
            m.paint('Steel').box((x, y, -1820), (50, 50, 3640), bevel=3)
            m.paint('Concrete').box((x, y, -3570), (180, 180, 95), bevel=8)
        for z in range(-3440, -200, 600):
            m.paint('Rust').beam((x, 585, z), (x, 1360, z+560), 8, 10, 1)
    for flight in range(6):
        y = 990 if flight%2==0 else 1210
        direction = 1 if flight%2==0 else -1
        base = -3600+600*flight
        for i in range(30):
            x = direction*(-750+25+50*i)
            m.paint('Concrete').box((x, y, base+20*(i+1)-10), (50.4, 201, 20), bevel=1)
        for side in (-1,1):
            m.paint('DarkMetal').beam((-750*direction, y+side*99, base-14), (750*direction, y+side*99, base+586), 11, 10, 1)
        m.paint('Greenline').beam((-750*direction, y+direction*-100, base+88), (750*direction, y+direction*-100, base+688), 4, 10, 1)
        m.paint('Steel').box((direction*856, 1100, base+590), (212, 434, 20), bevel=2)
    m.paint('Steel').box((-856, 850, -10), (212, 420, 20), bevel=2)
    for x in (-1030, 1030):
        m.paint('Greenline').box((x, 795, 169), (22, 22, 338), bevel=2)
        m.paint('CircuitLight').box((x, 795, 347), (40, 40, 22), bevel=4)
    m.save('SM_RingStation')


def generate_railway():
    sled()
    guideway()
    station()


if __name__ == '__main__':
    nature=runpy.run_path(str(Path(__file__).with_name('environment-art.py')))
    nature['pine'](138,'SM_Pine')
    nature['pine'](238,'SM_PineB',1370)
    nature['architecture'](tenement_only=True)
    generate_railway()
    detail=OUT/'mesh-detail.json'
    data=json.loads(detail.read_text())
    data.update(STATS)
    detail.write_text(json.dumps(data,indent=2))
    manifest=OUT/'manifest.json'
    data=json.loads(manifest.read_text())
    for key,pattern in [('meshes','*.obj'),('textures','*.png')]:
        data[key]=sorted(p.name for p in OUT.glob(pattern))
    manifest.write_text(json.dumps(data,indent=2))
