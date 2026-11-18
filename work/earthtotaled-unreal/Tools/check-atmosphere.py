"""Audio delivery contract: useful levels, stereo score and seamless loops."""
from pathlib import Path
from array import array
import wave, math, json
root=Path(__file__).resolve().parents[1]/'ArtSource'
expected={'MusicForest':(2,64,True),'MusicCity':(2,64,True),'MusicNight':(2,64,True),
          'WeatherRain':(1,16,True),'WeatherWind':(1,24,True),'WeatherThunder':(1,8,False)}
for name,(channels,seconds,loop) in expected.items():
    path=root/('S_'+name+'.wav')
    assert path.exists(), 'Missing weather/music asset: '+name
    with wave.open(str(path),'rb') as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(channels,2,44100),name
        assert abs(w.getnframes()/w.getframerate()-seconds)<.001,name
        samples=array('h'); samples.frombytes(w.readframes(w.getnframes()))
    peak=max(abs(v) for v in samples)/32768
    rms=math.sqrt(sum((v/32768)**2 for v in samples)/len(samples))
    assert .02<rms<.35 and .2<peak<.95,(name,rms,peak)
    if loop: assert max(abs(samples[c]-samples[-channels+c]) for c in range(channels))<32768*.2,'Loop discontinuity: '+name
    if channels==2: assert sum(abs(samples[i]-samples[i+1]) for i in range(0,len(samples),200))>10000,'Score lacks stereo width'
    print('PASS',name,'format, duration, levels'+(', loop join' if loop else ''))
assert (root/'T_WeatherClouds.png').is_file()
print('Weather/music assets passed.')
