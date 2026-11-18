import assert from 'node:assert/strict';
import * as THREE from './dist/vendor/three.module.js';
import {createExpansion} from './dist/expansion.js';
const context=new Proxy({},{get:(o,k)=>o[k]||(()=>{}),set:(o,k,v)=>(o[k]=v,true)});
globalThis.document={createElement:()=>({width:256,height:256,getContext:()=>context})};
const scene=new THREE.Scene(),mats={};for(const k of ['concrete','wall','metal','dark','black','leaf','cyan','purple','light'])mats[k]=new THREE.MeshStandardMaterial({color:0x888888});
const colliders=[],interactables=[];let seed=14;const rnd=(a,b)=>{seed=(seed*1664525+1013904223)>>>0;return a+(seed/4294967296)*(b-a);};
function box(w,h,d,x,y,z,material,parent=scene){const m=new THREE.Mesh(new THREE.BoxGeometry(w,h,d),material);m.position.set(x,y,z);parent.add(m);return m;}
function cylinder(a,b,h,x,y,z,material,parent=scene,s=8){const m=new THREE.Mesh(new THREE.CylinderGeometry(a,b,h,s),material);m.position.set(x,y,z);parent.add(m);return m;}
function blocker(x,z,w,d,h=4,y=h/2,material=mats.concrete){colliders.push({x,z,w:w/2,d:d/2,minY:y-h/2,maxY:y+h/2});return box(w,h,d,x,y,z,material);}
function addSign(t,x,y,z,w,h){return box(w,h,.02,x,y,z,mats.wall);}
function addInteractable(type,x,z,data){const i={type,x,z,...data};interactables.push(i);return i;}
function createAlly(id,x,z,name,robot){addInteractable(robot?'friendly':'human',x,z,{id,name,y:0});}
const result=createExpansion({THREE,scene,mats,box,cylinder,blocker,addSign,rnd,addInteractable,createCrate(){},createAlly});
function hits(point,r=.8){const y=point.y||0;return colliders.filter(c=>Math.abs(point.x-c.x)<c.w+r&&Math.abs(point.z-c.z)<c.d+r&&c.minY<y+1.72&&c.maxY>y+.11);}
for(const item of interactables)assert.equal(hits(item).length,0,`${item.type} ${item.id} overlaps geometry at ${item.x},${item.z},${item.y}`);
for(const b of result.buildings){for(const p of [b.interior,b.door.outside,b.door.threshold,b.door.inside])assert.equal(hits(p).length,0,`${b.id} doorway/center obstructed`);for(let t=0;t<=1;t+=.02){const a=b.door.outside,c=b.door.inside;assert.equal(hits({x:a.x+(c.x-a.x)*t,z:a.z+(c.z-a.z)*t,y:b.floorY},.38).length,0,`${b.id} doorway sweep obstructed`);}}
for(const r of result.climbRoutes){assert.equal(hits(r.bottomPoint,.38).length,0,`${r.id} ladder bottom obstructed`);assert.equal(hits(r.topPoint,.38).length,0,`${r.id} ladder exit obstructed`);assert(result.platforms.some(p=>Math.abs(p.y-r.top)<.01&&Math.abs(p.x-r.exitX)<=p.w/2-.4&&Math.abs(p.z-r.exitZ)<=p.d/2-.4),`${r.id} lacks roof support`);for(let t=0;t<=1;t+=.025){const up={x:r.x,z:r.z,y:r.bottom+(r.top+.65-r.bottom)*t};assert.equal(hits(up,.38).length,0,`${r.id} vertical climb obstructed`);const over={x:r.x+(r.exitX-r.x)*t,z:r.z+(r.exitZ-r.z)*t,y:r.top+.65};assert.equal(hits(over,.38).length,0,`${r.id} parapet crossing obstructed`);const down={x:r.exitX,z:r.exitZ,y:r.top+.65*(1-t)};assert.equal(hits(down,.38).length,0,`${r.id} roof landing obstructed`);}}
console.log(`Expansion checked: ${result.buildings.length} hollow buildings, ${result.platforms.length} walking surfaces, ${result.climbRoutes.length} climb routes, ${interactables.length} reachable items/NPCs. All doorway sweeps and ladder endpoints are clear.`);
