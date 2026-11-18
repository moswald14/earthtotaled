import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import * as THREE from './dist/vendor/three.module.js';

// Geometry and gameplay checks use real Three.js meshes with a small DOM stand-in.
// They do not claim browser rendering, pointer permissions, or visual verification.
const root=path.dirname(fileURLToPath(import.meta.url));
const gradient={addColorStop(){}};
const context=new Proxy({createLinearGradient:()=>gradient,createRadialGradient:()=>gradient,measureText:t=>({width:t.length*8}),createImageData:(w,h)=>({width:w,height:h,data:new Uint8ClampedArray(w*h*4)})},{get:(t,k)=>k in t?t[k]:(()=>{}),set:(t,k,v)=>(t[k]=v,true)});
const elements=new Map();
function element(id=''){
 if(elements.has(id))return elements.get(id);
 const classes=new Set(),e={id,width:600,height:600,value:'survivor',style:{},textContent:'',innerHTML:'',classList:{add:x=>classes.add(x),remove:x=>classes.delete(x),toggle:(x,v)=>v?classes.add(x):classes.delete(x),contains:x=>classes.has(x)},getContext:()=>context,append(){},remove(){},addEventListener(){},removeEventListener(){},querySelector:s=>element(id+s),querySelectorAll:()=>[],focus(){},requestPointerLock:()=>Promise.resolve(),setAttribute(){}};
 elements.set(id,e);return e;
}
globalThis.document={getElementById:element,createElement:t=>element(t+Math.random()),addEventListener(){},pointerLockElement:null,exitPointerLock(){},hidden:false};
globalThis.window={addEventListener(){}};globalThis.matchMedia=()=>({matches:false});globalThis.innerWidth=1280;globalThis.innerHeight=720;
const storage=new Map();globalThis.localStorage={getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v)};
let passed=0;const failures=[];
const diagnostic=error=>String(error.stack||error).replace(/data:text\/javascript;base64,[A-Za-z0-9+/=]+/g,'[in-memory game]');
function check(name,fn){try{fn();passed++;console.log('PASS '+name);}catch(error){failures.push({name,error});console.error('FAIL '+name+'\n'+diagnostic(error));}}
const near=(a,b,epsilon=.015)=>Math.abs(a-b)<=epsilon;
function makeFixture(){
 const scene=new THREE.Scene(),solids=[],interactables=[],mats=new Proxy({},{get(t,k){return t[k]||(t[k]=new THREE.MeshStandardMaterial({color:0x888888}));}});
 function box(w,h,d,x,y,z,material,parent=scene,solid=false){const mesh=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),material);mesh.position.set(x,y,z);parent.add(mesh);if(solid)solids.push(mesh);return mesh;}
 function cylinder(a,b,h,x,y,z,material,parent=scene,segments=8){const mesh=new THREE.Mesh(new THREE.CylinderGeometry(a,b,h,segments),material);mesh.position.set(x,y,z);parent.add(mesh);return mesh;}
 function blocker(x,z,w,d,h=4,y=h/2,material=mats.concrete){return box(w,h,d,x,y,z,material,scene,true);}
 function addSign(_text,x,y,z,w,h){return box(w,h,.02,x,y,z,mats.wall);}
 function addInteractable(type,x,z,data){const item={type,x,z,...data};interactables.push(item);return item;}
 function bounds(){scene.updateMatrixWorld(true);return solids.map(mesh=>new THREE.Box3().setFromObject(mesh));}
 return {THREE,scene,mats,box,cylinder,blocker,addSign,addInteractable,interactables,solids,bounds};
}
function bodyClear(bounds,x,z,feet,r=.38){return !bounds.some(b=>x+r>b.min.x&&x-r<b.max.x&&z+r>b.min.z&&z-r<b.max.z&&feet+1.7>b.min.y&&feet+.11<b.max.y);}
function clearSegment(bounds,a,b,feet){const steps=Math.max(1,Math.ceil(Math.hypot(b.x-a.x,b.z-a.z)/.15));for(let n=0;n<=steps;n++){const t=n/steps;if(!bodyClear(bounds,a.x+(b.x-a.x)*t,a.z+(b.z-a.z)*t,feet))return false;}return true;}

