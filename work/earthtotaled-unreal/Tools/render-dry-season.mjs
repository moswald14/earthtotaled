import { resolve, dirname, join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { writeFileSync } from 'node:fs';

// Pass a checkout of Doodle Jazz. Only the resulting stereo music is needed at runtime.
const { djMakeBank, DJRenderer } = await import(pathToFileURL(resolve(process.argv[2], 'jazz-core.mjs')));
const out = resolve(dirname(fileURLToPath(import.meta.url)), '../ArtSource');
const rate = 24000;
const bank = djMakeBank(rate);
for (const [id, style, tempo, swing, seed] of [
  ['Saltline', 'cool', 96, .61, 225309], ['PumpHouse', 'bebop', 126, .67, 84371],
  ['NightFurrow', 'modal', 78, .56, 161980]
]) {
  const synth = new DJRenderer(rate, bank, { style, tempo, swing, seed });
  const frames = Math.round(rate * 16 * 4 * 60 / tempo);
  const pcm = Buffer.alloc(44 + frames * 4);
  pcm.write('RIFF'); pcm.writeUInt32LE(pcm.length - 8, 4); pcm.write('WAVEfmt ', 8);
  pcm.writeUInt32LE(16, 16); pcm.writeUInt16LE(1, 20); pcm.writeUInt16LE(2, 22);
  pcm.writeUInt32LE(rate, 24); pcm.writeUInt32LE(rate * 4, 28); pcm.writeUInt16LE(4, 32);
  pcm.writeUInt16LE(16, 34); pcm.write('data', 36); pcm.writeUInt32LE(frames * 4, 40);
  for (let pos = 0; pos < frames; pos += 128) {
    const n = Math.min(128, frames - pos);
    const l = new Float32Array(n), r = new Float32Array(n);
    synth.render(l, r);
    for (let i = 0; i < n; i++) {
      const fade = Math.min(1, (pos+i)/160, (frames-pos-i)/900);
      for (const [ch, value] of [l[i], r[i]].entries())
        pcm.writeInt16LE(Math.round(Math.max(-1, Math.min(1, value)) * 24000 * fade), 44 + (pos+i)*4 + ch*2);
    }
  }
  writeFileSync(join(out, 'S_MusicJazz'+id+'.wav'), pcm);
}
