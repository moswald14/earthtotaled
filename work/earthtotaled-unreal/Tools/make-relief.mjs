// Original tileable surfaces. Normals come from the same relief field as the pigment.
import fs from 'node:fs';
import zlib from 'node:zlib';
import {fileURLToPath} from 'node:url';
const out=fileURLToPath(new URL('../ArtSource/',import.meta.url)), tau=2*Math.PI;
const ultra=new Set(['Ground','Bark','Concrete','Asphalt','Steel','Carapace','AlloyHull']);
const crcTable=Array.from({length:256},(_,i)=>{for(let j=0;j<8;j++)i=(i>>>1)^((i&1)?0xedb88320:0);return i>>>0;});
function chunk(type,data) {
    const name=Buffer.from(type), b=Buffer.alloc(12+data.length); b.writeUInt32BE(data.length);name.copy(b,4);data.copy(b,8);
    let crc=0xffffffff;for(const x of b.subarray(4,-4))crc=crcTable[(crc^x)&255]^(crc>>>8);b.writeUInt32BE((crc^0xffffffff)>>>0,b.length-4);return b;
}
function png(name,data,size) {
    const header=Buffer.alloc(13);header.writeUInt32BE(size);header.writeUInt32BE(size,4);header[8]=8;header[9]=2;
    fs.writeFileSync(out+'T_Relief'+name+'.png',Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',header),chunk('IDAT',zlib.deflateSync(data,{level:3})),chunk('IEND',Buffer.alloc(0))]));
}
let seed=517231;
function lattice(n,rows=n) {
    const grid=new Float32Array(n*rows); for(let i=0;i<grid.length;i++){seed=(Math.imul(seed,1664525)+1013904223)>>>0;grid[i]=seed/4294967296;}
    return (u,v)=>{u*=n;v*=rows;const x=Math.floor(u),y=Math.floor(v), mask=n-1, iy=(y&(rows-1))*n, jy=((y+1)&(rows-1))*n;
        let a=u-x,b=v-y;a=a*a*(3-2*a);b=b*b*(3-2*b);
        const left=grid[iy+(x&mask)]*(1-a)+grid[iy+((x+1)&mask)]*a;
        return left*(1-b)+(grid[jy+(x&mask)]*(1-a)+grid[jy+((x+1)&mask)]*a)*b;};
}
const clamp=x=>Math.max(0,Math.min(1,x));
const palettes={Ground:[[44,39,30],[134,119,87]],Bark:[[31,28,25],[130,104,78]],Concrete:[[73,72,68],[176,174,159]],FaultShale:[[34,38,42],[116,121,123]],FaultIron:[[44,39,35],[174,102,55]],FaultGold:[[42,44,44],[218,174,62]]};
palettes.AlloyHull=[[32,40,45],[160,175,170]]; palettes.Carapace=[[22,27,22],[117,92,56]];
palettes.Pallor=[[62,62,56],[173,172,140]]; palettes.RotCloth=[[28,36,35],[95,111,102]];
palettes.Asphalt=[[22,25,27],[127,124,113]]; palettes.Steel=[[42,48,51],[151,162,161]];
for(const [name,[dark,light]] of Object.entries(palettes)){
    const noise=[4,16,64,256,1024].map(width=>lattice(width));
    if(process.argv.length>2&&!process.argv.slice(2).includes(name)) continue;
    const fine=ultra.has(name),size=fine?4096:2048,height=new Float32Array(size*size);
    const nextSeed=seed, detail=fine?lattice(2048):null;
    const striation=name==='Bark'?lattice(2048,256):name==='Steel'||name==='AlloyHull'?lattice(256,2048):null; seed=nextSeed;
    const stride=size*3+1, color=Buffer.alloc(stride*size), normal=Buffer.alloc(color.length), rough=Buffer.alloc(color.length);
    const mineral=name.startsWith('Fault'), gold=name==='FaultGold', iron=name==='FaultIron';
    for(let y=0;y<size;y++)for(let x=0;x<size;x++){
        const u=x/size,v=y/size, a=noise[0](u,v),b=noise[1](u,v),c=noise[2](u,v),grain=noise[3](u,v),pore=noise[4](u,v);
        let h,t,r,tint=[0,0,0];
        if(name==='Bark'){
            const fissure=Math.pow(.5+.5*Math.cos(tau*(u*19+1.6*a+.45*b+.12*c)),5);
            h=.44*(1-fissure)+b*.18+c*.13+grain*.10+pore*.04; t=.18+.52*h+.25*b-.20*fissure;r=.69+.22*grain;
        }else if(mineral){
            const fold=v*8+u*3+1.55*a+.54*b+.11*c;
            const fracture=Math.abs(Math.sin(tau*fold)),cross=Math.abs(Math.sin(tau*(u*7-v*4+1.7*b+.2*c)));
            const seam=clamp((.30-fracture)*5.5)*clamp((b-.24)*3.4);
            const crystal=clamp((grain-.56)*5.5)*clamp((c-.31)*5);
            const ore=clamp(seam*(.66+.66*crystal)+clamp((.13-cross)*7)*crystal+crystal*clamp((b-.63)*4));
            const flake=Math.floor((.58*c+.42*grain)*9)/9;
            h=.22*a+.17*b+.21*flake+.15*fracture+.09*pore+ore*.16-.16*clamp((.085-cross)*11);
            t=gold||iron?clamp(.08+.16*flake+.10*pore+ore*.98):clamp(.13+.41*flake+.23*fracture+.17*b);
            tint=gold?[ore*30,ore*14,-ore*18]:iron?[ore*24,-ore*6,-ore*12]:[0,2,5];
            r=clamp(.84+.12*pore-(gold?.64:.36)*ore);
        }else if(name==='Carapace'){
            const ridge=Math.abs(Math.sin(tau*(v*6+u*2+.64*a+.22*b)));
            const joint=clamp((.23-ridge)*5);const pit=clamp((.30-grain)*4);
            h=.28*b+.22*c+.20*ridge-.21*joint-.08*pit;t=.22+.35*b+.28*ridge+.12*pore-.32*joint;
            r=.37+.35*joint+.16*grain;tint=[6*c,5*(1-a),-4];
        }else if(name==='Pallor'){
            const stain=clamp((.60-b)*4)*c,crease=clamp((.08-Math.abs(Math.sin(tau*(u*6+v*3+1.4*b))))*9)*grain;
            h=.17*b+.06*grain+.035*pore-.08*crease;t=.53+.26*b+.13*grain-.27*stain;tint=[stain*19,stain*-8,stain*2];r=.62+.16*grain;
        }else if(name==='RotCloth'){
            const warp=.5+.5*Math.cos(tau*(u*180+b*.55)),weft=.5+.5*Math.sin(tau*(v*131+c*.45));
            const dirt=clamp((a*.65+b*.35-.5)*3.8);
            h=.055*warp+.045*weft+.1*grain+.09*b;t=.30+.22*c+.18*grain+.12*warp-.21*dirt;tint=[dirt*25,dirt*9,-8*dirt];r=.83+.1*grain;
        }else if(name==='AlloyHull'){
            const brush=noise[3](u,v*8),scratch=clamp((.052-Math.abs(Math.sin(tau*(v*42+u+1.2*b))))*19)*clamp((a-.4)*4);
            const patina=clamp((.31-b)*5)*(.4+.6*c);
            h=.15*b+.04*brush+.045*grain-.12*scratch+.06*patina;
            t=.23+.29*b+.18*brush+.18*scratch-.16*patina;tint=[patina*42,patina*9,-patina*13];r=.32+.27*grain+.32*patina;
        }else if(name==='Asphalt'){
            const aggregate=clamp((.59*grain+.41*pore-.52)*5),tar=clamp((.62-b)*2.4);
            const split=clamp((.09-Math.abs(Math.sin(tau*(u*3+v+1.8*a+.38*b))))*12)*clamp((c-.27)*3.6);
            h=.12*b+.18*grain+.17*aggregate-.24*split;t=.17+.24*b+.44*aggregate-.19*split-.08*tar;r=.83-.17*tar+.11*pore;
        }else if(name==='Steel'){
            const brushing=noise[3](u,v*8), pits=clamp((.38-b)*5)*clamp((.55-grain)*3);
            const score=clamp((.06-Math.abs(Math.sin(tau*(v*68+u*2+.47*b))))*17)*c;
            h=.07*b+.07*brushing-.13*pits-.07*score;t=.45+.19*b+.2*brushing-.28*pits+.15*score;
            tint=[22*pits,3*pits,-10*pits];r=.31+.17*brushing+.40*pits;
        }else if(name==='Concrete'){
            const pit=clamp((.31-grain)*5),chip=clamp((pore-.79)*5);
            h=.27*b+.16*c+.09*grain-.22*pit+.12*chip;t=.48+.29*b+.12*pore-.33*pit-.14*(1-c);r=.64+.25*grain;
        }else{
            const gravel=clamp((grain-.59)*5.7),moss=clamp((.58*a+.42*b-.45)*4);
            const root=clamp((.055-Math.abs(Math.sin(tau*(u*9+v*4+1.5*a+.35*b))))*17)*clamp((c-.29)*3.8);
            const leaf=clamp((noise[2](u+v,v*3)-.71)*6);
            h=.28*b+.19*c+.21*gravel+.16*root+.10*leaf+.065*pore;
            t=.12+.27*b+.15*c+.32*gravel+.19*root+.16*leaf;tint=[14*leaf-12*moss,11*moss-4*leaf,-13*moss-7*leaf];r=.71+.14*pore+.11*moss-.22*gravel;
        }
        if(fine){
            const micro=detail(u,v)-.5;
            const tooth=striation?striation(u,v)-.5:micro;
            h+=.04*tooth+.021*micro;t+=.11*tooth+.055*micro;r+=.085*micro;
        }
        if(!Number.isFinite(h+t+r)) throw new Error(`Invalid ${name} surface at (${x}, ${y})`);
        height[y*size+x]=h;const i=y*stride+1+x*3; for(let k=0;k<3;k++){color[i+k]=Math.max(0,Math.min(255,Math.round(dark[k]+(light[k]-dark[k])*clamp(t)+tint[k])));rough[i+k]=Math.round(255*clamp(r));}
    }
    const mask=size-1;
    for(let y=0;y<size;y++)for(let x=0;x<size;x++){
        const slope=4.7*size/2048;
        const dx=(height[y*size+((x-1)&mask)]-height[y*size+((x+1)&mask)])*slope;
        const dy=(height[((y+1)&mask)*size+x]-height[((y-1)&mask)*size+x])*slope;
        const m=127.5/Math.hypot(dx,dy,1),i=y*stride+1+x*3; normal[i]=Math.round(127.5+dx*m);normal[i+1]=Math.round(127.5+dy*m);normal[i+2]=Math.round(127.5+m);
    }
    png(name+'_BaseColor',color,size);png(name+'_Normal',normal,size);png(name+'_Roughness',rough,size);
    process.stdout.write(name+'\n');
}