function reachableHouse(house,allBounds,targets){
 const margin=4,step=.35,minX=house.x-house.w/2-margin,minZ=house.z-house.d/2-margin;
 const nx=Math.ceil((house.w+margin*2)/step)+1,nz=Math.ceil((house.d+margin*2)/step)+1;
 const bounds=allBounds.filter(b=>b.max.x>=minX&&b.min.x<=minX+(nx-1)*step&&b.max.z>=minZ&&b.min.z<=minZ+(nz-1)*step);
 const points=Array.from({length:nx*nz},(_,n)=>({x:minX+(n%nx)*step,z:minZ+Math.floor(n/nx)*step}));
 const open=points.map(p=>bodyClear(bounds,p.x,p.z,house.floorY)),seen=new Uint8Array(points.length),queue=[];
 for(let n=0;n<points.length;n++)if(open[n]&&Math.hypot(points[n].x-house.door.outside.x,points[n].z-house.door.outside.z)<.7&&clearSegment(bounds,house.door.outside,points[n],house.floorY)){seen[n]=1;queue.push(n);}
 assert(queue.length,house.id+' has no clear exterior approach');
 for(let head=0;head<queue.length;head++){
  const n=queue[head],x=n%nx,z=Math.floor(n/nx);
  for(const [dx,dz] of [[1,0],[-1,0],[0,1],[0,-1]]){
   const xx=x+dx,zz=z+dz,i=zz*nx+xx;if(xx<0||xx>=nx||zz<0||zz>=nz||seen[i]||!open[i])continue;
   if(clearSegment(bounds,points[n],points[i],house.floorY)){seen[i]=1;queue.push(i);}
  }
 }
 for(const target of targets){
  assert(bodyClear(bounds,target.x,target.z,house.floorY),house.id+' target '+(target.id||'room')+' intersects a solid');
  assert(queue.some(n=>Math.hypot(points[n].x-target.x,points[n].z-target.z)<.7&&clearSegment(bounds,points[n],target,house.floorY)),house.id+' has no physical route to '+(target.id||JSON.stringify(target)));
 }
}

const {WILDERNESS_LIMIT,createWildernessPlan,terrainHeight,createWilderness}=await import('./dist/wilderness.js');
const plans=new Map();
check('World seeds reproduce terrain, house contents, wildlife and traps deterministically',()=>{
 for(const seed of [0,1,84371,0xffffffff]){const a=createWildernessPlan(seed),b=createWildernessPlan(seed);assert.deepEqual(a,b,'same seed changed world '+seed);assert.equal(a.seed,seed);plans.set(seed,a);}
 const a=plans.get(84371),b=plans.get(1);assert.notDeepEqual(a.heights,b.heights,'different seeds changed only metadata');assert.notDeepEqual(a.houses.map(h=>[h.x,h.z,h.loot]),b.houses.map(h=>[h.x,h.z,h.loot]));
});

check('Generated terrain stays finite, keeps the city flat, and provides elevated wilderness',()=>{
 for(const plan of plans.values()){
  assert.equal(plan.heights.length,plan.gridSize**2);assert(plan.heights.every(Number.isFinite));
  for(let z=-180;z<=180;z+=30)for(let x=-180;x<=180;x+=30)assert.equal(terrainHeight(plan,x,z),0,'new ground displaced the existing city');
  assert(Math.max(...plan.heights)-Math.min(...plan.heights)>10,'wilderness is visually flat');
  for(const x of [-WILDERNESS_LIMIT,WILDERNESS_LIMIT])for(const z of [-WILDERNESS_LIMIT,WILDERNESS_LIMIT])assert(Number.isFinite(terrainHeight(plan,x,z)));
  const ids=[...plan.houses.map(h=>h.id),...plan.houses.flatMap(h=>h.loot.map(i=>i.id)),...plan.animals.map(a=>a.id),...plan.traps.map(t=>t.id)];assert.equal(new Set(ids).size,ids.length,'generated identifiers collide');
  for(const h of plan.houses){assert(Math.max(Math.abs(h.x),Math.abs(h.z))>220);assert(h.rooms.length>=3&&h.loot.length>=2);for(const p of [h.interior,h.door.outside,h.door.threshold,h.door.inside,...h.loot])assert(near(terrainHeight(plan,p.x,p.z),h.y,.025),h.id+' has uneven ground beneath its floor or entrance');}
 }
});

