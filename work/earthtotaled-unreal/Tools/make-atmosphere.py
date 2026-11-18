"""Original looping score, weather recordings and cloud texture. Standard library only.

The score is composed here as notes, chords, envelopes and stereo delay; no samples
or third-party songs are used. Circular mixing makes the ends join without a gap.
"""
from pathlib import Path
from array import array
import math, random, struct, wave, json, zlib

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource'
RATE=44100
TAU=math.tau
REPORT={}

def write(name,left,right=None,loop=False,title=''):
    channels=[left] if right is None else [left,right]
    peak=max(max(abs(x) for x in ch) for ch in channels)
    scale=(.68 if name.startswith('Music') else .72)/max(peak,.0001)
    pcm=array('h')
    for frame in zip(*channels): pcm.extend(round(v*scale*32767) for v in frame)
    with wave.open(str(OUT/('S_'+name+'.wav')),'wb') as w:
        w.setnchannels(len(channels)); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(pcm.tobytes())
    REPORT[name]={'title':title,'seconds':round(len(left)/RATE,3),'channels':len(channels),'loop':loop,
                  'rms':round(math.sqrt(sum((v/32768)**2 for v in pcm)/len(pcm)),5),
                  'peak':max(abs(v) for v in pcm)/32768,
                  'join_delta':max(abs(ch[0]-ch[-1])*scale for ch in channels)}
    print('Created',name,flush=True)

def score(kind,chords,melody,title):
    duration=64; n=RATE*duration
    left=array('f',[0])*n; right=array('f',[0])*n
    def note(midi,start,length,volume,pan,pad=False):
        f=440*2**((midi-69)/12); start=int(start*RATE); total=int(length*RATE)
        lg=math.sqrt((1-pan)/2)*volume; rg=math.sqrt((1+pan)/2)*volume
        phase=TAU*f/RATE; detune=phase*1.0016
        for k in range(total):
            t=k/RATE; x=k/total
            if pad:
                envelope=math.sin(math.pi*x)**2
                value=(math.sin(k*phase)*.66+math.sin(k*detune)*.24+math.sin(k*phase*2)*.1)*envelope
            else:
                envelope=min(1,t/.018)*math.exp(-t/1.8)*min(1,(length-t)/.22)
                value=(math.sin(k*phase)*.74+math.sin(k*phase*2)*.16+math.sin(k*phase*3)*.07+math.sin(k*phase*4)*.03)*envelope
            idx=(start+k)%n; left[idx]+=value*lg; right[idx]+=value*rg
    for bar,chord in enumerate(chords):
        for j,midi in enumerate(chord): note(midi,bar*16-6,28,.095,[-.65,.5,-.25,.7][j],True)
        note(chord[0]-12,bar*16,16,.065,0,True)
        if kind=='city':
            for beat in range(0,16,2): note(chord[0]-12,bar*16+beat,1.9,.055,-.08)
    for i,midi in enumerate(melody):
        if midi: note(midi,i*2+.15,5.5,.13 if kind!='night' else .105,math.sin(i*2.3)*.65)
    dryl=array('f',left); dryr=array('f',right)
    for delay,gain in [(.327,.21),(.673,.16),(1.117,.12),(1.731,.08)]:
        offset=int(delay*RATE)
        for i in range(n):
            left[i]+=dryr[(i-offset)%n]*gain; right[i]+=dryl[(i-offset)%n]*gain
    write('Music'+kind.title(),left,right,True,title)

