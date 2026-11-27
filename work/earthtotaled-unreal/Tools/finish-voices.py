"""Normalize the locally synthesized PCM, add a restrained robot radio treatment, and verify."""
from pathlib import Path
import array, json, math, sys, wave

root=Path(__file__).resolve().parents[1]
dialogue=json.loads((root/'Content/Data/dialogue.json').read_text())
total=0
lines=[line for line in dialogue['lines'] if not sys.argv[1:] or line['id'] in sys.argv[1:]]
for line in lines:
    folder=root/('Content/Data/Voices' if line['id'].startswith('wake') else 'ArtSource')
    path=folder/('S_Voice_'+line['id']+'.wav')
    with wave.open(str(path),'rb') as sound:
        assert sound.getnchannels()==1 and sound.getsampwidth()==2 and sound.getframerate()==22050
        samples=array.array('h',sound.readframes(sound.getnframes()))
    assert len(samples)>22050, path
    values=[x/32768 for x in samples]
    peak=max(abs(x) for x in values)
    assert peak>.01, ('Silent voice',path)
    values=[x*.78/peak for x in values]
    if line.get('robot'):
        # Keep the clear voice dominant. A quiet delayed, ring-modulated copy suggests a radio.
        source=values[:]
        delay=460
        for i in range(len(values)):
            echo=source[i-delay] if i>=delay else 0
            values[i]=source[i]*.86+echo*.1+source[i]*math.sin(i*math.tau*43/22050)*.12
    fade=220
    for i in range(min(fade,len(values))):
        values[i]*=i/fade
        values[-1-i]*=i/fade
    pcm=array.array('h',(round(max(-.95,min(.95,x))*32767) for x in values))
    with wave.open(str(path),'wb') as sound:
        sound.setnchannels(1); sound.setsampwidth(2); sound.setframerate(22050); sound.writeframes(pcm.tobytes())
    duration=len(pcm)/22050
    rms=math.sqrt(sum((x/32768)**2 for x in pcm)/len(pcm))
    assert 1<duration<30 and rms>.015, (path,duration,rms)
    total+=duration
    print(f'{path.name}: {duration:.2f}s, RMS {rms:.3f}, mono 22050Hz PCM16')
print(f'{len(lines)} voice clips verified; {total:.1f} seconds total.')