const worlds=[];
for(const seed of [84371,0,0xffffffff]){
 const fixture=makeFixture(),plan=plans.get(seed),data=createWilderness({...fixture,plan});
 worlds.push({...fixture,plan,data,bounds:fixture.bounds()});
}

check('Physics terrain heights match downward ray hits on the actual rendered triangles',()=>{
 for(const {plan,data} of worlds){
  assert(data.terrainMesh?.isMesh);data.terrainMesh.updateMatrixWorld(true);
  const points=[{x:0,z:65},{x:-716.3,z:-709.7},{x:713.1,z:702.7},...plan.houses.slice(0,4).map(h=>({x:h.x+.27,z:h.z-.61}))];
  for(let n=0;n<20;n++)points.push({x:-690+(n*139.73)%1380,z:-680+(n*217.17)%1360});
  for(const p of points){const ray=new THREE.Raycaster(new THREE.Vector3(p.x,200,p.z),new THREE.Vector3(0,-1,0),0,400),hit=ray.intersectObject(data.terrainMesh,false)[0];assert(hit,'terrain mesh has a hole at '+JSON.stringify(p));assert(near(hit.point.y,terrainHeight(plan,p.x,p.z),.0001),'rendered slope and physical terrain differ');}
 }
});

check('Every generated house has clear physical routes through its door to all rooms and loot',()=>{
 for(const world of worlds)for(const house of world.data.buildings){
  assert(clearSegment(world.bounds,house.door.outside,house.door.inside,house.floorY),house.id+' front doorway is blocked for seed '+world.plan.seed);
  for(const room of house.rooms)assert(clearSegment(world.bounds,room.door.outside,room.door.inside,house.floorY),room.id+' partition doorway is blocked');
  reachableHouse(house,world.bounds,[house.door.inside,...house.rooms.map(r=>({...r.center,id:r.id})),...house.loot]);
  const room=house.rooms[0].center;assert(world.data.platforms.some(p=>near(p.y,house.floorY)&&Math.abs(room.x-p.x)<p.w/2&&Math.abs(room.z-p.z)<p.d/2),house.id+' lacks floor support');
  assert(world.data.platforms.some(p=>near(p.y,house.roofY)&&p.x===house.x&&p.z===house.z),house.id+' lacks roof support');
  const side=house.door.normal,rear={x:house.x-side.x*(Math.abs(side.x)?house.w:house.d)/2,z:house.z-side.z*(Math.abs(side.z)?house.d:house.w)/2};assert(!bodyClear(world.bounds,rear.x,rear.z,house.floorY),house.id+' rear wall is not physically solid');
 }
});

check('House windows are real shot apertures rather than painted solid walls',()=>{
 for(const world of worlds){world.scene.updateMatrixWorld(true);for(const house of world.data.buildings){
  assert(house.windows.length>=4);const window=house.windows[0],direction=new THREE.Vector3(-window.normal.x,0,-window.normal.z),origin=new THREE.Vector3(window.center.x,window.center.y,window.center.z).addScaledVector(direction,-1);
  const ray=new THREE.Raycaster(origin,direction,0,2);assert.equal(ray.intersectObjects(world.solids,false).length,0,house.id+' window opening blocks shots');
 }}
});