def weather():
    # Seamless wind and rain noise with slowly varying, periodic gust envelopes.
    for name,seconds in [('WeatherRain',16),('WeatherWind',24)]:
        rng=random.Random(name); samples=array('f'); low=0; slow=0; last=0
        for i in range(RATE*seconds):
            t=i/RATE; noise=rng.uniform(-1,1); low+=.035*(noise-low); slow+=.0016*(noise-slow)
            if name=='WeatherRain':
                value=(noise*.13+low*.7)*(1+.16*math.sin(TAU*t/seconds))
                # Tiny dense splashes, not large rhythmic drips.
                last=last*.92+(rng.uniform(.02,.10) if rng.random()<.0008 else 0)
                value+=last*math.sin(TAU*t*2600)
            else: value=(low*.45+slow*2.8)*(1+.38*math.sin(TAU*t/seconds)+.12*math.sin(TAU*t*3/seconds))
            samples.append(value)
        # Match the noise seam using a short equal-time overlap, with no silent gap.
        fade=int(.08*RATE)
        for i in range(fade):
            a=(i+1)/fade; samples[i]=samples[-fade+i]*(1-a)+samples[i]*a
        write(name,samples,loop=True)
    rng=random.Random(1843); samples=array('f'); rumble=0
    for i in range(RATE*8):
        t=i/RATE; rumble+=.012*(rng.uniform(-1,1)-rumble)
        attack=min(1,t/.14); tail=min(1,(8-t)/1.8)
        booms=sum(math.exp(-(t-b)*1.2) for b in [0,.6,1.65] if t>=b)
        value=(rumble*2.8+math.sin(TAU*t*39)*.1+math.sin(TAU*t*63)*.07)*booms*attack*tail
        samples.append(value)
    write('WeatherThunder',samples)

def cloud_texture():
    n=512; rng=random.Random(7313); fields=[]
    for size in [4,8,16,32,64]: fields.append((size,[rng.random() for _ in range(size*size)]))
    pixels=bytearray()
    for y in range(n):
        for x in range(n):
            value=0; weight=0
            for octave,(size,data) in enumerate(fields):
                fx=x*size/n; fy=y*size/n; ix=int(fx); iy=int(fy); a=fx-ix; b=fy-iy
                a=a*a*(3-2*a); b=b*b*(3-2*b)
                row0=data[(iy%size)*size+ix%size]*(1-a)+data[(iy%size)*size+(ix+1)%size]*a
                row1=data[((iy+1)%size)*size+ix%size]*(1-a)+data[((iy+1)%size)*size+(ix+1)%size]*a
                w=.54**octave; value+=(row0*(1-b)+row1*b)*w; weight+=w
            v=max(0,min(255,round(((value/weight-.5)*1.7+.5)*255))); pixels.extend([v,v,v])
    def chunk(tag,body): return struct.pack('>I',len(body))+tag+body+struct.pack('>I',zlib.crc32(tag+body)&0xffffffff)
    rows=b''.join(b'\0'+pixels[y*n*3:(y+1)*n*3] for y in range(n))
    (OUT/'T_WeatherClouds.png').write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',n,n,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,8))+chunk(b'IEND',b''))

def generate():
    OUT.mkdir(exist_ok=True)
    score('forest',[[50,57,60,64],[46,53,57,62],[53,57,60,67],[48,55,62,64]],
        [74,0,69,72,65,0,67,69,70,0,65,69,62,0,65,67,69,72,0,77,74,0,72,69,67,0,64,62,67,0,69,0], 'Embers in the Canopy')
    score('city',[[50,57,60,65],[43,50,58,62],[46,53,57,62],[45,52,57,62]],
        [62,0,69,65,62,0,60,0,62,0,65,70,69,0,65,0,65,0,62,69,70,0,65,62,64,0,69,74,69,0,62,0], 'Concrete Echoes')
    score('night',[[50,57,60,64],[46,53,57,62],[43,50,57,62],[50,57,60,65]],
        [81,0,0,77,0,74,0,0,77,0,0,74,0,70,0,0,74,0,0,69,0,67,0,0,69,0,0,65,0,62,0,0], 'The Last Frequency')
    weather(); cloud_texture()
    (OUT/'atmosphere-audio.json').write_text(json.dumps({'license':'Original EARTHtotaled compositions and synthesized weather; no third-party samples.','sample_rate':RATE,'sounds':REPORT},indent=2))
    manifest=OUT/'manifest.json'
    if manifest.exists():
        d=json.loads(manifest.read_text()); d['sounds']=sorted(p.name for p in OUT.glob('*.wav')); d['textures']=sorted(p.name for p in OUT.glob('*.png')); manifest.write_text(json.dumps(d,indent=2))

if __name__=='__main__': generate()
