import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {pathToFileURL} from 'node:url';
import {spawnSync} from 'node:child_process';

// Run real game logic with a small DOM/canvas stand-in. Browser rendering is
// checked separately; these tests exercise the mission and combat rules.
const gradient={addColorStop(){}};
const context=new Proxy({createLinearGradient:()=>gradient,createRadialGradient:()=>gradient,measureText:t=>({width:t.length*8}),createImageData:(w,h)=>({width:w,height:h,data:new Uint8ClampedArray(w*h*4)})}, {get:(t,k)=>k in t?t[k]:(...args)=>{},set:(t,k,v)=>(t[k]=v,true)});
const elements=new Map();
function element(id=''){if(elements.has(id))return elements.get(id);const classes=new Set();const e={id,width:600,height:600,value:'survivor',style:{},classList:{add:x=>classes.add(x),remove:x=>classes.delete(x),toggle:(x,v)=>v?classes.add(x):classes.delete(x),contains:x=>classes.has(x)},textContent:'',innerHTML:'',getContext:()=>context,append(){},remove(){},addEventListener(){},querySelector:s=>element(id+s),requestPointerLock:()=>Promise.resolve(),setAttribute(){}};elements.set(id,e);return e;}
globalThis.document={getElementById:element,createElement:t=>element(t+Math.random()),addEventListener(){},pointerLockElement:null,exitPointerLock(){},hidden:false};
globalThis.window={addEventListener(){}};globalThis.matchMedia=()=>({matches:false});globalThis.innerWidth=1280;globalThis.innerHeight=720;
const storage=new Map();globalThis.localStorage={getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v)};
const realTimeout=globalThis.setTimeout;globalThis.setTimeout=()=>0;
let code=fs.readFileSync('dist/game.js','utf8').replace(/from '\.\/([^']+)'/g,(_,p)=>'from '+JSON.stringify(pathToFileURL(process.cwd()+'/dist/'+p).href)).replace(/from\s+(['"])(\.\/[^'"]+)\1/g,(_match,_quote,specifier)=>'from '+JSON.stringify(pathToFileURL(path.resolve('dist',specifier)).href)).replace(/\ninit\(\);\s*$/,'');
code+=`\nexport {createWorld,makeNav,findPath,isClear,lineClear,resetRun,initialState,readSave,save,interactables,enemies,colliders,spawns,player,shoot,reload,finishReload,switchWeapon,heal,hurt,interact,closePanel,showEnding,begin,createEnemy,damageEnemy,updateEnemies,weapons,updatePlayer,safeRestore,drink,updateSurvival,collectSupply,collectRobotGold,acceptQuest,claimQuest,claimWelcome,questReady,pickups,quests,allies};
export const inspect=()=>({state,mode,scene,camera,gun,shootTimer,reloadTimer,interactTarget});
export const configure=(v)=>{if(v.state)state=v.state;if(v.mode)mode=v.mode;if(v.target)interactTarget=v.target;if(v.yaw!==undefined)yaw=v.yaw;if(v.pitch!==undefined)pitch=v.pitch;if(v.keys)keys=v.keys;shootTimer=0;invulnerable=v.invulnerable??0;};
`;
fs.writeFileSync('environment-runtime.mjs',code);
const game=await import('./environment-runtime.mjs?'+Date.now());
let passed=0;
function test(name,fn){fn();passed++;console.log('PASS '+name);}
game.createWorld();game.makeNav();game.resetRun();game.configure({mode:'playing'});
test('Every core, cache, record, bench and uplink is reachable',()=>{for(const item of game.interactables){assert(game.isClear(item.x,item.z,.3),item.name+' is inside a collider');const p=game.findPath(0,65,item.x,item.z);assert(p.length>0,item.name+' has no path');}});
test('Enemy spawn points are navigable',()=>{for(const [x,z,type] of game.spawns){assert(game.isClear(x,z,.55),`${type} at ${x},${z} is obstructed`);assert(game.findPath(x,z,0,65).length>0);}});
test('Buildings block visibility and movement; streets remain open',()=>{assert(!game.lineClear(20,55,40,55));assert(!game.isClear(31,58));assert(game.lineClear(0,64,0,35));const old={...game.player};game.player.x=18.5;game.player.z=58;game.configure({yaw:0,keys:{KeyD:true}});for(let i=0;i<50;i++)game.updatePlayer(.05);assert(game.player.x<20,'walked through building');Object.assign(game.player,old);game.configure({keys:{}});});