const {createWildlife,WILDLIFE_SPECIES}=await import('./dist/wildlife.js');
const wildlifeState=()=>({water:3,hydration:81,elapsed:0,wildlifeKilled:[],wildlifeHarvested:[],wildlifeBodies:{},trapsCleared:[]});
function wildlifeFixture({animals=[],traps=[],heightAt=()=>5,blocked=()=>false}={}){
 const fixture=makeFixture(),state=wildlifeState(),life=createWildlife({...fixture,root:fixture.scene,plan:{seed:84371,limit:720,animals,traps},heightAt,blocked});life.reset(state);
 return {...fixture,state,life,heightAt};
}

check('Marked traps trigger once at ground level and remain cleared after reload',()=>{
 const f=wildlifeFixture({traps:[{id:'trap:jaw',x:310,z:300,kind:'jaw'},{id:'trap:snare',x:320,z:300,kind:'snare'}]});
 let damage=0,dirty=0;
 for(const trap of f.life.traps){
  const player={x:trap.x,z:trap.z,y:trap.y+1.72+4};
  const tick=()=>f.life.update(.1,{player,onDamage(amount,reason){assert(f.state.trapsCleared.includes(trap.id),'damage preceded the persistent clear flag');assert.equal(reason,'trap');damage+=amount;},onDirty(){dirty++;}});
  tick();assert.equal(trap.armed,true,'a trap hit someone above its floor');player.y=trap.y+1.72;const before=damage;tick();
  assert.equal(damage-before,trap.kind==='snare'?12:20);assert.equal(trap.armed,false);assert.equal(trap.item.disabled,true);tick();assert.equal(damage-before,trap.kind==='snare'?12:20);
 }
 assert.equal(dirty,2);assert.equal(f.state.water,3);assert.equal(f.state.hydration,81);
 const saved=JSON.parse(JSON.stringify(f.state));f.life.reset(saved);
 for(const trap of f.life.traps){assert.equal(trap.armed,false);f.life.update(.1,{player:{x:trap.x,z:trap.z,y:trap.y+1.72},onDamage(){assert.fail('a saved trap triggered again');}});}
});

check('Traps can be disarmed from a safe reachable distance exactly once',()=>{
 const f=wildlifeFixture({traps:[{id:'trap:disarm',x:310,z:300,kind:'jaw'}]}),trap=f.life.traps[0];
 f.life.update(.1,{player:{x:trap.x+8,z:trap.z,y:trap.y+1.72}});assert.equal(f.life.disarm(trap.item,f.state),false,'distant disarm succeeded');
 f.life.update(.1,{player:{x:trap.x+2,z:trap.z,y:trap.y+6.72}});assert.equal(f.life.disarm(trap.item,f.state),false,'disarm passed through a floor');
 let dirty=0;f.life.update(.1,{player:{x:trap.x+2,z:trap.z,y:trap.y+1.72},onDirty(){dirty++;},onDamage(){assert.fail('safe disarm approach triggered damage');}});
 assert.equal(f.life.disarm(trap.item,f.state),true);assert.equal(f.life.disarm(trap.item,f.state),false);assert.equal(dirty,1);assert.deepEqual(f.state.trapsCleared,[trap.id]);
 f.life.reset(JSON.parse(JSON.stringify(f.state)));assert.equal(trap.armed,false);assert.equal(trap.item.disabled,true);
});

