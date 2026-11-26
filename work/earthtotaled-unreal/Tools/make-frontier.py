"""Original Fieldworks models and short gameplay cues."""
import array
import json
import math
import random
import wave
from meshkit import Mesh, OUT, STATS


def drone():
    m = Mesh()
    m.paint('DarkMetal').ellipsoid((0, 0, 0), (34, 24, 14), 18, 10)
    m.paint('Signal').ellipsoid((29, 0, 1), (9, 15, 8), 12, 8)
    for x in (-28, 26):
        for side in (-1, 1):
            y = side * 38
            m.paint('Steel').beam((x*.5, 0, 0), (x, y, 4), 4)
            m.paint('Rust').torus((x, y, 4), 17, 3, 'z', 20, 6)
            m.paint('DarkMetal').box((x, y, 5), (29, 5, 2), x+side*15)
    m.save('SM_ShardDrone')


def brood(spitter):
    m = Mesh()
    m.paint('DarkMetal').ellipsoid((-12, 0, 43), (60, 32, 27), 18, 10)
    m.paint('Rust').ellipsoid((35, 0, 52), (30, 26, 21), 16, 8)
    for side in (-1, 1):
        for i, x in enumerate((-42, -13, 19)):
            knee = (x-12+i*6, side*(48+i*4), 59-i*6)
            m.paint('Rust').beam((x, side*19, 43), knee, 9)
            m.paint('DarkMetal').beam(knee, (x+10, side*(65+i*6), 3), 7, taper=.1)
        m.paint('Signal').ellipsoid((55, side*15, 58), (7, 5, 5), 10, 6)
        m.paint('Gold').beam((49, side*17, 43), (77, side*29, 22), 8, taper=.08)
    for i in range(4):
        x = -51+i*20
        m.paint('Rust').ellipsoid((x, 0, 61), (15, 29-i*2, 15), 14, 7)
        m.paint('Gold').beam((x, 0, 69), (x-14, 0, 94-i*4), 7, taper=.1)
    if spitter:
        for side in (-1, 1):
            m.paint('Greenline').ellipsoid((-22, side*17, 80), (37, 19, 26), 18, 10)
            m.paint('DarkMetal').beam((1, side*14, 82), (44, side*12, 87), 10, taper=.62)
    m.save('SM_BroodSpitter' if spitter else 'SM_BroodRunner')


def build_parts():
    # Geometry is centered for each placement transform, including the inclined deck.
    for name, length in [('Floor', 400), ('Ramp', math.hypot(400, 280))]:
        m = Mesh()
        m.paint('Steel').box((0, 0, -5), (length, 400, 10))
        for i in range(8):
            m.paint('Wood').box((0, -175+i*50, 3), (length-2, 48, 12))
        for side in (-1, 1):
            m.paint('Rust').box((0, side*190, 8), (length-2, 8, 4))
        m.save('SM_Field'+name)
    m = Mesh()
    m.paint('Rust').box((0, 0, 0), (12, 398, 278))
    for i in range(8):
        m.paint('Wood').box((6, -175+i*50, 0), (6, 47, 268))
    for h in (-112, 112):
        m.paint('Steel').box((11, 0, h), (8, 396, 9))
    m.paint('DarkMetal').beam((14, -179, -120), (14, 179, 120), 5)
    m.save('SM_FieldWall')
    m = Mesh()
    corners = [(-200, -200, -70), (200, -200, -70), (200, 200, -70), (-200, 200, -70)]
    m.paint('DarkMetal').face(list(reversed(corners)))
    for i, c in enumerate(corners):
        m.paint('Rust' if i%2 else 'Steel').face([c, corners[(i+1)%4], (0, 0, 70)])
        m.paint('Wood').beam(c, (0, 0, 69), 4)
    m.save('SM_FieldRoof')


def cue(name, duration, mode):
    rate = 22050
    rng = random.Random(mode)
    samples = array.array('h')
    noise = 0
    for i in range(round(rate*duration)):
        t = i/rate
        noise = .76*noise + .24*rng.uniform(-1, 1)
        if mode == 1:
            value = .9*noise + .18*math.sin(math.tau*(91*t+12*t*t))
        elif mode == 2:
            value = .33*math.sin(math.tau*(880*t-490*t*t))*(.5+.5*math.cos(42*t)) + .18*noise
        elif mode == 3:
            value = .44*math.sin(math.tau*(150*t+185*t*t)+1.7*math.sin(125*t)) + .5*noise
        else:
            value = (.7*noise+.28*math.sin(580*t))*math.exp(-8*t) + .15*math.sin(2100*t)*math.exp(-5*t)
        env = min(1, t/.02, (duration-t)/.06)
        samples.append(round(max(-1, min(1, value*env))*30000))
    with wave.open(str(OUT/('S_'+name+'.wav')), 'wb') as clip:
        clip.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
        clip.writeframes(samples.tobytes())


if __name__ == '__main__':
    drone()
    brood(False)
    brood(True)
    build_parts()
    for i, (name, length) in enumerate([('JetThrust', .68), ('ShardChirp', .37), ('BroodCall', .69), ('FieldLatch', .38)], 1):
        cue(name, length, i)
    path = OUT/'mesh-detail.json'
    details = json.loads(path.read_text()) if path.exists() else {}
    details.update(STATS)
    path.write_text(json.dumps(details, indent=2))
    path = OUT/'manifest.json'
    if path.exists():
        manifest = json.loads(path.read_text())
        for key, pattern in [('meshes','*.obj'), ('sounds','*.wav')]:
            manifest[key] = sorted(p.name for p in OUT.glob(pattern))
        path.write_text(json.dumps(manifest, indent=2))
