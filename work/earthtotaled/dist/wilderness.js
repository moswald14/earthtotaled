export const WILDERNESS_LIMIT=720;
const GRID_STEP=8;
const GRID_SIZE=WILDERNESS_LIMIT*2/GRID_STEP+1;
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
const lerp=(a,b,t)=>a+(b-a)*t;
const smooth=(a,b,v)=>{const t=clamp((v-a)/(b-a),0,1);return t*t*(3-2*t);};
function generator(seed){let v=seed>>>0;return ()=>{v=(Math.imul(v,1664525)+1013904223)>>>0;return v/4294967296;};}
function hash(seed,x,z){let v=(seed^Math.imul(x,374761393)^Math.imul(z,668265263))>>>0;v=Math.imul(v^(v>>>13),1274126177);return ((v^(v>>>16))>>>0)/4294967296;}
function noise(seed,x,z){const ix=Math.floor(x),iz=Math.floor(z),u=smooth(0,1,x-ix),v=smooth(0,1,z-iz);return lerp(lerp(hash(seed,ix,iz),hash(seed,ix+1,iz),u),lerp(hash(seed,ix,iz+1),hash(seed,ix+1,iz+1),u),v)*2-1;}
function baseHeight(seed,x,z){const edge=Math.max(Math.abs(x),Math.abs(z));if(edge<=200)return 0;const phase=hash(seed,3,7)*6.28,phase2=hash(seed,11,9)*6.28;
  const gully=Math.exp(-Math.pow((z+x*.34-Math.sin(x/150+phase)*55-245)/28,2)) *4.5;
  const hills=18+Math.sin(x/172+phase)*7+Math.cos(z/185+phase2)*6+noise(seed^0x421c,x/180,z/180)*6+noise(seed^0x5b27,x/62,z/62)*1.6-gully;
  return Math.max(.5,hills)*smooth(200,280,edge);
}
function pointSegmentDistance(x,z,a,b){const dx=b.x-a.x,dz=b.z-a.z,l=dx*dx+dz*dz;const t=l?clamp(((x-a.x)*dx+(z-a.z)*dz)/l,0,1):0;return Math.hypot(x-a.x-dx*t,z-a.z-dz*t);}
function trailDistance(plan,x,z){let best=Infinity;for(const trail of plan.trails)for(let i=1;i<trail.points.length;i++)best=Math.min(best,pointSegmentDistance(x,z,trail.points[i-1],trail.points[i]));return best;}
const NORMALS={north:{x:0,z:-1},south:{x:0,z:1},east:{x:1,z:0},west:{x:-1,z:0}};
function localPoint(house,u,v,y=house.floorY){const n=NORMALS[house.front],s={x:n.z,z:-n.x};return {x:house.x+s.x*u-n.x*v,z:house.z+s.z*u-n.z*v,y};}