check('Every wildlife species respects shot obstruction and yields no water or duplicate harvest',()=>{
 for(const species of Object.keys(WILDLIFE_SPECIES)){
  const f=wildlifeFixture({animals:[{id:'animal:'+species,species,x:300,z:300,heading:0,range:45}]}),animal=f.life.animals[0];
  assert(animal,'species was not created');f.life.update(.1,{player:{x:300,z:305,y:6.72}});
  const target=new THREE.Vector3(animal.x,animal.y+WILDLIFE_SPECIES[species].bodyY,animal.z),origin=target.clone().add(new THREE.Vector3(0,0,-10));
  const ray=new THREE.Raycaster(origin,target.clone().sub(origin).normalize()),before=animal.health;
  assert.equal(f.life.shoot(ray,30,1,9999),null);assert.equal(animal.health,before);assert.equal(f.life.shoot(ray,3,Infinity,9999),null);
  const hit=f.life.shoot(ray,30,Infinity,9999);assert(hit?.killed,species+' did not receive the visible shot');assert(f.state.wildlifeKilled.includes(animal.id));
  const body={x:animal.x,z:animal.z},saved=JSON.parse(JSON.stringify(f.state));f.life.reset(saved);
  assert.equal(animal.dead,true);assert(near(animal.x,body.x)&&near(animal.z,body.z),'saved carcass moved back to its spawn');assert.equal(animal.item.disabled,false);
  f.life.update(.1,{player:{x:animal.x+8,z:animal.z,y:animal.y+1.72}});assert.equal(f.life.harvest(animal.item,saved),null);
  f.life.update(.1,{player:{x:animal.x+2,z:animal.z,y:animal.y+1.72}});
  const reward=f.life.harvest(animal.item,saved);assert(reward);assert.equal(reward.water,0);assert(reward.scrap>0||reward.health>0);assert.equal(saved.water,3);assert.equal(saved.hydration,81);assert.equal(f.life.harvest(animal.item,saved),null);
  assert.equal(f.interactables.some(i=>i.type==='water'),false,'a wildlife kill created drinking water');
  f.life.reset(JSON.parse(JSON.stringify(saved)));assert.equal(animal.group.visible,false);assert.equal(animal.item.disabled,true);assert.equal(f.life.harvest(animal.item,saved),null);
 }
});

check('Moving wildlife follows terrain and cannot cross a solid obstacle',()=>{
 const heightAt=(x,z)=>5+(x-300)*.03+(z-300)*.015;
 const f=wildlifeFixture({animals:[{id:'animal:walker',species:'deer',x:300,z:300,range:45}],heightAt,blocked:(x,_z,r)=>x+r>303}),animal=f.life.animals[0];
 animal.mode='wander';animal.timer=100;animal.target={x:320,z:300};const start={x:animal.x,z:animal.z};
 for(let n=0;n<100;n++){f.life.update(.1,{player:{x:300,z:370,y:heightAt(300,370)+1.72},elapsed:n*.1});assert(animal.x+WILDLIFE_SPECIES.deer.radius<=303.00001,'animal crossed a solid wall');assert(near(animal.y,heightAt(animal.x,animal.z),.00001));assert(near(animal.group.position.y,animal.y,.00001));}
 assert(Math.hypot(animal.x-start.x,animal.z-start.z)>.5,'the obstacle check never exercised movement');
});

check('Aggressive wildlife cannot damage players through walls or on higher floors',()=>{
 let wall=false;const f=wildlifeFixture({animals:[{id:'animal:wolf',species:'wolf',x:300,z:300,range:45}],blocked:(x,_z,r)=>wall&&x-r<300.8&&x+r>300.4}),animal=f.life.animals[0];let damage=0;
 const tick=player=>f.life.update(.01,{player,onDamage(amount){damage+=amount;}});
 wall=true;tick({x:301.3,z:300,y:6.72});assert.equal(damage,0);wall=false;tick({x:animal.x+1.3,z:animal.z,y:animal.y+6.72});assert.equal(damage,0);
 tick({x:animal.x+1.3,z:animal.z,y:animal.y+1.72});assert(damage>0,'control case never exercised a visible same-level attack');
});

