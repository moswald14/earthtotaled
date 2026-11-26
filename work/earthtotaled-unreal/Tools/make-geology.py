"""Build the survey borer and original low-resolution mineral textures and sounds."""
import math
import json
import random
import struct
import wave
import zlib
from meshkit import Mesh, OUT, STATS


def borer():
    mesh = Mesh()
    mesh.paint('DarkMetal').box((-13, 0, -2), (30, 13, 12))
    mesh.paint('Rubber').box((-12, 0, -11), (7, 7, 15))
    mesh.paint('Steel').beam((-10, 0, 0), (19, 0, 0), 6.4, taper=1)
    for x in (-6, 0, 6, 12):
        mesh.paint('Rust').torus((x, 0, 0), 6.4, 1.2, segments=12, sides=4)
    for side in (-1, 1):
        mesh.paint('Steel').box((9, side*6, 1.5), (25, 2.1, 3.2))
        mesh.paint('Gold').beam((18, side*5, 1.5), (29, side*3.5, 1.5), 2.4, taper=.34)
    mesh.paint('DarkMetal').box((-15, 0, 5.5), (12, 9, 4))
    mesh.paint('Greenline').box((-16, 0, 7.6), (6, 6, .8))
    mesh.save('SM_SurveyBorer')


def mineral(name, palette, seed):
    rng = random.Random(seed)
    size = 64
    data = bytearray()
    for y in range(size):
        data.append(0)
        for x in range(size):
            vein = math.sin(math.tau*(y/size*3 + x/size + .14*math.sin(math.tau*x/size*2)))
            grain = rng.random()
            band = 3 if vein+.24*grain > .96 else 2 if vein+.5*grain > .65 else 1 if grain > .48 else 0
            data.extend(palette[band])
    def chunk(kind, raw):
        return struct.pack('>I', len(raw)) + kind + raw + struct.pack('>I', zlib.crc32(kind+raw))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', size, size, 8, 2, 0, 0, 0))
    (OUT/('T_'+name+'.png')).write_bytes(png+chunk(b'IDAT', zlib.compress(bytes(data), 9))+chunk(b'IEND', b''))


def sound(name, duration, ringing):
    rate = 24000
    rng = random.Random(156 if ringing else 333)
    samples = bytearray()
    low = 0
    for i in range(round(rate*duration)):
        t = i/rate
        low = .84*low + .16*rng.uniform(-1, 1)
        if ringing:
            v = sum(math.sin(math.tau*f*t)*a*math.exp(-k*t) for f, a, k in ((650,.4,9),(977,.23,12),(1641,.13,17)))
        else:
            v = .56*math.sin(math.tau*(43*t + 2.6*(1-math.exp(-24*t))))*math.exp(-6*t)
            v += low*1.2*math.exp(-4*t) + rng.uniform(-.22,.22)*math.exp(-22*t)
        v *= min(1, i/55, (rate*duration-i)/220)
        samples.extend(struct.pack('<h', int(30000*max(-1, min(1, v)))))
    with wave.open(str(OUT/('S_'+name+'.wav')), 'wb') as wav:
        wav.setparams((1,2,rate,0,'NONE','not compressed'))
        wav.writeframes(samples)


if __name__ == '__main__':
    borer()
    mineral('FaultShale', [(56,48,55),(76,66,68),(95,77,70),(124,99,78)], 1)
    mineral('FaultIron', [(49,52,58),(86,67,65),(137,80,52),(197,151,110)], 2)
    mineral('FaultGold', [(73,52,52),(106,74,47),(211,150,47),(255,227,134)], 3)
    sound('BorerPulse', .78, False)
    sound('OreChime', .57, True)
    manifest_path = OUT/'manifest.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        for key, pattern in [('meshes','*.obj'),('textures','*.png'),('sounds','*.wav')]:
            manifest[key] = sorted(p.name for p in OUT.glob(pattern))
        manifest_path.write_text(json.dumps(manifest, indent=2))
    details_path = OUT/'mesh-detail.json'
    if details_path.exists():
        details = json.loads(details_path.read_text())
        details['SM_SurveyBorer'] = STATS['SM_SurveyBorer']
        details_path.write_text(json.dumps(details, indent=2))