export function createWildernessPlan(seed){
  seed=Number(seed)>>>0;const random=generator(seed^0x73cb40a5),range=(a,b)=>a+(b-a)*random();
  const plan={seed,limit:WILDERNESS_LIMIT,gridStep:GRID_STEP,gridSize:GRID_SIZE,houses:[],animals:[],traps:[],props:[],trails:[],heights:null};
  const exits=[{x:179,z:-28},{x:-179,z:20},{x:0,z:-179},{x:0,z:179}];
  const ends=[{x:668,z:-32},{x:-665,z:20},{x:0,z:-666},{x:0,z:666}];
  for(let i=0;i<4;i++){const middle={x:lerp(exits[i].x,ends[i].x,.55)+(i<2?0:range(-22,22)),z:lerp(exits[i].z,ends[i].z,.55)+(i<2?range(-22,22):0)};plan.trails.push({id:'outbound-'+i,width:6.5,points:[exits[i],middle,ends[i]]});}
  const styles=['farmhouse','forester','ranger','abandoned'];
  for(let i=0;i<22;i++){
    let x,z;for(let attempt=0;attempt<200;attempt++){
      if(i<4&&attempt===0){const positions=[[range(294,330),range(-52,-8)],[-range(294,330),range(0,44)],[range(-24,24),-range(294,335)],[range(0,42),range(298,340)]];[x,z]=positions[i];}
      else {const angle=(i/22*Math.PI*2)+range(-.11,.11),radius=range(i%3===0?335:420,658);x=Math.cos(angle)*radius;z=Math.sin(angle)*radius;}
      x=Math.round(x/4)*4;z=Math.round(z/4)*4;
      if(Math.max(Math.abs(x),Math.abs(z))>260&&Math.max(Math.abs(x),Math.abs(z))<674&&plan.houses.every(h=>Math.hypot(x-h.x,z-h.z)>68))break;
    }
    const w=[12,14,16][Math.floor(random()*3)],d=[12,14,16][Math.floor(random()*3)];
    const front=Math.abs(x)>Math.abs(z)?(x<0?'east':'west'):(z<0?'south':'north');
    const y=Math.fround(baseHeight(seed,x,z)),floorY=y+.08,wallHeight=3.45+(i%3)*.12;
    const house={id:'wild-house-'+String(i).padStart(2,'0'),index:i,x,z,w,d,width:w,depth:d,y,floorY,roofY:floorY+wallHeight+.16,h:wallHeight,front,style:styles[i%styles.length],rooms:[],loot:[],windows:[]};
    const normal=NORMALS[front],deep=Math.abs(normal.z)?d:w,across=Math.abs(normal.z)?w:d;
    const threshold=localPoint(house,0,-deep/2),outside=localPoint(house,0,-deep/2-3.5),inside=localPoint(house,0,-deep/2+3.5);
    house.door={outside,threshold,inside,normal:{...normal},width:3.6,height:2.65};
    house.interior=localPoint(house,0,-deep*.18);house.across=across;house.deep=deep;
    house.rooms.push({id:house.id+'-living',name:'Front room',center:localPoint(house,0,-deep*.23),door:house.door});
    for(const sign of [-1,1]){
      const center=localPoint(house,sign*across*.25,deep*.23),doorCenter=localPoint(house,sign*across*.25,.3);
      const room={id:house.id+(sign<0?'-bedroom':'-store'),name:sign<0?'Bedroom':'Store room',center,door:{outside:localPoint(house,sign*across*.25,-1.3),threshold:doorCenter,inside:localPoint(house,sign*across*.25,1.9),width:2.8,height:2.6}};
      house.rooms.push(room);
      const at=localPoint(house,sign*across*.16,deep*.31);
      house.loot.push({id:house.id+(sign<0?'-medical':'-pantry'),...at,name:sign<0?'Search survivor medical chest':'Search sealed pantry supplies',water:sign>0?2:1,scrap:10+Math.floor(random()*20),meds:sign<0&&random()>.35?1:0,ammo:18+Math.floor(random()*22)});
    }
    plan.houses.push(house);
    // A worn spur joins the nearest city route and ends square to the open door.
    const main=Math.abs(x)>Math.abs(z)?(x<0?1:0):(z<0?2:3),t=plan.trails[main];
    const junction=main<2?{x,z:t.points[1].z}:{x:t.points[1].x,z};
    const approach=localPoint(house,0,-deep/2-9);
    plan.trails.push({id:house.id+'-spur',width:3.2,points:[junction,{x:approach.x,z:approach.z},{x:outside.x,z:outside.z}]});
  }
  // Terrain samples include wide flat house pads, smoothly feathered into hills.
  const heights=new Float32Array(GRID_SIZE*GRID_SIZE);
  for(let iz=0;iz<GRID_SIZE;iz++)for(let ix=0;ix<GRID_SIZE;ix++){
    const x=-WILDERNESS_LIMIT+ix*GRID_STEP,z=-WILDERNESS_LIMIT+iz*GRID_STEP;let h=baseHeight(seed,x,z);
    for(const house of plan.houses){const dx=Math.max(0,Math.abs(x-house.x)-house.w/2-10),dz=Math.max(0,Math.abs(z-house.z)-house.d/2-10),distance=Math.hypot(dx,dz);if(distance<36)h=lerp(h,house.y,1-smooth(0,36,distance));}
    heights[iz*GRID_SIZE+ix]=h;
  }
  plan.heights=heights;
  const farFromHomes=(x,z,margin=24)=>plan.houses.every(h=>Math.hypot(Math.max(0,Math.abs(x-h.x)-h.w/2),Math.max(0,Math.abs(z-h.z)-h.d/2))>margin);
  function ruralPoint(margin=24){for(let attempt=0;attempt<10000;attempt++){const x=range(-675,675),z=range(-675,675);if(Math.max(Math.abs(x),Math.abs(z))>238&&farFromHomes(x,z,margin))return {x,z};}return {x:680,z:680};}
  for(let i=0;i<24;i++){const p=ruralPoint(32);plan.animals.push({id:'wildlife-'+String(i).padStart(2,'0'),species:i%6===0?'wolf':i%3===0?'boar':'deer',...p,heading:range(0,Math.PI*2),range:range(35,70)});}
  for(let i=0;i<30;i++){const p=ruralPoint(26);plan.traps.push({id:'snare-'+String(i).padStart(2,'0'),...p,y:terrainHeight(plan,p.x,p.z),heading:range(0,Math.PI*2),kind:i%3===0?'jaw':'snare'});}
  const clearOfLife=(x,z,r)=>plan.animals.every(a=>Math.hypot(x-a.x,z-a.z)>r)&&plan.traps.every(t=>Math.hypot(x-t.x,z-t.z)>r);
  for(let i=0;i<1950;i++){
    const p=ruralPoint(9);if(trailDistance(plan,p.x,p.z)<6||!clearOfLife(p.x,p.z,4))continue;
    const moisture=noise(seed^0x6b1,p.x/190,p.z/190),type=i%8===0?'boulder':moisture>.08?'pine':'ashTree';
    plan.props.push({id:'wild-prop-'+i,type,...p,y:terrainHeight(plan,p.x,p.z),height:type==='boulder'?range(.5,2):type==='pine'?range(4,9):range(3.5,8),scale:range(.7,1.35),heading:range(0,Math.PI*2),solid:plan.props.filter(v=>v.solid).length<220&&i%6===0});
  }
  return plan;
}