let code=fs.readFileSync(path.join(root,'dist/game.js'),'utf8').replace(/from\s+(['"])(\.\/[^'"]+)\1/g,(_match,_quote,specifier)=>'from '+JSON.stringify(pathToFileURL(path.resolve(root,'dist',specifier)).href)).replace(/\ninit\(\);\s*$/,'');
code+=`\nexport {createWorld,resetRun,initialState,safeRestore,save,readSave,player,interactables,colliders,solidMeshes,worldBuildings,platforms,wildernessPlan,wildernessData,wildlife,terrainFloor,supportAt,isClear,lineClear,updateVertical,updatePlayer,updateWildlife,interact,lootWilderness,streamedRegions,regionPlan,syncRegions,createEndlessPlan};
export const inspect=()=>({state,mode,velocityY});
export const configure=v=>{if(v.mode)mode=v.mode;if(v.keys)keys=v.keys;if(v.yaw!==undefined)yaw=v.yaw;if(Object.hasOwn(v,'target'))interactTarget=v.target;if(v.velocityY!==undefined)velocityY=v.velocityY;if(v.feet!==undefined)fallStart=v.feet;invulnerable=v.invulnerable??0;};`;
const realTimeout=globalThis.setTimeout;globalThis.setTimeout=()=>0;
try{
 const game=await import('data:text/javascript;base64,'+Buffer.from(code).toString('base64'));
 game.createWorld();game.resetRun();game.configure({mode:'playing'});
 const place=point=>{game.player.x=point.x;game.player.z=point.z;game.player.y=point.y+1.72;game.configure({mode:'playing',keys:{},velocityY:0,feet:point.y,target:null});};

 check('Real player movement crosses every wilderness entrance without wall clipping or falling',()=>{
  for(const house of game.wildernessData.buildings){
   const a=house.door.outside,b=house.door.inside;place({x:a.x,z:a.z,y:game.terrainFloor(a.x,a.z)});
   const dx=b.x-a.x,dz=b.z-a.z,distance=Math.hypot(dx,dz);game.configure({yaw:Math.atan2(-dx,-dz),keys:{KeyW:true}});
   let remaining=distance/4.6;while(remaining>1e-8){const dt=Math.min(.02,remaining);game.updatePlayer(dt);remaining-=dt;}
   game.configure({keys:{}});assert(Math.hypot(game.player.x-b.x,game.player.z-b.z)<.2,house.id+' cannot be entered using normal movement');assert(near(game.player.y-1.72,house.floorY,.03),house.id+' entered at the wrong floor height');
  }
 });

 check('Terrain supports falling and walking players and blocks underground sight lines',()=>{
  let point=null;
  for(let z=240;z<660&&!point;z+=23)for(let x=240;x<660;x+=29){const y=game.terrainFloor(x,z);if(y>3&&Array.from({length:30},(_,n)=>({x:x+n*.2,z})).every(p=>game.isClear(p.x,p.z,.5,game.terrainFloor(p.x,p.z)))){point={x,z,y};break;}}
  assert(point,'no clear rural movement sample found');place({...point,y:point.y+2});
  for(let n=0;n<150;n++)game.updateVertical(.02);assert(near(game.player.y-1.72,point.y,.001),'fall stopped above or below the terrain');
  game.configure({yaw:0,keys:{KeyD:true}});for(let n=0;n<50;n++)game.updatePlayer(.02);game.configure({keys:{}});
  assert(game.player.x>point.x+4,'terrain movement stuck on an ordinary slope');for(let n=0;n<30;n++)game.updateVertical(.02);
  assert(near(game.player.y-1.72,game.terrainFloor(game.player.x,game.player.z),.02),'walking left the player floating or underground');
  assert(game.solidMeshes.includes(game.wildernessData.terrainMesh),'terrain omitted from shot obstruction geometry');
  assert.equal(game.lineClear(point.x,point.z,point.x+6,point.z,point.y-1,game.terrainFloor(point.x+6,point.z)-1),false,'line of sight crossed terrain');
  const house=game.wildernessData.buildings[0],room=house.rooms[0].center;
  assert(near(game.supportAt(room.x,room.z,house.floorY+.02),house.floorY));assert(near(game.supportAt(room.x,room.z,house.roofY+.02),house.roofY));
 });

 check('House supplies can be collected only once and keep their saved water attribution',()=>{
  game.resetRun();game.configure({mode:'playing'});const item=game.interactables.find(i=>i.type==='wildernessLoot'&&i.water>0),state=game.inspect().state;
  assert(item,'world contains no sealed household supplies');place({x:item.x,z:item.z,y:item.y});const water=state.water,scrap=state.scrap;
  assert.equal(game.lootWilderness(item),true);assert.equal(state.water,water+item.water);assert(state.scrap>scrap);assert.equal(game.lootWilderness(item),false);
  game.save();game.resetRun(game.readSave());const restored=game.inspect().state,loaded=game.interactables.find(i=>i.id===item.id);assert.equal(game.lootWilderness(loaded),false);assert.equal(restored.water,water+item.water);
 });

 check('Game interactions disarm traps and harvest wildlife without creating water rewards',()=>{
  game.resetRun();game.configure({mode:'playing'});const state=game.inspect().state,trap=game.wildlife.traps[0],animal=game.wildlife.animals[0];assert(trap&&animal);
  const water=state.water;place({x:trap.x+2,z:trap.z,y:trap.y});game.updateWildlife(.01);game.configure({target:trap.item});const scrap=state.scrap;game.interact();
  assert.equal(trap.armed,false);assert.equal(state.scrap,scrap+4);assert.equal(state.water,water);game.interact();assert.equal(state.scrap,scrap+4);
  place({x:animal.x,z:animal.z-8,y:animal.y});game.updateWildlife(.01);const target=new THREE.Vector3(animal.x,animal.y+WILDLIFE_SPECIES[animal.species].bodyY,animal.z),origin=target.clone().add(new THREE.Vector3(0,0,-10));
  assert(game.wildlife.shoot(new THREE.Raycaster(origin,target.clone().sub(origin).normalize()),30,Infinity,9999)?.killed);
  place({x:animal.x+2,z:animal.z,y:animal.y});game.updateWildlife(.01);game.configure({target:animal.item});game.interact();assert(state.wildlifeHarvested.includes(animal.id));assert.equal(state.water,water);
  const earned=state.scrap;game.interact();assert.equal(state.scrap,earned);assert.equal(state.water,water);
 });

 check('Saved worlds restore the same seed, location, house loot, carcasses and trap state after another seed',()=>{
  const seed=84371;game.resetRun(game.initialState('survivor',seed));game.configure({mode:'playing'});
  const state=game.inspect().state,house=game.wildernessData.buildings[0],trap=game.wildlife.traps[0],animal=game.wildlife.animals[0],item=game.interactables.find(i=>i.type==='wildernessLoot');
  state.trapsCleared.push(trap.id);state.wildlifeKilled.push(animal.id);state.wildlifeBodies[animal.id]={x:animal.x,z:animal.z};state.opened.push(item.id);state.water=7;state.gold=123;state.quests.filter='done';place({x:house.door.inside.x,z:house.door.inside.z,y:house.floorY});
  const location={...game.player},layout=JSON.stringify(game.wildernessPlan.houses),counts=[game.colliders.length,game.interactables.length,game.platforms.length,game.worldBuildings.length];assert.equal(game.save(),true);const saved=game.readSave();assert.equal(saved.worldSeed,seed);
  game.resetRun(game.initialState('survivor',seed+1));assert.equal(game.wildernessPlan.seed,seed+1);assert.notEqual(JSON.stringify(game.wildernessPlan.houses),layout);
  game.resetRun(saved);assert.equal(game.wildernessPlan.seed,seed);assert.equal(JSON.stringify(game.wildernessPlan.houses),layout);assert.deepEqual([game.colliders.length,game.interactables.length,game.platforms.length,game.worldBuildings.length],counts,'rebuilding accumulated stale world objects');
  assert.deepEqual(game.player,location);assert.equal(game.inspect().state.water,7);assert.equal(game.inspect().state.gold,123);assert.equal(game.inspect().state.quests.filter,'done');assert(game.inspect().state.opened.includes(item.id));assert.equal(game.wildlife.traps.find(t=>t.id===trap.id).armed,false);assert.equal(game.wildlife.animals.find(a=>a.id===animal.id).dead,true);
 });

 check('Endless regions meet without terrain seams in every direction',()=>{
  game.resetRun();const seed=game.inspect().state.worldSeed;
  for(const [cx,cz] of [[1,0],[-1,0],[0,1],[0,-1],[8,-5]]){const p=game.createEndlessPlan(seed,cx,cz),q=game.createEndlessPlan(seed,cx+1,cz);for(let z=-720;z<=720;z+=80)assert(near(terrainHeight(p,720,z),terrainHeight(q,-720,z),.0001),'east/west terrain seam');const r=game.createEndlessPlan(seed,cx,cz+1);for(let x=-720;x<=720;x+=80)assert(near(terrainHeight(p,x,720),terrainHeight(r,x,-720),.0001),'north/south terrain seam');}
 });
 check('Travel beyond the old fence streams houses, saves distant position and preserves loot',()=>{
  game.resetRun();game.configure({mode:'playing'});const base=game.colliders.length;
  for(const [x,z] of [[1500,50],[-3000,1600],[12000,-9000]]){game.player.x=x;game.player.z=z;game.syncRegions(true);game.player.y=game.terrainFloor(x,z)+1.72;assert(game.isClear(x,z,.38,game.player.y-1.72));assert(game.streamedRegions.size<=6,'distant scenery accumulated');assert(game.worldBuildings.some(b=>Math.hypot(b.x-x,b.z-z)<1100),'no generated houses beyond boundary');}
  const item=game.interactables.find(i=>i.type==='wildernessLoot'&&i.id.startsWith('region:'));assert(item);const id=item.id;game.lootWilderness(item);const saved={...game.inspect().state,position:{...game.player}};game.resetRun(saved);assert.equal(game.player.x,12000);assert.equal(game.player.z,-9000);assert(game.inspect().state.opened.includes(id));const loaded=game.interactables.find(i=>i.id===id);assert(loaded);assert.equal(game.lootWilderness(loaded),false);game.resetRun();assert.equal(game.streamedRegions.size,0);assert.equal(game.colliders.length,base);
 });
 check('Older saves gain a stable world seed and empty wildlife state without losing progress',()=>{
  const old={...game.initialState('desolate'),health:61,gold:72,water:6,quests:{filter:'done',pump:'active'},position:{x:-48,z:20},opened:[0,2],killed:[0,1]};
  for(const field of ['worldSeed','wildlifeKilled','wildlifeHarvested','wildlifeBodies','trapsCleared'])delete old[field];
  const a=game.safeRestore(old),b=game.safeRestore(old);assert.equal(a.worldSeed,84371);assert.equal(a.worldSeed,b.worldSeed);assert.equal(a.health,61);assert.equal(a.water,6);assert.equal(a.gold,72);assert.equal(a.quests.filter,'done');assert.deepEqual(a.opened,[0,2]);assert.deepEqual(a.killed,[0,1]);assert.deepEqual(a.wildlifeKilled,[]);assert.deepEqual(a.wildlifeHarvested,[]);assert.deepEqual(a.trapsCleared,[]);assert.deepEqual(a.wildlifeBodies,{});
  const zero=game.safeRestore({...old,worldSeed:0});assert.equal(zero.worldSeed,0,'seed zero must survive a continuation');
 });
}catch(error){failures.push({name:'Game integration setup',error});console.error('FAIL Game integration setup\n'+diagnostic(error));}finally{globalThis.setTimeout=realTimeout;}

console.log(`\n${passed} wilderness checks passed; ${failures.length} failed. Physical paths sampled across ${worlds.reduce((n,w)=>n+w.data.buildings.length,0)} houses; browser rendering requires separate verification.`);
if(failures.length)process.exitCode=1;

