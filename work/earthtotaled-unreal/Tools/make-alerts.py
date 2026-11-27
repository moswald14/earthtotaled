"""Render original mono alert one-shots from the game's foley and synthesized calls."""
import array
import json
import math
from pathlib import Path
import random
import wave

ART = Path(__file__).resolve().parents[1] / 'ArtSource'
RATE = 24000
# Each phrase has a different rhythm, contour and timbre. Times are in seconds.
# A note specifies onset, duration, starting frequency and ending frequency.
CALLS = {
    'Warden': ('Robot', 'radio', [(0, .16, 260, 305), (.21, .39, 610, 435)]),
    'Shard': ('ShardChirp', 'radio', [(0, .09, 920, 1210), (.13, .09, 1050, 1360), (.26, .20, 1180, 650)]),
    'Runner': ('BroodCall', 'rasp', [(0, .10, 180, 245), (.14, .07, 240, 280), (.25, .29, 260, 120)]),
    'Spitter': ('Animal', 'hiss', [(0, .46, 980, 1720), (.53, .13, 1600, 720)]),
    'Bulwark': ('BroodCall', 'growl', [(0, .37, 94, 58), (.46, .45, 130, 62)]),
    'Skimmer': ('ShardChirp', 'wing', [(0, .44, 230, 510), (.48, .16, 540, 300)]),
    'Hive': ('BroodCall', 'hollow', [(0, .87, 84, 130), (.32, .66, 143, 103)]),
    'Walker': ('Animal', 'hollow', [(0, .86, 148, 91)]),
    'Wolf': ('Animal', 'growl', [(0, .30, 110, 144), (.35, .48, 156, 90)]),
    'Boar': ('Animal', 'snort', [(0, .18, 85, 58), (.29, .24, 122, 65), (.64, .12, 110, 60)]),
    'Bear': ('BearHuff', 'roar', [(0, .23, 89, 60), (.32, .88, 73, 121)]),
}


def render(name, stem, voice, notes):
    with wave.open(str(ART / f'S_{stem}.wav'), 'rb') as source:
        assert source.getsampwidth() == 2
        source_rate = source.getframerate()
        foley = array.array('h', source.readframes(source.getnframes()))[::source.getnchannels()]
    rng = random.Random(name)
    audio = [0.] * int((max(t + d for t, d, _, _ in notes) + .035) * RATE)
    for onset, duration, start, end in notes:
        phase = low = band = 0.
        for i in range(int(duration * RATE)):
            t = i / RATE
            u = t / duration
            env = math.sin(math.pi * u) ** (.6 if voice == 'radio' else 1.05)
            frequency = start + (end - start) * u
            phase += math.tau * frequency * (1 + .018 * math.sin(30 * t)) / RATE
            noise = rng.uniform(-1, 1)
            low += .11 * (noise - low)
            band += .55 * (noise - band)
            pulse = math.sin(phase)
            texture = foley[int(t * source_rate * .81) % len(foley)] / 32768.
            if voice == 'radio':
                sample = .56 * pulse + .22 * math.sin(2 * phase) + .18 * math.sin(phase * 3.03)
            elif voice == 'hiss':
                sample = 2.1 * (band - low) + .16 * math.sin(phase + 2.9 * math.sin(phase * .47))
            elif voice == 'wing':
                sample = (.58 * pulse + .3 * math.sin(2.04 * phase)) * (.55 + .45 * math.sin(175 * t)) + low
            elif voice == 'hollow':
                sample = .38 * pulse + .29 * math.sin(3 * phase) + .18 * math.sin(5.05 * phase) + .9 * low
            else:
                rate = {'growl':32, 'rasp':61, 'snort':43, 'roar':21}[voice]
                grit = (1.3 * low + .50 * pulse + .25 * math.sin(phase * 2.03))
                sample = grit * (.68 + .32 * math.sin(t * rate * math.tau))
                if voice in ('snort', 'roar'):
                    sample += (band - low) * (.7 if voice == 'roar' else 1.4)
            audio[int(onset * RATE) + i] += env * (.83 * sample + .28 * texture)
    gain = 24000 / max(abs(a) for a in audio)
    with wave.open(str(ART / f'S_Alert{name}.wav'), 'wb') as out:
        out.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        out.writeframes(array.array('h', (round(gain * a) for a in audio)).tobytes())


if __name__ == '__main__':
    for name, profile in CALLS.items():
        render(name, *profile)
    manifest = ART / 'manifest.json'
    data = json.loads(manifest.read_text())
    data['sounds'] = sorted(p.name for p in ART.glob('*.wav'))
    manifest.write_text(json.dumps(data, indent=2) + '\n')