// A tile has stable global terrain samples, even while its scenery is unloaded.
export function createEndlessPlan(worldSeed,cx,cz){
 if(cx===0&&cz===0)return createWildernessPlan(worldSeed);
 const seed=(worldSeed^Math.imul(cx,73856093)^Math.imul(cz,19349663))>>>0,plan=createWildernessPlan(seed),ox=cx*1440,oz=cz*1440,prefix=`region:${cx}:${cz}:`;
 for(const h of plan.houses){const delta=baseHeight(worldSeed,h.x+ox,h.z+oz)-h.y;h.id=prefix+h.id;h.y+=delta;h.floorY+=delta;h.roofY+=delta;
 const points=[h.interior,...Object.values(h.door).filter(p=>p&&Number.isFinite(p.y)),...h.rooms.flatMap(r=>[r.center,...Object.values(r.door).filter(p=>p&&Number.isFinite(p.y))]),...h.loot];const seen=new Set();for(const p of points)if(!seen.has(p)){seen.add(p);p.y+=delta;}for(const l of h.loot)l.id=prefix+l.id;}
 for(let iz=0;iz<plan.gridSize;iz++)for(let ix=0;ix<plan.gridSize;ix++){const x=-720+ix*8,z=-720+iz*8;let y=baseHeight(worldSeed,x+ox,z+oz);for(const h of plan.houses){const d=Math.hypot(Math.max(0,Math.abs(x-h.x)-h.w/2-10),Math.max(0,Math.abs(z-h.z)-h.d/2-10));if(d<36)y=lerp(y,h.y,1-smooth(0,36,d));}plan.heights[iz*plan.gridSize+ix]=y;}
 for(const list of [plan.props,plan.traps,plan.animals])for(const p of list){p.id=prefix+p.id;p.y=terrainHeight(plan,p.x,p.z);}plan.originX=ox;plan.originZ=oz;return plan;
}

// Matches the exact diagonal used by createWilderness's terrain triangles.
export function terrainHeight(plan,x,z){
  if(!plan?.heights)return 0;const n=plan.gridSize||GRID_SIZE,step=plan.gridStep||GRID_STEP,limit=plan.limit||WILDERNESS_LIMIT;
  const gx=clamp((x+limit)/step,0,n-1),gz=clamp((z+limit)/step,0,n-1),ix=Math.min(n-2,Math.floor(gx)),iz=Math.min(n-2,Math.floor(gz)),u=gx-ix,v=gz-iz;
  const a=plan.heights[iz*n+ix],b=plan.heights[iz*n+ix+1],c=plan.heights[(iz+1)*n+ix],d=plan.heights[(iz+1)*n+ix+1];
  return u+v<=1?a+(b-a)*u+(c-a)*v:d+(c-d)*(1-u)+(b-d)*(1-v);
}

