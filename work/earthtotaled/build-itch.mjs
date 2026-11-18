import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {deflateRawSync} from 'node:zlib';
import {createHash} from 'node:crypto';

const root=path.dirname(fileURLToPath(import.meta.url));
const output=process.argv[2]?path.resolve(process.argv[2]):path.resolve(root,'../../outputs/itch-io');
const staging=path.join(root,'itch-build');
const version=JSON.parse(fs.readFileSync(path.join(root,'package.json'),'utf8')).version;
function run(script,args=[]){
  const result=spawnSync(process.execPath,[path.join(root,script),...args],{cwd:root,encoding:'utf8'});
  if(result.error)throw result.error;
  if(result.status!==0)throw new Error(`${script} failed:\n${result.stdout}\n${result.stderr}`);
  return result.stdout.trim();
}
console.log(run('build.mjs',[staging]));
const html=fs.readFileSync(path.join(staging,'EARTHtotaled.html'));
const text=html.toString('utf8');
if(/<(?:script|link)\b[^>]*(?:src|href)=["'](?:https?:|\/|[a-z]:)/i.test(text))throw new Error('External or absolute startup dependency found.');
if(/<script\b[^>]*\bsrc=/i.test(text)||/<link\b[^>]*rel=["']stylesheet/i.test(text))throw new Error('Standalone game still requires an external script or stylesheet.');
fs.writeFileSync(path.join(staging,'index.html'),html);
const checks=run('test.mjs',[path.join(staging,'index.html')])+'\n'+run('wilderness-check.mjs');
console.log(checks.split(/\r?\n/).filter(line=>/checks passed/.test(line)).join('\n'));
const license=fs.readFileSync(path.join(root,'dist/vendor/THREE-LICENSE.txt'));
fs.writeFileSync(path.join(staging,'THREE-LICENSE.txt'),license);

// A deterministic standard ZIP with DEFLATE and UTF-8 paths. No package install
// or platform-specific archive utility is needed for this two-file release.
function crc32(data){let crc=0xffffffff;for(const byte of data){crc^=byte;for(let bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}return (crc^0xffffffff)>>>0;}
function zip(entries){
  const local=[],central=[];let offset=0;
  for(const entry of entries){
    const name=Buffer.from(entry.name,'utf8'),data=entry.data,compressed=deflateRawSync(data,{level:9}),crc=crc32(data);
    const header=Buffer.alloc(30);header.writeUInt32LE(0x04034b50,0);header.writeUInt16LE(20,4);header.writeUInt16LE(0x800,6);header.writeUInt16LE(8,8);header.writeUInt16LE(0x21,12);header.writeUInt32LE(crc,14);header.writeUInt32LE(compressed.length,18);header.writeUInt32LE(data.length,22);header.writeUInt16LE(name.length,26);
    local.push(header,name,compressed);
    const directory=Buffer.alloc(46);directory.writeUInt32LE(0x02014b50,0);directory.writeUInt16LE(20,4);directory.writeUInt16LE(20,6);directory.writeUInt16LE(0x800,8);directory.writeUInt16LE(8,10);directory.writeUInt16LE(0x21,14);directory.writeUInt32LE(crc,16);directory.writeUInt32LE(compressed.length,20);directory.writeUInt32LE(data.length,24);directory.writeUInt16LE(name.length,28);directory.writeUInt32LE(offset,42);
    central.push(directory,name);offset+=header.length+name.length+compressed.length;
  }
  const size=central.reduce((n,b)=>n+b.length,0),end=Buffer.alloc(22);end.writeUInt32LE(0x06054b50,0);end.writeUInt16LE(entries.length,8);end.writeUInt16LE(entries.length,10);end.writeUInt32LE(size,12);end.writeUInt32LE(offset,16);
  return Buffer.concat([...local,...central,end]);
}
const entries=[{name:'index.html',data:html},{name:'THREE-LICENSE.txt',data:license}];
const archive=zip(entries),filename=`EARTHtotaled-itch-v${version}.zip`;
fs.mkdirSync(output,{recursive:true});fs.mkdirSync(path.join(output,'media'),{recursive:true});fs.writeFileSync(path.join(output,filename),archive);
fs.writeFileSync(path.join(output,'BUILD-REPORT.txt'),[
  `EARTHtotaled v${version} — itch.io HTML5 upload`,
  `Upload file: ${filename}`,
  `ZIP size: ${archive.length} bytes`,
  `ZIP SHA-256: ${createHash('sha256').update(archive).digest('hex')}`,
  ...entries.map(entry=>`${entry.name}: ${entry.data.length} bytes; SHA-256 ${createHash('sha256').update(entry.data).digest('hex')}`),
  '',checks,'',
  'ZIP contains only the browser entry point and the Three.js license.',
  'All game code, styles, sounds and rendering assets are included locally.',
  'The source archive is not the browser upload.',
  'Browser/iframe checks and any limitations are recorded separately in QA-NOTES.md.',
  'No itch.io upload or publication was performed by this build.'
].join('\n')+'\n');
console.log(`Ready: ${path.join(output,filename)} (${(archive.length/1024).toFixed(0)} KB)`);
