// Original tileable 2K surfaces. Normals come from the same relief field as the pigment.
import fs from 'node:fs';
import zlib from 'node:zlib';
import {fileURLToPath} from 'node:url';
const out=fileURLToPath(new URL('../ArtSource/',import.meta.url)), size=2048, tau=2*Math.PI;
const crcTable=Array.from({length:256},(_,i)=>{for(let j=0;j<8;j++)i=(i>>>1)^((i&1)?0xedb88320:0);return i>>>0;});
function chunk(type,data) {
    const name=Buffer.from(type), b=Buffer.alloc(12+data.length); b.writeUInt32BE(data.length);name.copy(b,4);data.copy(b,8);
    let crc=0xffffffff;for(const x of b.subarray(4,-4))crc=crcTable[(crc^x)&255]^(crc>>>8);b.writeUInt32BE((crc^0xffffffff)>>>0,b.length-4);return b;
}
function png(name,data) {
    const header=Buffer.alloc(13);header.writeUInt32BE(size);header.writeUInt32BE(size,4);header[8]=8;header[9]=2;
    fs.writeFileSync(out+'T_Relief'+name+'.png',Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',header),chunk('IDAT',zlib.deflateSync(data,{level:3})),chunk('IEND',Buffer.alloc(0))]));
}
let seed=517231;
function lattice(n) {
    const grid=new Float32Array(n*n); for(let i=0;i<grid.length;i++){seed=(Math.imul(seed,1664525)+1013904223)>>>0;grid[i]=seed/4294967296;}
    return (u,v)=>{u*=n;v*=n;const x=Math.floor(u),y=Math.floor(v), mask=n-1, iy=(y&mask)*n, jy=((y+1)&mask)*n;
        let a=u-x,b=v-y;a=a*a*(3-2*a);b=b*b*(3-2*b);
        const left=grid[iy+(x&mask)]*(1-a)+grid[iy+((x+1)&mask)]*a;
        return left*(1-b)+(grid[jy+(x&mask)]*(1-a)+grid[jy+((x+1)&mask)]*a)*b;};
}
const clamp=x=>Math.max(0,Math.min(1,x));
const palettes={Ground:[[44,39,30],[134,119,87]],Bark:[[31,28,25],[130,104,78]],Concrete:[[73,72,68],[176,174,159]],FaultShale:[[34,38,42],[116,121,123]],FaultIron:[[44,39,35],[174,102,55]],FaultGold:[[42,44,44],[218,174,62]]};
for(const [name,[dark,light]] of Object.entries(palettes)){
    const noise=[4,16,64,256,1024].map(lattice), height=new Float32Array(size*size);
    const stride=size*3+1, color=Buffer.alloc(stride*size), normal=Buffer.alloc(color.length), rough=Buffer.alloc(color.length);
    const mineral=name.startsWith('Fault'), gold=name==='FaultGold', iron=name==='FaultIron';
    for(let y=0;y<size;y++)for(let x=0;x<size;x++){
        const u=x/size,v=y/size, a=noise[0](u,v),b=noise[1](u,v),c=noise[2](u,v),grain=noise[3](u,v),pore=noise[4](u,v);
        let h,t,r;
        if(name==='Bark'){
            const fissure=Math.pow(.5+.5*Math.cos(tau*(u*19+1.6*a+.45*b+.12*c)),5);
            h=.44*(1-fissure)+b*.18+c*.13+grain*.10+pore*.04; t=.18+.52*h+.25*b-.20*fissure;r=.69+.22*grain;
        }else if(mineral){
            const bed=.5+.5*Math.sin(tau*(v*12+u*3+1.3*a+.37*b+.09*c));
            const seam=Math.pow(bed,iron?3:9)*clamp((b-.26)*2.6);
            h=.23*bed+.28*b+.15*c+.18*grain+.055*pore+seam*.16;
            t=(gold||iron)?clamp(.08+.22*c+.16*grain+seam*(gold?1.12:.89)):clamp(.22+bed*.21+b*.19+grain*.23);
            r=.83-.51*seam+.09*pore;
        }else if(name==='Concrete'){
            const pit=clamp((.31-grain)*5),chip=clamp((pore-.79)*5);
            h=.27*b+.16*c+.09*grain-.22*pit+.12*chip;t=.48+.29*b+.12*pore-.33*pit-.14*(1-c);r=.64+.25*grain;
        }else{
            const gravel=clamp((grain-.59)*4.5),soil=clamp((b-.36)*2.3);
            h=b*.27+c*.21+gravel*.24+pore*.08;t=.13+.32*b+.23*c+.27*gravel+.10*pore;r=.62+.25*soil+.11*pore;
        }
        height[y*size+x]=h;const i=y*stride+1+x*3; for(let k=0;k<3;k++){color[i+k]=Math.round(dark[k]+(light[k]-dark[k])*clamp(t));rough[i+k]=Math.round(255*clamp(r));}
    }
    const mask=size-1;
    for(let y=0;y<size;y++)for(let x=0;x<size;x++){
        const dx=(height[y*size+((x-1)&mask)]-height[y*size+((x+1)&mask)])*4.7;
        const dy=(height[((y+1)&mask)*size+x]-height[((y-1)&mask)*size+x])*4.7;
        const m=127.5/Math.hypot(dx,dy,1),i=y*stride+1+x*3; normal[i]=Math.round(127.5+dx*m);normal[i+1]=Math.round(127.5+dy*m);normal[i+2]=Math.round(127.5+m);
    }
    png(name+'_BaseColor',color);png(name+'_Normal',normal);png(name+'_Roughness',rough);
    process.stdout.write(name+'\n');
}