export function createWilderness({THREE,scene,mats,box,cylinder,blocker,addSign,addInteractable,plan}){
  const root=new THREE.Group();root.name='Procedural wilderness / seed '+plan.seed;scene.add(root);
  const buildings=[],platforms=[],staticBatches=new Map(),dummy=new THREE.Object3D();
  const random=generator(plan.seed^0x414f35),range=(a,b)=>a+(b-a)*random();
  const makeMat=(color,roughness=.95,metalness=0)=>new THREE.MeshStandardMaterial({color,roughness,metalness});
  const plaster=[makeMat(0xa7a58d),makeMat(0x8d9685),makeMat(0x8f7e68),makeMat(0x6b7163)],wood=makeMat(0x635c49),floorMat=makeMat(0x7c7965),roofMat=makeMat(0x515f54),metal=makeMat(0x66776a,.75,.25),ashWood=makeMat(0x363a33),pineBark=makeMat(0x514d3e),pineNeedles=makeMat(0x3f5135),pineLight=makeMat(0x586143),rockMat=makeMat(0x77766b),fabric=makeMat(0x6c7660),paperMat=makeMat(0xb4ad91),dark=makeMat(0x29372d),deadGrass=makeMat(0x797057),dryShrub=makeMat(0x625d48),charred=makeMat(0x34382f);
  deadGrass.side=THREE.DoubleSide;
  function remember(mesh,w,h,d){if(!mesh)return mesh;root.add(mesh);mesh.updateMatrixWorld(true);let rows=staticBatches.get(mesh.material);if(!rows){rows=[];staticBatches.set(mesh.material,rows);}rows.push({mesh,w,h,d});mesh.visible=false;return mesh;}
  function solid(x,z,w,d,h,y,mat){return remember(blocker(x,z,w,d,h,y,mat),w,h,d);}
  function collisionOnly(x,z,w,d,h,y,mat){const mesh=blocker(x,z,w,d,h,y,mat);if(mesh){root.add(mesh);mesh.visible=false;}return mesh;}
  function detail(w,h,d,x,y,z,mat,rotation=0){const mesh=box(w,h,d,x,y,z,mat,root);mesh.rotation.y=rotation;return remember(mesh,w,h,d);}
  function platform(id,x,z,w,d,y,mat=floorMat){solid(x,z,w,d,.16,y-.08,mat);const p={id,x,z,w,d,y};platforms.push(p);return p;}
  function texture(){const c=document.createElement('canvas');c.width=c.height=512;const ctx=c.getContext('2d');ctx.fillStyle='#aaa08b';ctx.fillRect(0,0,512,512);
    for(let i=0;i<135;i++){ctx.fillStyle=i%3===0?'#4c504734':'#71634d25';ctx.beginPath();ctx.ellipse(range(0,512),range(0,512),range(9,48),range(5,29),range(0,6.28),0,Math.PI*2);ctx.fill();}
    for(let i=0;i<28000;i++){const v=Math.floor(range(30,190));ctx.fillStyle=`rgba(${v+7},${v+2},${v-7},${range(.12,.34)})`;ctx.fillRect(range(0,512),range(0,512),range(.4,2.4),range(.4,2));}
    for(let i=0;i<23;i++){let x=range(0,512),y=range(0,512);ctx.strokeStyle=i%4?'#3c413879':'#2f352f9a';ctx.lineWidth=range(.4,1.5);ctx.beginPath();ctx.moveTo(x,y);for(let j=0;j<9;j++){x+=range(-8,8);y+=range(3,16);ctx.lineTo(x,y);if(j===4){ctx.lineTo(x+range(-15,15),y+9);ctx.moveTo(x,y);}}ctx.stroke();}
    const tex=new THREE.CanvasTexture(c);tex.wrapS=tex.wrapT=THREE.RepeatWrapping;tex.colorSpace=THREE.SRGBColorSpace;tex.anisotropy=4;return tex;}
  const groundMap=texture(),n=plan.gridSize,positions=new Float32Array(n*n*3),colors=new Float32Array(n*n*3),uv=new Float32Array(n*n*2),indices=new Uint32Array((n-1)*(n-1)*6);const ashColor=new THREE.Color(0x8a8070),greenColor=new THREE.Color(0x737352),trailColor=new THREE.Color(0x9f9278),mixColor=new THREE.Color();
  for(let iz=0;iz<n;iz++)for(let ix=0;ix<n;ix++){const index=iz*n+ix,x=-plan.limit+ix*plan.gridStep,z=-plan.limit+iz*plan.gridStep;positions[index*3]=x;positions[index*3+1]=plan.heights[index];positions[index*3+2]=z;uv[index*2]=x/11;uv[index*2+1]=z/11;const fertility=smooth(-.3,.5,noise(plan.seed^0x6b1,x/190,z/190));mixColor.copy(ashColor).lerp(greenColor,fertility);const road=1-smooth(2,8,trailDistance(plan,x,z));mixColor.lerp(trailColor,road*.7);const shade=.94+hash(plan.seed,ix,iz)*.1;colors[index*3]=mixColor.r*shade;colors[index*3+1]=mixColor.g*shade;colors[index*3+2]=mixColor.b*shade;}
  let at=0;for(let iz=0;iz<n-1;iz++)for(let ix=0;ix<n-1;ix++){const a=iz*n+ix,b=a+1,c=a+n,d=c+1;indices.set([a,c,b,b,c,d],at);at+=6;}
  const geometry=new THREE.BufferGeometry();geometry.setAttribute('position',new THREE.BufferAttribute(positions,3));geometry.setAttribute('color',new THREE.BufferAttribute(colors,3));geometry.setAttribute('uv',new THREE.BufferAttribute(uv,2));geometry.setIndex(new THREE.BufferAttribute(indices,1));geometry.computeVertexNormals();geometry.computeBoundingSphere();
  const terrainMesh=new THREE.Mesh(geometry,new THREE.MeshStandardMaterial({map:groundMap,vertexColors:true,roughness:1,metalness:0,polygonOffset:true,polygonOffsetFactor:1,polygonOffsetUnits:1}));terrainMesh.name='wilderness-terrain';terrainMesh.receiveShadow=true;root.add(terrainMesh);
  // Trail ribbons follow the same sampled ground surface and give clear routes home.
  const trailPositions=[],trailIndices=[];for(const trail of plan.trails){for(let p=1;p<trail.points.length;p++){const a=trail.points[p-1],b=trail.points[p],length=Math.hypot(b.x-a.x,b.z-a.z);if(length<.01)continue;const nx=-(b.z-a.z)/length,nz=(b.x-a.x)/length,steps=Math.max(1,Math.ceil(length/4));for(let k=0;k<=steps;k++){const t=k/steps,x=lerp(a.x,b.x,t),z=lerp(a.z,b.z,t);for(const side of [-1,1]){const xx=x+nx*trail.width/2*side,zz=z+nz*trail.width/2*side;trailPositions.push(xx,terrainHeight(plan,xx,zz)+.027,zz);}if(k<steps){const q=trailPositions.length/3-2;trailIndices.push(q,q+2,q+1,q+1,q+2,q+3);}}}}
  const trailGeo=new THREE.BufferGeometry();trailGeo.setAttribute('position',new THREE.Float32BufferAttribute(trailPositions,3));trailGeo.setIndex(trailIndices);trailGeo.computeVertexNormals();const trailMesh=new THREE.Mesh(trailGeo,new THREE.MeshStandardMaterial({color:0x817d61,roughness:1,transparent:true,opacity:.34,depthWrite:false,polygonOffset:true,polygonOffsetFactor:-1}));trailMesh.receiveShadow=true;root.add(trailMesh);

  function houseBox(house,u,v,lw,ld,height,offset,mat,collision=true){const p=localPoint(house,u,v),n=NORMALS[house.front],w=Math.abs(n.z)?lw:ld,d=Math.abs(n.z)?ld:lw;return collision?solid(p.x,p.z,w,d,height,house.floorY+offset+height/2,mat):detail(w,height,d,p.x,house.floorY+offset+height/2,p.z,mat);}
  function wall(house,side,length,openings,mat){
    const normal=NORMALS[side],tangent={x:normal.z,z:-normal.x},depth=Math.abs(normal.z)?house.d:house.w,cx=house.x+normal.x*depth/2,cz=house.z+normal.z*depth/2,thickness=.27,height=house.h;
    const add=(start,end,low,high)=>{if(end-start<.025||high-low<.025)return;const u=(start+end)/2,x=cx+tangent.x*u,z=cz+tangent.z*u;solid(x,z,Math.abs(normal.z)?end-start:thickness,Math.abs(normal.z)?thickness:end-start,high-low,house.floorY+(low+high)/2,mat);};
    let edge=-length/2;for(const opening of openings.sort((a,b)=>a.u-b.u)){const left=opening.u-opening.w/2,right=opening.u+opening.w/2;add(edge,left,0,height);add(left,right,0,opening.low);add(left,right,opening.high,height);edge=right;
      if(opening.window){const center={x:cx+tangent.x*opening.u,z:cz+tangent.z*opening.u,y:house.floorY+(opening.low+opening.high)/2};house.windows.push({center,normal:{...normal},width:opening.w,bottom:house.floorY+opening.low,top:house.floorY+opening.high});for(const u of [left,right])detail(Math.abs(normal.z)?.09:.34,opening.high-opening.low+.15,Math.abs(normal.z)?.34:.09,cx+tangent.x*u,center.y,cz+tangent.z*u,wood);for(const yy of [opening.low,opening.high])detail(Math.abs(normal.z)?opening.w+.18:.36,.09,Math.abs(normal.z)?.36:opening.w+.18,center.x,house.floorY+yy,center.z,charred);detail(Math.abs(normal.z)?opening.w+.3:.48,.12,Math.abs(normal.z)?.48:opening.w+.3,center.x+normal.x*.09,house.floorY+opening.low-.11,center.z+normal.z*.09,dark);}
    }add(edge,length/2,0,height);
  }
  function partition(house){const a=house.across,deep=house.deep,door=2.8,centers=[-a*.25,a*.25],segments=[[-a/2,centers[0]-door/2],[centers[0]+door/2,centers[1]-door/2],[centers[1]+door/2,a/2]];for(const [lo,hi] of segments)houseBox(house,(lo+hi)/2,.3,hi-lo,.18,house.h,0,plaster[1]);for(const u of centers)houseBox(house,u,.3,door,.18,house.h-2.6,2.6,plaster[1]);houseBox(house,0,(deep/2+.3)/2,.18,deep/2-.3,house.h,0,plaster[1]);}
  function loot(house,record){const group=new THREE.Group();group.position.set(record.x,record.y,record.z);root.add(group);box(.95,.62,.68,0,.31,0,mats.metal,group);box(1.03,.1,.75,0,.67,0,wood,group);const strip=box(.6,.06,.025,0,.43,.35,mats.light,group);return addInteractable('wildernessLoot',record.x,record.z,{...record,group,strip,houseId:house.id});}
  for(const source of plan.houses){
    const house={...source,windows:[]},mat=plaster[house.index%plaster.length],doorWidth=house.door.width;
    for(const side of ['north','south','east','west']){const length=side==='north'||side==='south'?house.w:house.d;const openings=side===house.front?[{u:-length*.34,w:1.8,low:1.05,high:2.42,window:true},{u:0,w:doorWidth,low:0,high:2.65},{u:length*.34,w:1.8,low:1.05,high:2.42,window:true}]:[{u:-length*.25,w:2,low:1.05,high:2.42,window:true},{u:length*.25,w:2,low:1.05,high:2.42,window:true}];wall(house,side,length,openings,mat);}
    platform(house.id+'-floor',house.x,house.z,house.w-.04,house.d-.04,house.floorY,floorMat);platform(house.id+'-roof',house.x,house.z,house.w+.65,house.d+.65,house.roofY,roofMat);partition(house);
    const deep=house.deep,across=house.across;
    // Furnished living space plus two accessible rooms behind real partition doors.
    houseBox(house,-across*.29,-deep*.23,2.1,1.15,.78,0,wood);houseBox(house,across*.29,-deep*.23,2.7,1.1,.68,0,fabric);houseBox(house,across*.29,-deep*.28,2.7,.18,.4,.68,fabric);
    for(const sign of [-1,1]){houseBox(house,sign*across*.36,deep*.29,1.45,2.6,.55,0,wood);houseBox(house,sign*across*.36,deep*.29,1.38,2.5,.16,.55,fabric);houseBox(house,sign*across*.36,deep*.36,1.25,.55,.15,.71,paperMat);}
    // Counter and stove sit beside the front wall, away from the central doorway.
    houseBox(house,-across*.37,-deep*.4,1.7,.8,.9,0,metal);houseBox(house,-across*.37,-deep*.4,1.75,.82,.06,.9,dark);
    for(let i=0;i<7;i++){const p=localPoint(house,range(-across*.2,across*.2),range(-deep*.32,-deep*.09));detail(.22,.01,.32,p.x,house.floorY+.012,p.z,paperMat,range(0,6.28));}
    const threshold=house.door.threshold,normal=house.door.normal;detail(Math.abs(normal.z)?4.4:2,.1,Math.abs(normal.z)?2:4.4,threshold.x+normal.x*.8,house.y+.04,threshold.z+normal.z*.8,mats.concrete);
    // A chimney and patched roof silhouette replace the repeated featureless cubes.
    solid(house.x-house.w*.32,house.z-house.d*.32,.75,.75,1.5,house.roofY+.75,mats.concrete);detail(.95,.15,.95,house.x-house.w*.32,house.roofY+1.55,house.z-house.d*.32,metal);
    for(let i=0;i<4;i++)detail(range(1,2.6),.025,range(1,2.4),house.x+range(-house.w*.3,house.w*.3),house.roofY+.015,house.z+range(-house.d*.3,house.d*.3),house.index%2?wood:metal,range(-.2,.2));
    // Broken upper roof skins, blackened rafters and peeling lower cladding are
    // cosmetic only: the tested doorway, window gaps and roof slab stay intact.
    for(const side of ['north','south','east','west']){const n=NORMALS[side],t={x:n.z,z:-n.x},length=n.z?house.w:house.d,depth=n.z?house.d:house.w,cx=house.x+n.x*(depth/2+.16),cz=house.z+n.z*(depth/2+.16);
      detail(n.z?length+.12:.08,.19,n.z?.08:length+.12,cx,house.floorY+house.h-.2,cz,charred);
      for(const sign of [-1,1]){const u=sign*length*.36;for(let j=0;j<3;j++){detail(n.z?range(1.5,2.5):.025,.08,n.z?.025:range(1.5,2.5),cx+t.x*u,house.floorY+.24+j*.18,cz+t.z*u,house.index%2?wood:charred);}}
    }
    for(let r=0;r<3;r++){const z=house.z-house.d*.31+r*.8;for(const side of [-1,1]){const beam=detail(house.w*.42,.13,.12,house.x+side*house.w*.2,house.roofY+.65,z,charred);beam.rotation.z=side*-.24;}}
    const bentSheet=detail(house.w*.38,.045,house.d*.31,house.x+house.w*.19,house.roofY+.45,house.z-house.d*.23,metal,.13);bentSheet.rotation.z=-.18;
    for(let i=0;i<3;i++){const board=detail(.15,range(1.6,2.5),.22,house.x+house.w/2+.24+i*.13,house.floorY+.9,house.z-house.d*.4,wood);board.rotation.z=range(-.22,-.07);}
    const name=['HOMESTEAD','FORESTER CABIN','RANGER HOUSE','EVACUATED HOME'][house.index%4]+' / '+String(house.index+1).padStart(2,'0');const sign=addSign(name,threshold.x+normal.x*.21,house.floorY+2.95,threshold.z+normal.z*.21,Math.min(5.5,across*.65),.42,'#d3d7b1');sign.rotation.y=Math.atan2(normal.x,normal.z);root.add(sign);
    for(const record of house.loot)loot(house,record);buildings.push(house);
  }

  // Forest props use a few instanced draw calls, not thousands of tiny meshes.
  const ashTrunks=[],pineTrunks=[],branches=[],leaves=[],leafTips=[],rocks=[],grass=[],shrubs=[],fallenLogs=[];
  for(const p of plan.props){if(p.type==='boulder'){rocks.push([p.x,p.y+p.height*.42,p.z,p.scale*1.3,p.height*.7,p.scale,p.heading]);if(p.solid)collisionOnly(p.x,p.z,p.scale*1.7,p.scale*1.5,p.height*.85,p.y+p.height*.425,rockMat);continue;}
    const trunkScale=p.type==='pine'?.23:.16,trunks=p.type==='pine'?pineTrunks:ashTrunks;trunks.push([p.x,p.y+p.height/2,p.z,trunkScale*p.scale,p.height,trunkScale*p.scale,p.heading]);
    if(p.solid)collisionOnly(p.x,p.z,.48*p.scale,.48*p.scale,p.height,p.y+p.height/2,p.type==='pine'?pineBark:ashWood);
    if(p.type==='pine'){for(let j=0;j<4;j++){const a=p.heading+j*2.4,off=p.height*(.065-j*.009),radius=p.height*(.25-j*.039);const cluster=[p.x+Math.cos(a)*off,p.y+p.height*(.48+j*.12),p.z+Math.sin(a)*off,radius,p.height*(.18-j*.012),radius*range(.75,1.08),a];(j%2?leafTips:leaves).push(cluster);}for(let j=0;j<3;j++){const a=p.heading+j*2.1;branches.push([p.x+Math.cos(a)*p.height*.1,p.y+p.height*.55,p.z+Math.sin(a)*p.height*.1,.065,p.height*.28,.065,a,.72]);}}
    else{for(let j=0;j<4;j++){const side=j%2?1:-1,a=p.heading+j*2.4;branches.push([p.x+Math.cos(a)*p.height*.11,p.y+p.height*(.39+j*.13),p.z+Math.sin(a)*p.height*.11,.07*p.scale,p.height*(.3-j*.027),.07*p.scale,a,side*.78]);}if(Number(p.id.split('-').pop())%13===0){const length=range(2.8,5.5),a=p.heading;const ax=p.x+Math.cos(a)*1.5,az=p.z+Math.sin(a)*1.5,bx=ax+Math.cos(a)*length,bz=az+Math.sin(a)*length;fallenLogs.push({a:new THREE.Vector3(ax,terrainHeight(plan,ax,az)+.17,az),b:new THREE.Vector3(bx,terrainHeight(plan,bx,bz)+.17,bz),radius:range(.13,.24)});}}
  }
  function grassClump(x,z,scale=1){const y=terrainHeight(plan,x,z);for(let j=0;j<4;j++){const a=j*1.57+range(-.3,.3),height=range(.18,.55)*scale;grass.push([x+Math.cos(a)*.15,y+height/2,z+Math.sin(a)*.15,range(.05,.13)*scale,height,1,a,range(-.22,.22)]);}if(random()>.87){shrubs.push([x,y+.22*scale,z,range(.3,.65)*scale,range(.18,.36)*scale,range(.3,.65)*scale,range(0,6.28)]);}}
  for(let i=0;i<6500;i++){const x=range(-700,700),z=range(-700,700);if(Math.max(Math.abs(x),Math.abs(z))<216||plan.houses.some(h=>Math.abs(x-h.x)<h.w/2+1&&Math.abs(z-h.z)<h.d/2+1)||trailDistance(plan,x,z)<1.3)continue;grassClump(x,z,range(.65,1.3));}
  for(const h of plan.houses)for(let i=0;i<180;i++){const x=h.x+range(-h.w/2-24,h.w/2+24),z=h.z+range(-h.d/2-24,h.d/2+24);if(Math.abs(x-h.x)<h.w/2+.8&&Math.abs(z-h.z)<h.d/2+.8)continue;if(pointSegmentDistance(x,z,h.door.threshold,h.door.outside)<1.5)continue;grassClump(x,z,range(.65,1.25));}
  function instanceShape(rows,geometry,material,cast=true){if(!rows.length)return null;const mesh=new THREE.InstancedMesh(geometry,material,rows.length);mesh.castShadow=cast;mesh.receiveShadow=true;for(let i=0;i<rows.length;i++){const p=rows[i];dummy.position.set(p[0],p[1],p[2]);dummy.scale.set(p[3],p[4],p[5]);dummy.rotation.set(0,p[6]||0,p[7]||0);dummy.updateMatrix();mesh.setMatrixAt(i,dummy.matrix);}root.add(mesh);return mesh;}
  instanceShape(ashTrunks,new THREE.CylinderGeometry(.7,1,1,7),ashWood);instanceShape(pineTrunks,new THREE.CylinderGeometry(.6,1,1,7),pineBark);instanceShape(branches,new THREE.CylinderGeometry(.35,1,1,5),ashWood);instanceShape(leaves,new THREE.IcosahedronGeometry(1,1),pineNeedles);instanceShape(leafTips,new THREE.IcosahedronGeometry(1,1),pineLight);instanceShape(rocks,new THREE.IcosahedronGeometry(1,1),rockMat);instanceShape(grass,new THREE.PlaneGeometry(1,1),deadGrass,false);instanceShape(shrubs,new THREE.IcosahedronGeometry(1,0),dryShrub,false);
  if(fallenLogs.length){const logs=new THREE.InstancedMesh(new THREE.CylinderGeometry(.72,1,1,7),ashWood,fallenLogs.length),axis=new THREE.Vector3(0,1,0),direction=new THREE.Vector3();logs.castShadow=true;logs.receiveShadow=true;fallenLogs.forEach((log,i)=>{direction.subVectors(log.b,log.a);dummy.position.copy(log.a).add(log.b).multiplyScalar(.5);dummy.quaternion.setFromUnitVectors(axis,direction.clone().normalize());dummy.scale.set(log.radius,direction.length(),log.radius);dummy.updateMatrix();logs.setMatrixAt(i,dummy.matrix);});root.add(logs);}
  const smokeCanvas=document.createElement('canvas');smokeCanvas.width=128;smokeCanvas.height=256;const smokeCtx=smokeCanvas.getContext('2d');smokeCtx.clearRect(0,0,128,256);smokeCtx.filter='blur(8px)';for(let i=0;i<38;i++){const y=range(8,240),width=lerp(8,39,1-y/256);smokeCtx.fillStyle=`rgba(74,77,65,${range(.02,.055)})`;smokeCtx.beginPath();smokeCtx.ellipse(64+Math.sin(y*.035)*12,y,width,range(10,28),0,0,Math.PI*2);smokeCtx.fill();}const smokeMap=new THREE.CanvasTexture(smokeCanvas);smokeMap.colorSpace=THREE.SRGBColorSpace;const smokeMaterial=new THREE.SpriteMaterial({map:smokeMap,color:0xb4b1a4,transparent:true,opacity:.5,depthWrite:false,fog:true});for(const h of plan.houses.filter(h=>h.index%5===0)){const smoke=new THREE.Sprite(smokeMaterial);smoke.position.set(h.x-h.w*.28,h.roofY+9,h.z-h.d*.28);smoke.scale.set(7,19,1);root.add(smoke);}
  // Low distant haze and hanging seed/ash motes stay entirely cosmetic.
  const dustPositions=new Float32Array(550*3);for(let i=0;i<550;i++){const x=range(-700,700),z=range(-700,700);dustPositions[i*3]=x;dustPositions[i*3+1]=terrainHeight(plan,x,z)+range(1,12);dustPositions[i*3+2]=z;}const dustGeometry=new THREE.BufferGeometry();dustGeometry.setAttribute('position',new THREE.BufferAttribute(dustPositions,3));const dust=new THREE.Points(dustGeometry,new THREE.PointsMaterial({color:0xd6d5b8,size:.1,transparent:true,opacity:.25,depthWrite:false}));root.add(dust);
  for(const [index,trail] of plan.trails.slice(0,4).entries()){const p=trail.points[1],y=terrainHeight(plan,p.x,p.z);detail(.1,3,.1,p.x+4,y+1.5,p.z,wood);const sign=addSign(['ASHWOOD / HOMESTEADS','CANAL WILDS / HOUSES','NORTH RIDGE / CABINS','REGROWTH / FARMLAND'][index],p.x+4,y+2.6,p.z,6.5,.55,'#d0d4ae');root.add(sign);}
  // Render batched equivalents of static boxes. The original hidden blockers
  // remain in the parent's explicit shot-target list and under this owned root.
  let staticInstances=0;for(const [material,rows] of staticBatches){const mesh=new THREE.InstancedMesh(new THREE.BoxGeometry(1,1,1),material,rows.length);mesh.castShadow=true;mesh.receiveShadow=true;for(let i=0;i<rows.length;i++){const row=rows[i];dummy.position.copy(row.mesh.position);dummy.rotation.copy(row.mesh.rotation);dummy.scale.set(row.w,row.h,row.d);dummy.updateMatrix();mesh.setMatrixAt(i,dummy.matrix);}root.add(mesh);staticInstances+=rows.length;}
  root.updateMatrixWorld(true);
  return {root,buildings,houses:buildings,platforms,terrainMesh,trailMesh,plan,stats:{houses:buildings.length,loot:buildings.reduce((n,h)=>n+h.loot.length,0),props:plan.props.length,solidProps:plan.props.filter(p=>p.solid).length,staticInstances}};
}
