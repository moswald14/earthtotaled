// Original low-poly wildlife. No external assets, water drops, or graphic effects.
export const WILDLIFE_SPECIES = Object.freeze({
  deer: {name:'Deer',health:52,radius:.6,bodyY:1.12,walk:1.05,run:6.2,damage:0,scrap:3,healthReward:8},
  boar: {name:'Wild boar',health:96,radius:.65,bodyY:.72,walk:.7,run:4.4,damage:14,scrap:5,healthReward:12},
  wolf: {name:'Grey wolf',health:66,radius:.5,bodyY:.8,walk:1.2,run:4.8,damage:10,scrap:4,healthReward:8}
});

export function createWildlife({THREE,root,mats={},plan={},heightAt=()=>0,blocked=()=>false,addInteractable}) {
  const animals=[],traps=[],animalById=new Map(),trapById=new Map();
  const geometry={round:new THREE.IcosahedronGeometry(1,1),box:new THREE.BoxGeometry(1,1,1),cylinder:new THREE.CylinderGeometry(1,1,1,7),cone:new THREE.ConeGeometry(1,1,7),ring:new THREE.TorusGeometry(1,.045,5,20)};
  const material=(color,metalness=0)=>new THREE.MeshStandardMaterial({color,roughness:.91,metalness});
  const colors={deer:material(0x92775b),deerDark:material(0x66523f),cream:material(0xc4b69b),boar:material(0x66594a),bristle:material(0x3c3a32),wolf:material(0x778079),wolfDark:material(0x4a5552),black:mats.black||material(0x1e2722),rust:material(0x8d5b3e,.22),metal:mats.metal||material(0x71756a,.35),warning:material(0xd0ad51),safe:material(0x6f927a),rope:material(0x9f9678)};
  const meshes=[];
  let currentState=null,lastPlayer=null,callbacks={},elapsedTime=0;
  const clamp=(n,a,b)=>Math.max(a,Math.min(b,n));
  const distance=(a,b)=>Math.hypot(a.x-b.x,a.z-b.z);
  const ground=(x,z)=>{const y=heightAt(x,z);return Number.isFinite(y)?y:0;};
  const outsideCity=(x,z)=>Math.max(Math.abs(x),Math.abs(z))>205;
  const feet=p=>Number.isFinite(p?.feetY)?p.feetY:(Number.isFinite(p?.y)?p.y-1.72:ground(p.x,p.z));
  const safe=(x,z,r=.5)=>Number.isFinite(x)&&Number.isFinite(z)&&outsideCity(x,z)&&(plan.bounds?(x>plan.bounds.minX+4&&x<plan.bounds.maxX-4&&z>plan.bounds.minZ+4&&z<plan.bounds.maxZ-4):Math.max(Math.abs(x),Math.abs(z))<(plan.limit||720)-4)&&!blocked(x,z,r);
  const validId=id=>typeof id==='string'&&id.length>0&&id.length<100&&id!=='__proto__'&&id!=='constructor'&&id!=='prototype';
  function mesh(type,scale,x,y,z,mat,parent){const m=new THREE.Mesh(geometry[type],mat);m.scale.set(...scale);m.position.set(x,y,z);m.castShadow=true;m.receiveShadow=true;parent.add(m);return m;}
  function link(parent,a,b,r,mat){const start=new THREE.Vector3(...a),end=new THREE.Vector3(...b),delta=end.sub(start),m=mesh('cylinder',[r,delta.length(),r],...start.clone().addScaledVector(delta,.5).toArray(),mat,parent);m.quaternion.setFromUnitVectors(new THREE.Vector3(0,1,0),delta.normalize());return m;}
  function rngFor(id){let n=(plan.seed||7621)>>>0;for(let i=0;i<id.length;i++)n=(Math.imul(n^id.charCodeAt(i),16777619))>>>0;return()=>{n=(Math.imul(n,1664525)+1013904223)>>>0;return n/4294967296;};}
  function interaction(type,x,z,data){return typeof addInteractable==='function'?(addInteractable(type,x,z,data)||{type,x,z,...data}):{type,x,z,...data};}
  function ensureState(state){
    for(const key of ['wildlifeKilled','wildlifeHarvested','trapsCleared'])if(!Array.isArray(state[key]))state[key]=[];
    if(!state.wildlifeBodies||typeof state.wildlifeBodies!=='object'||Array.isArray(state.wildlifeBodies))state.wildlifeBodies={};
    return state;
  }
  function dirty(){if(typeof callbacks.onDirty==='function')callbacks.onDirty();}
  function message(text){if(typeof callbacks.onMessage==='function')callbacks.onMessage(text);}
  function buildAnimal(spec){
    const species=WILDLIFE_SPECIES[spec.species],random=rngFor(spec.id),group=new THREE.Group(),rig=new THREE.Group(),legs=[];
    group.name=species.name+' / '+spec.id;group.position.set(spec.x,ground(spec.x,spec.z),spec.z);group.rotation.y=spec.heading||0;root.add(group);group.add(rig);rig.position.y=species.bodyY;
    const fur=colors[spec.species],shade=colors[spec.species+'Dark']||colors.bristle;
    const bodyScale=spec.species==='deer'?[.34,.43,.77]:spec.species==='boar'?[.48,.43,.72]:[.3,.31,.72];
    mesh('round',bodyScale,0,0,0,fur,rig);
    mesh('round',[bodyScale[0]*.9,bodyScale[1]*.92,.38],0,.03,-.49,shade,rig);
    const head=new THREE.Group();head.position.set(0,spec.species==='deer'?.57:spec.species==='boar'?-.02:.16,.72);rig.add(head);
    if(spec.species==='deer'){
      const neck=mesh('round',[.22,.5,.27],0,.36,.51,fur,rig);neck.rotation.x=-.32;
      mesh('round',[.23,.28,.36],0,.05,.05,fur,head);mesh('round',[.16,.14,.28],0,-.09,.34,shade,head);
      for(const sign of [-1,1]){const ear=mesh('round',[.1,.24,.09],sign*.24,.32,-.02,fur,head);ear.rotation.z=sign*-.7;mesh('round',[.065,.16,.05],sign*.25,.34,.04,colors.cream,head);}
      if(random()>.3)for(const sign of [-1,1]){
        link(head,[sign*.12,.27,-.09],[sign*.29,.84,-.3],.033,colors.cream);
        link(head,[sign*.29,.84,-.3],[sign*.37,1.03,-.16],.022,colors.cream);
        link(head,[sign*.21,.59,-.22],[sign*.43,.8,-.08],.025,colors.cream);
      }
      mesh('round',[.13,.16,.19],0,.12,-.78,colors.cream,rig);
    }else if(spec.species==='boar'){
      mesh('round',[.34,.32,.37],0,0,.04,fur,head);mesh('round',[.24,.2,.3],0,-.08,.33,shade,head);mesh('round',[.22,.17,.08],0,-.08,.57,colors.deerDark,head);
      for(const sign of [-1,1]){const ear=mesh('cone',[.13,.29,.12],sign*.23,.33,-.01,fur,head);ear.rotation.z=sign*-.4;const tusk=mesh('cone',[.055,.24,.05],sign*.24,-.08,.4,colors.cream,head);tusk.rotation.z=sign*-.5;}
      for(let i=0;i<6;i++)mesh('cone',[.065,.22,.075],0,.46,-.42+i*.17,colors.bristle,rig);
      const tail=mesh('cylinder',[.035,.39,.035],0,.1,-.82,shade,rig);tail.rotation.x=.9;
    }else{
      mesh('round',[.25,.27,.29],0,.03,.05,fur,head);mesh('round',[.15,.15,.3],0,-.04,.3,colors.cream,head);mesh('round',[.11,.1,.08],0,-.01,.55,colors.black,head);
      for(const sign of [-1,1]){const ear=mesh('cone',[.11,.33,.11],sign*.16,.32,-.03,shade,head);ear.rotation.z=sign*-.2;}
      const tail=mesh('round',[.14,.15,.55],0,-.17,-1.03,shade,rig);tail.rotation.x=-.36;
      mesh('round',[.24,.2,.36],0,-.13,.4,colors.cream,rig);
    }
    for(const sign of [-1,1])mesh('round',[.035,.04,.035],sign*(spec.species==='boar'?.27:.19),.11,.24,colors.black,head);
    const legLength=species.bodyY-.22,legWidth=spec.species==='boar'?.12:.075;
    for(let i=0;i<4;i++){
      const pivot=new THREE.Group(),knee=new THREE.Group();pivot.position.set((i%2?1:-1)*(spec.species==='boar'?.3:.23),-.17,i<2?.47:-.47);rig.add(pivot);
      mesh('cylinder',[legWidth,legLength*.52,legWidth],0,-legLength*.26,0,fur,pivot);knee.position.y=-legLength*.52;pivot.add(knee);
      mesh('cylinder',[legWidth*.72,legLength*.48,legWidth*.72],0,-legLength*.24,0,shade,knee);mesh('box',[legWidth*1.6,.11,legWidth*2],0,-legLength*.48,.035,colors.black,knee);
      legs.push({pivot,knee,phase:i===0||i===3?0:Math.PI});
    }
    const a={id:spec.id,species:spec.species,name:species.name,x:spec.x,z:spec.z,y:group.position.y,home:{x:spec.x,z:spec.z},range:clamp(spec.range||45,20,85),heading:group.rotation.y,group,rig,head,legs,health:species.health,maxHealth:species.health,dead:false,harvested:false,mode:'graze',timer:random()*4+1,attackCooldown:0,alert:0,cycle:random()*6,random,target:{x:spec.x,z:spec.z},speed:0,stuck:0};
    group.traverse(o=>{if(o.isMesh){o.userData.wildlifeId=a.id;meshes.push(o);}});
    a.item=interaction('animalHarvest',a.x,a.z,{id:a.id,y:a.y,name:'Gather hide / field rations · '+a.name,group,disabled:true,animal:a});
    animals.push(a);animalById.set(a.id,a);return a;
  }
  function buildTrap(spec){
    const group=new THREE.Group(),jaws=[];group.name='Marked '+(spec.kind==='snare'?'snare':'jaw trap');group.position.set(spec.x,ground(spec.x,spec.z),spec.z);group.rotation.y=spec.heading||0;root.add(group);
    mesh('cylinder',[.57,.055,.57],0,.035,0,colors.bristle,group);mesh('box',[.3,.04,.29],0,.085,0,colors.rust,group);
    for(const sign of [-1,1]){
      const jaw=new THREE.Group();jaw.position.set(sign*.46,.08,0);group.add(jaw);jaws.push(jaw);
      mesh('box',[.08,.085,.76],0,.03,0,colors.rust,jaw);
      for(let j=0;j<5;j++){const tooth=mesh('cone',[.065,.17,.065],-sign*.05,.12,-.3+j*.15,colors.rust,jaw);tooth.rotation.z=sign*.5;}
    }
    if(spec.kind==='snare'){const loop=mesh('ring',[.6,.6,.6],0,.11,0,colors.rope,group);loop.rotation.x=-Math.PI/2;link(group,[.5,.05,0],[1.1,.05,-.6],.018,colors.rope);}
    // Painted stakes and a bright cross make hazards readable before entry.
    for(const sign of [-1,1]){mesh('cylinder',[.035,.65,.035],sign*.78,.31,-.48,colors.rust,group);const flag=mesh('box',[.28,.18,.025],sign*.78,.56,-.46,colors.warning,group);flag.rotation.z=sign*.1;}
    const warning=mesh('box',[.68,.025,.1],0,.085,.84,colors.warning,group);warning.rotation.y=.55;
    const cross=mesh('box',[.68,.025,.1],0,.087,.84,colors.warning,group);cross.rotation.y=-.55;
    const t={id:spec.id,kind:spec.kind||'jaw',x:spec.x,z:spec.z,y:group.position.y,group,jaws,warning,cross,armed:true,warned:false};
    t.item=interaction('trap',t.x,t.z,{id:t.id,y:t.y,name:'Disarm marked '+(t.kind==='snare'?'snare':'jaw trap'),group,disabled:false,trap:t});traps.push(t);trapById.set(t.id,t);return t;
  }
  for(const spec of Array.isArray(plan.animals)?plan.animals.slice(0,32):[])if(validId(spec?.id)&&!animalById.has(spec.id)&&Object.hasOwn(WILDLIFE_SPECIES,spec.species)&&safe(spec.x,spec.z,WILDLIFE_SPECIES[spec.species].radius))buildAnimal(spec);
  for(const spec of Array.isArray(plan.traps)?plan.traps:[])if(validId(spec?.id)&&!trapById.has(spec.id)&&safe(spec.x,spec.z,.6))buildTrap(spec);

  function syncItem(a){a.item.x=a.x;a.item.z=a.z;a.item.y=a.y;a.item.disabled=!a.dead||a.harvested;}
  function poseDead(a){a.rig.position.y=.38;a.rig.rotation.set(0,0,1.48);for(const leg of a.legs){leg.pivot.rotation.x=.2;leg.knee.rotation.x=-.35;}}
  function setTrap(t,armed){t.armed=armed;t.item.disabled=!armed;for(let i=0;i<t.jaws.length;i++)t.jaws[i].rotation.z=armed?0:(i===0?-1:1)*1.22;t.warning.material=t.cross.material=armed?colors.warning:colors.safe;}
  function reset(state){
    currentState=ensureState(state);lastPlayer=null;callbacks={};elapsedTime=Number(state.elapsed)||0;
    const killed=new Set(state.wildlifeKilled),harvested=new Set(state.wildlifeHarvested),cleared=new Set(state.trapsCleared);
    for(const a of animals){
      const body=state.wildlifeBodies[a.id],hasBody=body&&safe(body.x,body.z,.1)&&distance(body,a.home)<a.range*3;
      a.dead=killed.has(a.id)||harvested.has(a.id);a.harvested=harvested.has(a.id);a.health=a.dead?0:a.maxHealth;a.x=a.dead&&hasBody?body.x:a.home.x;a.z=a.dead&&hasBody?body.z:a.home.z;a.y=ground(a.x,a.z);a.group.position.set(a.x,a.y,a.z);a.group.visible=!a.harvested;a.rig.rotation.set(0,0,0);a.rig.position.y=WILDLIFE_SPECIES[a.species].bodyY;a.mode='graze';a.timer=1+a.random()*3;a.attackCooldown=0;a.alert=0;a.speed=0;a.stuck=0;
      for(const leg of a.legs){leg.pivot.rotation.x=0;leg.knee.rotation.x=0;}
      if(a.dead)poseDead(a);syncItem(a);
    }
    for(const t of traps){t.warned=false;t.group.visible=true;setTrap(t,!cleared.has(t.id));}
    root.updateMatrixWorld(true);
  }
  function kill(a){
    if(a.dead||!currentState)return;a.dead=true;a.health=0;a.speed=0;poseDead(a);syncItem(a);
    if(!currentState.wildlifeKilled.includes(a.id))currentState.wildlifeKilled.push(a.id);
    currentState.wildlifeBodies[a.id]={x:a.x,z:a.z};dirty();
  }
  function shoot(raycaster,range=100,obstructionDistance=Infinity,damage=1){
    if(!currentState||!raycaster?.ray||!Number.isFinite(damage)||damage<0)return null;
    const maxDistance=Math.min(Number.isFinite(range)?Math.max(0,range):100,Number.isFinite(obstructionDistance)?Math.max(0,obstructionDistance):Infinity);
    root.updateMatrixWorld(true);
    const targets=meshes.filter(m=>{const a=animalById.get(m.userData.wildlifeId);return a&&!a.dead&&a.group.visible;});
    const hit=raycaster.intersectObjects(targets,false).find(h=>h.distance<=maxDistance);
    if(!hit)return null;const a=animalById.get(hit.object.userData.wildlifeId);if(damage===0)return {animal:a,point:hit.point.clone(),distance:hit.distance,killed:false,damage:0};a.health=Math.max(0,a.health-damage);a.alert=7;a.mode='flee';a.timer=3.5;
    if(a.health<=0)kill(a);return {animal:a,point:hit.point.clone(),distance:hit.distance,killed:a.dead,damage};
  }
  function nearby(item,radius){return lastPlayer&&distance(lastPlayer,item)<=radius&&Math.abs(feet(lastPlayer)-item.y)<1.4;}
  function disarm(item,state=currentState){
    const t=trapById.get(item?.id);if(!t||!t.armed||state!==currentState||!nearby(t,3.4)||distance(lastPlayer,t)<.87)return false;
    if(state.trapsCleared.includes(t.id))return false;state.trapsCleared.push(t.id);setTrap(t,false);dirty();return true;
  }
  function harvest(item,state=currentState){
    const a=animalById.get(item?.id);if(!a||!a.dead||a.harvested||state!==currentState||!nearby(a,3.4)||state.wildlifeHarvested.includes(a.id))return null;
    state.wildlifeHarvested.push(a.id);a.harvested=true;a.group.visible=false;syncItem(a);dirty();
    const species=WILDLIFE_SPECIES[a.species];return {scrap:species.scrap,health:species.healthReward,food:0,water:0};
  }
  function routeClear(ax,az,bx,bz,r=.35){const length=Math.hypot(bx-ax,bz-az),steps=Math.max(1,Math.ceil(length/.65));for(let i=1;i<=steps;i++)if(blocked(ax+(bx-ax)*i/steps,az+(bz-az)*i/steps,r))return false;return true;}
  function chooseWander(a){const angle=a.random()*Math.PI*2,r=5+a.random()*a.range*.7;a.target={x:a.home.x+Math.sin(angle)*r,z:a.home.z+Math.cos(angle)*r};a.timer=4+a.random()*6;}
  function move(a,angle,speed,dt){
    const amount=speed*dt;if(amount<=0)return false;
    const radius=WILDLIFE_SPECIES[a.species].radius,baseY=ground(a.x,a.z);
    for(const turn of [0,.65,-.65,1.25,-1.25]){
      const heading=angle+turn,x=a.x+Math.sin(heading)*amount,z=a.z+Math.cos(heading)*amount;
      if(!safe(x,z,radius)||distance({x,z},a.home)>a.range*2||Math.abs(ground(x,z)-baseY)>Math.max(.17,amount*.9)||!routeClear(a.x,a.z,x,z,radius))continue;
      a.x=x;a.z=z;a.y=ground(x,z);a.heading=heading;a.group.position.set(x,a.y,z);const delta=Math.atan2(Math.sin(heading-a.group.rotation.y),Math.cos(heading-a.group.rotation.y));a.group.rotation.y+=delta*Math.min(1,dt*9);return true;
    }
    a.timer=Math.min(a.timer,.5);return false;
  }
  function animate(a,dt){
    const species=WILDLIFE_SPECIES[a.species],running=a.speed>species.walk*1.5;a.cycle+=dt*(running?12:5)*Math.min(1,a.speed);const swing=a.speed>.05?(running?.7:.3):0;
    for(const leg of a.legs){const step=Math.sin(a.cycle+leg.phase);leg.pivot.rotation.x=step*swing;leg.knee.rotation.x=Math.max(0,-step)*swing*.75;}
    a.rig.position.y=species.bodyY+(running?Math.abs(Math.sin(a.cycle))*.07:0);a.head.rotation.x=a.mode==='graze'?Math.sin(elapsedTime*.8+a.cycle)*.08+.18:-.04;
    const front=ground(a.x+Math.sin(a.heading)*.6,a.z+Math.cos(a.heading)*.6),back=ground(a.x-Math.sin(a.heading)*.6,a.z-Math.cos(a.heading)*.6);a.rig.rotation.x=clamp(-Math.atan2(front-back,1.2),-.35,.35);
  }
  function update(dt,{player,elapsed,noise=0,onDamage,onMessage,onDirty}={}){
    if(!currentState||!player||!Number.isFinite(dt)||dt<=0)return;
    dt=Math.min(dt,.1);lastPlayer=player;callbacks={onDamage,onMessage,onDirty};elapsedTime=Number.isFinite(elapsed)?elapsed:elapsedTime+dt;
    for(const t of traps){
      const d=distance(t,player);t.group.visible=d<155;if(!t.armed)continue;
      const low=Math.abs(feet(player)-t.y)<.75;
      if(d<4.5&&low&&!t.warned){message('MARKED TRAP · approach carefully, press E to disarm');t.warned=true;}else if(d>9)t.warned=false;
      if(d<.84&&low){if(!currentState.trapsCleared.includes(t.id))currentState.trapsCleared.push(t.id);setTrap(t,false);if(typeof onDamage==='function')onDamage(t.kind==='snare'?12:20,'trap');message('TRAP SPRUNG · the mechanism is now safe');dirty();}
    }
    for(const a of animals){
      const d=distance(a,player);a.group.visible=!a.harvested&&d<175;if(a.dead||d>125)continue;
      const species=WILDLIFE_SPECIES[a.species],sameLevel=Math.abs(feet(player)-a.y)<2.2;
      const heard=typeof noise==='number'?noise>0&&d<65:noise&&Number.isFinite(noise.x)&&Number.isFinite(noise.z)&&distance(a,noise)<(noise.radius||65);
      a.timer-=dt;a.attackCooldown=Math.max(0,a.attackCooldown-dt);a.alert=Math.max(0,a.alert-dt);
      if(heard)a.alert=Math.max(a.alert,3.5);
      let heading=Math.atan2(a.target.x-a.x,a.target.z-a.z),speed=species.walk;
      const away=Math.atan2(a.x-player.x,a.z-player.z),toward=away+Math.PI;
      if(a.species==='deer'&&(d<20||a.alert>0)){
        a.mode='flee';heading=away;speed=species.run;a.timer=1.2;
      }else if(a.species==='boar'&&sameLevel&&d<14){
        if(a.alert>0&&a.health<35){a.mode='flee';heading=away;speed=species.run;}
        else if(d<7||a.alert>0){a.mode='charge';heading=toward;speed=a.attackCooldown>1.2?0:species.run;}
        else{a.mode='watch';heading=toward;speed=0;a.group.rotation.y=toward;}
      }else if(a.species==='wolf'&&sameLevel&&d<18){
        if(a.attackCooldown>1.8||a.health<22){a.mode='retreat';heading=away+(a.random()-.5)*.3;speed=species.run*.7;}
        else if(d<3.5||a.alert>0||a.timer<0){a.mode='attack';heading=toward;speed=species.run;}
        else {a.mode='stalk';heading=toward+(d<7?1.4:.3);speed=d<6?species.walk*.6:species.walk*1.4;}
      }else{
        if(a.timer<=0||distance(a,a.target)<1.5){a.mode=a.random()<.45?'graze':'wander';chooseWander(a);}
        heading=Math.atan2(a.target.x-a.x,a.target.z-a.z);speed=a.mode==='graze'?0:species.walk;
        if(distance(a,a.home)>a.range){heading=Math.atan2(a.home.x-a.x,a.home.z-a.z);speed=species.walk*1.5;a.mode='wander';}
      }
      if((a.mode==='attack'||a.mode==='charge')&&d<1.7&&sameLevel&&a.attackCooldown<=0&&routeClear(a.x,a.z,player.x,player.z,.25)){
        a.attackCooldown=a.species==='wolf'?3.8:2.8;a.timer=3.5;if(typeof onDamage==='function')onDamage(species.damage,'wildlife');
      }
      if(a.mode==='flee'&&distance(a,a.home)>a.range*1.8)heading=away+Math.PI/2;
      a.speed=move(a,heading,speed,dt)?speed:0;animate(a,dt);syncItem(a);
    }
  }
  return {animals,traps,reset,update,shoot,disarm,harvest};
}
