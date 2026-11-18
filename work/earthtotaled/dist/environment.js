// Environmental detail for the same connected New Carthage map.
// The module has no state, network dependencies, or gameplay rewards.
export function enhanceEnvironment({THREE,scene,mats,box,cylinder,blocker,addSign,rnd,isClear,createTree}) {
  const root=new THREE.Group();root.name='New Carthage / weathered district detail';scene.add(root);
  const safePoints=[[4,65],[6,59],[-43,28],[0,65],[-51,19],[49,-29],[-50,-65],[0,-69.7],[-13,49],[-45,31],[-63,11],[17,21],[59,-20],[39,-36],[-15,-28],[-42,-57],[13,-69],[4,72],[-62,-38]];
  const safe=(x,z,r=3)=>safePoints.every(p=>Math.hypot(x-p[0],z-p[1])>r);
  const material=(color,roughness=1,metalness=0)=>new THREE.MeshStandardMaterial({color,roughness,metalness});
  const stone=material(0x979486),pavement=material(0x85877a),rust=material(0x886044,.93,.12),blacktop=material(0x242d29),brick=material(0x7c6150),canvas=material(0x777e60),bluePaint=material(0x668b83,.78,.16),glass=material(0x354c45,.31,.42),dryLeaf=material(0x69734b),chalk=material(0xc1bc9d);
  const lamp=new THREE.MeshStandardMaterial({color:0xb6ddd0,emissive:0x8db9a7,emissiveIntensity:.45});
  const identity=new THREE.Object3D();
  let detailInstances=0;
  function texture(kind){
    const c=document.createElement('canvas');c.width=c.height=512;const ctx=c.getContext('2d');
    const palette={concrete:'#b4afa2',asphalt:'#575e58',brick:'#927560',pavers:'#9b9d8f',metal:'#88715c'};
    ctx.fillStyle=palette[kind]||palette.concrete;ctx.fillRect(0,0,512,512);
    for(let i=0;i<19000;i++){const value=Math.floor(rnd(40,180));ctx.fillStyle=`rgba(${value},${value-3},${value-9},${rnd(.03,.16)})`;ctx.fillRect(rnd(0,512),rnd(0,512),rnd(.5,3),rnd(.5,4));}
    if(kind==='concrete'){
      // Rain and rust streaks break up the formerly flat building fronts.
      for(let i=0;i<45;i++){ctx.fillStyle=i%4===0?'#584b3520':'#343f3520';ctx.fillRect(rnd(0,512),rnd(0,260),rnd(1,9),rnd(40,290));}
      ctx.fillStyle='#35483c33';ctx.fillRect(0,460,512,52);
      ctx.strokeStyle='#5b5f514b';ctx.lineWidth=2;for(const x of [0,256,511]){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,512);ctx.stroke();}for(const y of [0,256,511]){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(512,y);ctx.stroke();}
      for(let i=0;i<14;i++){ctx.fillStyle='#d1c6ad50';ctx.fillRect(rnd(0,512),rnd(0,512),rnd(4,17),rnd(3,15));}
    }
    if(kind==='brick'||kind==='pavers'){
      const row=kind==='brick'?40:128,col=kind==='brick'?100:128;ctx.strokeStyle=kind==='brick'?'#332f265e':'#4f594752';ctx.lineWidth=kind==='brick'?4:3;
      for(let y=0;y<=512;y+=row){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(512,y);ctx.stroke();for(let x=-(y/row%2)*col/2;x<=512;x+=col){ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(x,y+row);ctx.stroke();}}
    }
    if(kind==='asphalt'||kind==='concrete'||kind==='pavers'){
      for(let i=0;i<(kind==='asphalt'?7:3);i++){ctx.strokeStyle=kind==='asphalt'?'#18251fa6':'#3c49395b';ctx.lineWidth=rnd(.5,2.8);let x=rnd(0,512),y=rnd(0,512);ctx.beginPath();ctx.moveTo(x,y);for(let j=0;j<11;j++){x+=rnd(-14,14);y+=rnd(5,23);ctx.lineTo(x,y);if(j===5){ctx.lineTo(x+rnd(-26,26),y+22);ctx.moveTo(x,y);}}ctx.stroke();}
    }
    const map=new THREE.CanvasTexture(c);map.colorSpace=THREE.SRGBColorSpace;map.wrapS=map.wrapT=THREE.RepeatWrapping;map.anisotropy=4;return map;
  }
  const concreteTex=texture('concrete'),asphaltTex=texture('asphalt'),brickTex=texture('brick'),pavingTex=texture('pavers');
  concreteTex.repeat.set(3,5);pavingTex.repeat.set(1,1);brickTex.repeat.set(2,2);
  mats.concrete.map=concreteTex;mats.concrete.color.setHex(0xc4c4b6);mats.concrete.roughness=1;mats.concrete.needsUpdate=true;
  mats.wall.map=concreteTex;mats.wall.color.setHex(0xd3c8af);mats.wall.needsUpdate=true;
  brick.map=brickTex;pavement.map=pavingTex;
  // Each road gets world-sized texture repetition, avoiding stretched asphalt.
  const roads=[];scene.traverse(o=>{if(o.isMesh&&o.material===mats.road)roads.push(o);});
  for(const road of roads){const map=asphaltTex.clone();map.repeat.set((road.geometry.parameters.width||1)/5,(road.geometry.parameters.depth||1)/5);map.needsUpdate=true;road.material=material(0xa3aaa0);road.material.map=map;road.material.needsUpdate=true;}
  function instanceBoxes(items,mat,cast=true){if(!items.length)return;const mesh=new THREE.InstancedMesh(new THREE.BoxGeometry(1,1,1),mat,items.length);mesh.castShadow=cast;mesh.receiveShadow=true;for(let i=0;i<items.length;i++){const b=items[i];identity.position.set(b[0],b[1],b[2]);identity.scale.set(b[3],b[4],b[5]);identity.rotation.set(b[6]||0,b[7]||0,b[8]||0);identity.updateMatrix();mesh.setMatrixAt(i,identity.matrix);}root.add(mesh);detailInstances+=items.length;return mesh;}
  const slabs=[],curbs=[],gutters=[],drainBars=[],roadMarks=[];
  // Pavements stop at junctions and never add collision across a route.
  for(const x of [-11.2,11.2])for(let z=-82;z<83;z+=2){if([20,-28,-65].some(j=>Math.abs(z-j)<9)||!safe(x,z,2.6))continue;slabs.push([x,.055,z,2.5,.09,1.96]);curbs.push([x+(x<0?1.27:-1.27),.12,z,.18,.23,1.96]);gutters.push([x+(x<0?1.48:-1.48),.043,z,.27,.018,1.96]);}
  for(const z of [11.5,28.5,-36.5,-19.5,-72,-58])for(let x=-81;x<82;x+=2){if(Math.abs(x)<12||Math.abs(x+51)<9||Math.abs(x-49)<9||!safe(x,z,2.6)||!isClear(x,z,.2))continue;slabs.push([x,.055,z,1.96,.09,1.2]);}
  for(const x of [-59,-43,41,57])for(let z=-56;z<45;z+=2){if([20,-28].some(j=>Math.abs(z-j)<10)||!safe(x,z,3)||!isClear(x,z,.2))continue;curbs.push([x,.1,z,.18,.18,1.96]);}
  for(const [x,z] of [[-9,46],[9,35],[-9,-42],[9,-53],[-48,37],[45,5]]){gutters.push([x,.049,z,1.1,.024,.65]);for(let j=0;j<7;j++)drainBars.push([x-.46+j*.15,.066,z,.055,.022,.6]);}
  for(const [x,z] of [[-48,3],[46,-10],[-6,-47],[5,52]]){roadMarks.push([x,.061,z,2.5,.016,.13]);roadMarks.push([x,.061,z-2.5,2.5,.016,.13]);roadMarks.push([x-1.2,.061,z-1.25,.13,.016,2.5]);}
  instanceBoxes(slabs,pavement,false);instanceBoxes(curbs,stone);instanceBoxes(gutters,blacktop,false);instanceBoxes(drainBars,mats.metal,false);instanceBoxes(roadMarks,chalk,false);
  // Fragmented patches, pothole edges, papers, and tiny debris vary the streets.
  const debris=[],papers=[],cracks=[];
  for(let i=0;i<220;i++){const x=rnd(-82,82),z=rnd(-83,83);if(!safe(x,z,3.4)||!isClear(x,z,.4))continue;const onRoad=Math.abs(x)<9||Math.abs(z-20)<7||Math.abs(z+28)<7||Math.abs(z+65)<6||Math.abs(x+51)<7||Math.abs(x-49)<7;if(onRoad){if(i%3===0)cracks.push([x,.071,z,rnd(.02,.05),.008,rnd(.5,1.9),0,rnd(0,6.28)]);if(i%8===0)papers.push([x,.08,z,.22,.008,.34,0,rnd(0,6.28)]);}else debris.push([x,.06,z,rnd(.12,.5),rnd(.08,.2),rnd(.15,.6),rnd(-.15,.15),rnd(0,6.28),rnd(-.15,.15)]);}
  instanceBoxes(debris,stone);instanceBoxes(papers,chalk,false);instanceBoxes(cracks,blacktop,false);
  for(const [x,z,r] of [[2,44,1.2],[-7,-42,.8],[34,20,1.1],[-50,-3,.9],[46,-58,.65]]){const hole=cylinder(r,r,.018,x,.061,z,blacktop,root,11);hole.rotation.y=rnd(0,6.28);for(let j=0;j<7;j++){const a=j/7*Math.PI*2;const p=box(.2,.075,.36,x+Math.cos(a)*r,.075,z+Math.sin(a)*r,stone,root);p.rotation.y=a;}}
  // Ground-floor scale: awnings, lintels, shutters, posters, and boarded glass.
  const storefronts=[
    [31,69.07,0,'CARTHAGE MART'],
    [-28,8.57,0,'OLD LINE / TICKETS'],[28,9.57,0,'ASH AVE. WORKS'],
    [-72,-37.93,0,'CITY WATER'],[28,-37.43,0,'SEED CONSERVATORY'],
    [-42.07,58,-Math.PI/2,'STATION 09'],[42.07,58,Math.PI/2,'NO ENTRY']
  ];
  const boards=[];
  for(const [x,z,rotation,label] of storefronts){const g=new THREE.Group();g.position.set(x,0,z);g.rotation.y=rotation;root.add(g);box(7.8,2.9,.07,0,1.6,0,brick,g);box(2.1,2.6,.1,-2.4,1.35,.06,mats.dark,g);box(3.8,2.2,.12,1.05,1.55,.08,glass,g);box(8.2,.22,.7,0,3.17,.27,stone,g);box(1.4,.14,.5,-2.4,.1,.25,stone,g);for(const xx of [-.4,.8,2]){box(.1,2.2,.16,xx,1.55,.17,mats.metal,g);}for(let i=0;i<3;i++){const b=box(4.1,.19,.12,1.05,.8+i*.65,.24,mats.bark,g);b.rotation.z=i%2?.12:-.08;}const sign=addSign(label,x,3.65,z,7,.6,'#c5c5a8');sign.rotation.y=rotation;sign.position.x+=Math.sin(rotation)*.1;sign.position.z+=Math.cos(rotation)*.1;}
  // Exposed slabs and localized collapse tell a more convincing ruined-city story.
  for(const [x,z,rotation] of [[19.7,-2,-Math.PI/2],[-17.7,-46,Math.PI/2],[62.7,-48,-Math.PI/2]]){
    const g=new THREE.Group();g.position.set(x,0,z);g.rotation.y=rotation;root.add(g);box(6,6,.07,0,5,.12,mats.dark,g);for(let k=0;k<3;k++){const slab=box(6.5,.23,1.7,0,2.1+k*2.4,.6,stone,g);slab.rotation.z=(k-1)*.025;}for(const xx of [-2.8,2.8])box(.42,7,.55,xx,4,.3,mats.concrete,g);for(let j=0;j<8;j++){const rod=box(.035,rnd(1,2.5),.035,rnd(-2.8,2.8),8,.6,mats.rust,g);rod.rotation.z=rnd(-.25,.25);}}
  // A stranded transit bus becomes a proper piece of cover, off the main routes.
  blocker(-65,40,8.6,2.5,2.6,1.3,rust);box(8.6,.2,2.65,-65,2.7,40,mats.wall,root);for(const z of [38.72,41.28]){for(let x=-68.5;x<-61;x+=1.35){box(1.05,.85,.04,x,1.95,z,glass,root);box(.06,1.1,.06,x-.58,1.9,z,mats.dark,root);}box(8.3,.16,.08,-65,.85,z,bluePaint,root);}
  for(const x of [-67.7,-62.3])for(const z of [38.7,41.3]){const wheel=cylinder(.55,.55,.23,x,.5,z,mats.black,root,10);wheel.rotation.x=Math.PI/2;}
  addSign('09  /  LAST SERVICE',-65,2.95,41.35,5.8,.47,'#c9bc9b');
  // Closed loading container and pallets in the eastern service yard.
  blocker(70,35,9,3.1,2.7,1.35,bluePaint);const ribs=[];for(let x=65.7;x<74.5;x+=.45)ribs.push([x,1.35,36.59,.07,2.5,.08]);instanceBoxes(ribs,mats.metal);addSign('CIVIL DEFENCE / 2048',70,1.8,36.66,6.5,.6,'#ced6c2');
  // Transit district: route boards and a ruined platform canopy.
  for(const x of [-60,-55])cylinder(.08,.08,3.4,x,1.7,31,mats.metal,root,6);const canopy=box(6.4,.12,2.8,-57.5,3.45,31,bluePaint,root);canopy.rotation.z=.045;box(3.3,.14,.55,-57.5,.65,31.3,mats.bark,root);box(.16,.7,.45,-58.8,.3,31.3,mats.metal,root);box(.16,.7,.45,-56.2,.3,31.3,mats.metal,root);addSign('OLD TRANSIT / WEST',-57.5,2.6,31.04,4.4,.55,'#e1c49d');
  const routeBoard=addSign('01 > 09 > 17 > —',-57.5,1.85,31.08,3.8,.44,'#8eaaa0');
  // Botanical district: dry irrigation beds and the surviving greenhouse frame.
  const green=new THREE.Group();green.position.set(67,0,-28);root.add(green);const greenhouseFrame=[];
  for(const x of [-4,4])for(const z of [-4,0,4]){greenhouseFrame.push([67+x,1.8,-28+z,.1,3.6,.1]);}
  greenhouseFrame.push([67,3.7,-32,8.2,.1,.1],[67,3.7,-24,8.2,.1,.1],[67,4.8,-28,.12,.12,8.2]);instanceBoxes(greenhouseFrame,mats.metal);
  for(const z of [-4,0,4])for(const side of [-1,1]){const beam=box(4.5,.1,.1,side*2,4.25,z,mats.metal,green);beam.rotation.z=side*-.265;}
  for(const x of [-2.3,2.3]){box(1.6,.35,6.5,x,.18,0,stone,green);box(1.4,.03,6.3,x,.37,0,mats.bark,green);for(let z=-2.5;z<3;z+=.75){cylinder(.025,.04,.55,x,.65,z,dryLeaf,green,4);}}
  addSign('BOTANICAL / IRRIGATION OFFLINE',67,2.8,-23.9,7.2,.6,'#d2c98d');
  // Observatory district: grounded solar panels and utility cabling.
  const solar=new THREE.MeshStandardMaterial({color:0x374a50,roughness:.48,metalness:.65});
  for(const [x,z,a] of [[-43,-79,.16],[-49,-79,-.1],[-55,-78,.25]]){box(.25,1.2,.25,x,.6,z,mats.metal,root);const panel=box(4,.09,2.5,x,1.35,z,solar,root);panel.rotation.x=-.4;panel.rotation.z=a;for(let j=-1;j<=1;j++){const strip=box(.035,.02,2.5,x+j*1.25,1.42,z,mats.metal,root);strip.rotation.x=-.4;strip.rotation.z=a;}}
  addSign('OBSERVATORY / ARRAY 03',-49,2.7,-77,6.4,.65,'#b9c5d0');
  // The shelter is inhabited now: patched cloth, a triage tent, storage and lights.
  blocker(-12,72,4.3,4.6,2.3,1.15,canvas);const tentRoof=box(4.7,.17,5,-12,2.43,72,canvas,root);tentRoof.rotation.z=.11;box(1.25,1.9,.08,-12,1,69.64,mats.dark,root);addSign('TRIAGE / SEALED',-12,2.1,69.57,3.5,.4,'#d8d6be');
  const safeSign=addSign('BUNKER 09 / SAFE HAVEN',0,4.6,68.4,7.5,.65,'#b6e2ca');
  for(const x of [-6,10]){box(.1,3.5,.1,x,1.75,66.5,mats.metal,root);box(.5,.18,.25,x,3.5,66.5,lamp,root);}
  const awning=box(12,.08,4,0,3.7,68,canvas,root);awning.rotation.z=.025;
  for(const [x,z] of [[-5,74.5],[-5.8,74.5]]){cylinder(.34,.34,.85,x,.42,z,rust,root,10);cylinder(.35,.35,.05,x,.88,z,mats.dark,root,10);}
  addSign('RAW / FILTER FIRST',-5.4,1.15,74.1,2,.35,'#d4be93');
  // A muted cyan banner makes the friendly-machine identity legible in the world.
  box(.08,4.4,.08,11,2.2,59,stone,root);box(1.6,2.1,.04,11.85,3.2,59,bluePaint,root);addSign('GREENLINE',11.8,3.75,59.05,1.5,.3,'#dcf1d9');addSign('REPAIR / NOT RULE',11.8,2.9,59.05,1.5,.24,'#dcf1d9');
  // No decorative prop here contains drinkable water or emits loot.
  return {root,detailInstances,textureCount:4,landmarks:['shelter camp','transit bus stop','botanical greenhouse','observatory solar array']};
}
