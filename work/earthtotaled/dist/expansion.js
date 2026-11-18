export const EXPANSION_LIMIT = 176;

// A physical extension of the existing city. Doors are gaps between wall
// colliders, and upper levels are actual support surfaces, never room teleports.
export function createExpansion({THREE,scene,mats,box,cylinder,blocker,addSign,rnd,addInteractable,createCrate,createAlly}) {
  const buildings=[],platforms=[],climbRoutes=[];
  const settlements=[
    {id:'riverside',name:'Riverside Commons',x:-125,z:55,radius:26,npcId:'ada',water:{x:-120,z:56.2}},
    {id:'cistern',name:'Cistern Market',x:110,z:105,radius:25,npcId:'malik',water:{x:116,z:108.2}}
  ];
  const spawns=[[-110,-63,'walker'],[-148,-20,'drone'],[-154,112,'heavy'],[117,-75,'walker'],[158,-72,'drone'],[148,65,'walker'],[-25,143,'walker'],[60,149,'heavy'],[124,157,'drone'],[48,-131,'heavy'],[-53,-127,'walker'],[150,-113,'drone']];
  const root=new THREE.Group();root.name='New Carthage / outer districts and interiors';scene.add(root);
  const makeMat=(color,roughness=.95,metalness=0)=>new THREE.MeshStandardMaterial({color,roughness,metalness});
  const mortar=makeMat(0x9b9c8c),wood=makeMat(0x6e6451),fabric=makeMat(0x737c63),cream=makeMat(0xa9a58f),tile=makeMat(0x9ba697),blue=makeMat(0x527c7a,.8,.12),glass=makeMat(0x354f49,.35,.4),soil=makeMat(0x434d37),rust=makeMat(0x8b6449,.9,.15),roofMat=makeMat(0x4d5b50),signPost=makeMat(0x647265,.7,.2);
  const warmLight=new THREE.MeshStandardMaterial({color:0xd8dcaa,emissive:0xd4c497,emissiveIntensity:.7});
  const dark=makeMat(0x2b382f),waterMat=new THREE.MeshStandardMaterial({color:0x4e6757,roughness:.38,metalness:.35,transparent:true,opacity:.82});
  const dummy=new THREE.Object3D();
  function instances(rows,material,cast=false){if(!rows.length)return;const mesh=new THREE.InstancedMesh(new THREE.BoxGeometry(1,1,1),material,rows.length);mesh.castShadow=cast;mesh.receiveShadow=true;for(let i=0;i<rows.length;i++){const p=rows[i];dummy.position.set(p[0],p[1],p[2]);dummy.scale.set(p[3],p[4],p[5]);dummy.rotation.set(p[6]||0,p[7]||0,p[8]||0);dummy.updateMatrix();mesh.setMatrixAt(i,dummy.matrix);}root.add(mesh);}
  function floor(id,x,z,w,d,y,material=roofMat){blocker(x,z,w,d,.18,y-.09,material);const p={id,x,z,w,d,y};platforms.push(p);return p;}
  function roadTexture(){const canvas=document.createElement('canvas');canvas.width=canvas.height=256;const ctx=canvas.getContext('2d');ctx.fillStyle='#646b5f';ctx.fillRect(0,0,256,256);for(let i=0;i<5000;i++){const c=Math.floor(rnd(30,150));ctx.fillStyle=`rgba(${c},${c+4},${c},.12)`;ctx.fillRect(rnd(0,256),rnd(0,256),rnd(.5,3),rnd(.5,3));}for(let i=0;i<6;i++){let x=rnd(0,256),y=rnd(0,256);ctx.beginPath();ctx.moveTo(x,y);for(let j=0;j<8;j++){x+=rnd(-12,12);y+=rnd(8,25);ctx.lineTo(x,y);}ctx.strokeStyle='#202d2480';ctx.lineWidth=rnd(.8,2.2);ctx.stroke();}const t=new THREE.CanvasTexture(canvas);t.colorSpace=THREE.SRGBColorSpace;t.wrapS=t.wrapT=THREE.RepeatWrapping;t.anisotropy=4;return t;}
  const asphalt=roadTexture(),roadLines=[],pavements=[];
  function road(x,z,w,d){const mat=makeMat(0x9a9f90);mat.map=asphalt.clone();mat.map.repeat.set(w/5,d/5);mat.map.needsUpdate=true;box(w,.04,d,x,.025,z,mat,root);if(w>d){for(let xx=x-w/2+2;xx<x+w/2;xx+=7)roadLines.push([xx,.053,z,2.8,.012,.12]);for(const edge of [-1,1])pavements.push([x,.06,z+edge*(d/2+.85),w,.1,1.6]);}else{for(let zz=z-d/2+2;zz<z+d/2;zz+=7)roadLines.push([x,.053,zz,.12,.012,2.8]);for(const edge of [-1,1])pavements.push([x+edge*(w/2+.85),.06,z,1.6,.1,d]);}}
  // Three clear exits through the former perimeter connect the new districts.
  road(-96,20,22,14);road(-103,37,12,138);road(-125,55,44,11);road(-118,42,30,9);road(-125,-17,44,9);
  road(97,-28,22,14);road(98,38,12,145);road(126,-7,56,10);road(136,-24,8,32);road(144,-23,16,8);
  road(20,98,12,22);road(67,105,106,12);road(115,105,30,13);road(111,119,18,9);
  instances(pavements,mortar);instances(roadLines,cream);
  // A shallow, visibly contaminated river defines the western district edge.
  box(8,.025,250,-171,.018,29,waterMat,root);box(1.1,.6,250,-166.4,.2,29,mats.concrete,root);
  addSign('RIVER WATER / CONTAMINATED',-162,3.4,56,6.5,.65,'#d6ba83');
  for(const [label,x,z] of [['RIVERSIDE COMMONS  <',-91,20],['EAST INDUSTRIAL  >',91,-28],['CISTERN MARKET  >',23,96]]){cylinder(.085,.085,4,x,2,z,signPost,root);addSign(label,x,3.6,z+.1,6.8,.7,'#d7d4aa');}

  const normals={south:{x:0,z:1},north:{x:0,z:-1},east:{x:1,z:0},west:{x:-1,z:0}};
  function room(spec){
    const {id,x,z,w,d,h,front='south',label,style='store'}=spec,normal=normals[front],side={x:normal.z,z:-normal.x};
    const floorY=.08,roofY=h+.2,thick=.44,doorWidth=6.4,doorHeight=3.2;
    const across=Math.abs(normal.z)?w:d,deep=Math.abs(normal.z)?d:w;
    const world=(u,v,y=floorY)=>({x:x+side.x*u-normal.x*v,z:z+side.z*u-normal.z*v,y});
    const wall=(wx,wz,ww,dd,hh=h,yy=hh/2)=>blocker(wx,wz,ww,dd,hh,yy,mats.wall);
    for(const which of ['south','north','east','west']){
      if(which===front)continue;
      if(which==='south')wall(x,z+d/2,w+thick,thick);
      if(which==='north')wall(x,z-d/2,w+thick,thick);
      if(which==='east')wall(x+w/2,z,thick,d+thick);
      if(which==='west')wall(x-w/2,z,thick,d+thick);
    }
    const frontX=x+normal.x*deep/2,frontZ=z+normal.z*deep/2,wing=(across-doorWidth)/2;
    for(const sign of [-1,1]){const wx=frontX+side.x*sign*(doorWidth/2+wing/2),wz=frontZ+side.z*sign*(doorWidth/2+wing/2);wall(wx,wz,Math.abs(normal.z)?wing:thick,Math.abs(normal.z)?thick:wing);}
    // The lintel starts above a standing survivor's full clearance.
    wall(frontX,frontZ,Math.abs(normal.z)?doorWidth:thick,Math.abs(normal.z)?thick:doorWidth,h-doorHeight,doorHeight+(h-doorHeight)/2);
    floor(id+'_ground',x,z,w-.25,d-.25,floorY,style==='clinic'?tile:mortar);
    floor(id+'_roof',x,z,w+.5,d+.5,roofY,roofMat);
    // Parapets define the roof edge; every ladder exits at least 2.5m inside.
    blocker(x,z-d/2,w,.28,.5,roofY+.25,mats.concrete);blocker(x,z+d/2,w,.28,.5,roofY+.25,mats.concrete);
    blocker(x-w/2,z,.28,d,.5,roofY+.25,mats.concrete);blocker(x+w/2,z,.28,d,.5,roofY+.25,mats.concrete);
    const entrance=addSign(label,frontX+normal.x*.3,h-.55,frontZ+normal.z*.3,Math.min(across-1,13),.85,style==='clinic'?'#d6eadc':'#ddd7b3');entrance.rotation.y=Math.atan2(normal.x,normal.z);
    const open=addSign('OPEN / SEARCH INSIDE',frontX+normal.x*.33,2.7,frontZ+normal.z*.33,4.4,.4,'#b6ddaa');open.rotation.y=entrance.rotation.y;
    const porch=world(0,-deep/2-.65);box(Math.abs(normal.z)?doorWidth+1:1.3,.12,Math.abs(normal.z)?1.3:doorWidth+1,porch.x,.06,porch.z,mortar,root);
    // Window frames sit on solid exterior walls, while the actual doorway stays open.
    for(const u of [-across*.35,across*.35]){const p=world(u,-deep/2-.26,2.1);const g=box(Math.abs(normal.z)?2.1:.07,1.45,Math.abs(normal.z)?.07:2.1,p.x,p.y,p.z,glass,root);}
    function furnish(u,v,lw,ld,height,material=wood){const p=world(u,v);const ww=Math.abs(normal.z)?lw:ld,dd=Math.abs(normal.z)?ld:lw;return blocker(p.x,p.z,ww,dd,height,floorY+height/2,material);}
    // Leave a wide central aisle so the door leads into real navigable room space.
    const shelf=furnish(-across*.34,deep*.13,1.15,Math.min(5,deep*.32),2.0,wood);
    const shelfZ=world(-across*.34,deep*.13);for(const yy of [.55,1.1,1.65])box(Math.abs(normal.z)?1.3:Math.min(5.15,deep*.33),.08,Math.abs(normal.z)?Math.min(5.15,deep*.33):1.3,shelfZ.x,floorY+yy,shelfZ.z,mats.metal,root);
    furnish(across*.32,deep*.15,2.0,Math.min(4,deep*.28),style==='clinic'?.7:.8,style==='clinic'?tile:fabric);
    furnish(0,deep*.34,3.8,1.35,.82,style==='workshop'?mats.metal:wood);
    const desk=world(0,deep*.34,1.12);box(.7,.5,.12,desk.x,desk.y,desk.z,mats.black,root);box(.55,.32,.025,desk.x,desk.y+.04,desk.z+.07,mats.cyan,root);
    const lampPosition=world(0,deep*.12,h-.5);box(2.5,.1,.35,lampPosition.x,lampPosition.y,lampPosition.z,warmLight,root);
    const light=new THREE.PointLight(style==='clinic'?0xcde8d2:0xf0d4a3,4,18,2);light.position.set(x,2.8,z);root.add(light);
    // Scattered books and a rug give floors scale without adding invisible walls.
    const rug=world(0,-deep*.12);box(4,.008,3,rug.x,floorY+.006,rug.z,style==='clinic'?blue:fabric,root);
    for(let i=0;i<5;i++){const p=world(rnd(-across*.25,across*.25),rnd(-deep*.2,deep*.15));const paper=box(.22,.009,.31,p.x,floorY+.012,p.z,cream,root);paper.rotation.y=rnd(0,6.28);}
    const threshold={x:frontX,z:frontZ,y:floorY},outside={x:frontX+normal.x*3.5,z:frontZ+normal.z*3.5,y:floorY},inside={x:frontX-normal.x*3.5,z:frontZ-normal.z*3.5,y:floorY};
    const result={id,x,z,w,d,width:w,depth:d,h,floorY,roofY,interior:{x,z,y:floorY},door:{outside,threshold,inside,normal,width:doorWidth,height:doorHeight},world};buildings.push(result);return result;
  }
  function cache(id,x,z,y=.08,reward=0,name='Search indoor supply locker'){
    const group=new THREE.Group();group.position.set(x,y,z);root.add(group);box(1.35,.72,.88,0,.36,0,mats.metal,group);box(1.42,.12,.96,0,.78,0,wood,group);const strip=box(.8,.07,.035,0,.52,.46,reward?mats.cyan:mats.light,group);box(.09,.65,.94,-.47,.4,0,rust,group);box(.09,.65,.94,.47,.4,0,rust,group);
    return addInteractable('interiorLoot',x,z,{id,y,name,group,strip,reward,water:1});
  }
  function questObject(id,x,z,y,name,color=mats.cyan){const group=new THREE.Group();group.position.set(x,y,z);root.add(group);box(.62,.32,.44,0,.19,0,mats.metal,group);box(.25,.06,.47,0,.39,0,color,group);const item=addInteractable('questItem',x,z,{id,y,name,group});return item;}
  function ladder({id,name,x,z,bottom=0,top,exitX,exitZ,axis='z',requires='climb',drawX=x,drawZ=z}){
    const height=top-bottom,railMaterial=mats.metal,rows=[];
    if(axis==='x'){for(const zz of [-.57,.57])box(.09,height+.75,.09,drawX,bottom+height/2+.3,drawZ+zz,railMaterial,root);for(let yy=bottom+.28;yy<top+.2;yy+=.34)rows.push([drawX,yy,drawZ,.09,.065,1.2]);}
    else {for(const xx of [-.57,.57])box(.09,height+.75,.09,drawX+xx,bottom+height/2+.3,drawZ,railMaterial,root);for(let yy=bottom+.28;yy<top+.2;yy+=.34)rows.push([drawX,yy,drawZ,1.2,.065,.09]);}
    instances(rows,railMaterial,true);const plate=addSign(requires==='mantle'?'SCRAMBLE UP':'LADDER / ROOF ACCESS',x,bottom+1.3,z,2.5,.34,'#d3e8b4');if(axis==='x')plate.rotation.y=x>exitX?Math.PI/2:-Math.PI/2;
    const route={id,name,x,z,bottom,top,exitX,exitZ,requires,bottomPoint:{x,z,y:bottom},topPoint:{x:exitX,z:exitZ,y:top}};climbRoutes.push(route);return route;
  }
  const clinic=room({id:'riverside_clinic',x:-126,z:27,w:22,d:18,h:4.8,front:'south',label:'RIVERSIDE / COMMUNITY CLINIC',style:'clinic'});
  cache(100,-122,23,.08,0,'Search clinic medical locker');questObject('medicine',-132,23,.08,'Recover sealed medicine for Ada');
  ladder({id:'clinic_roof',name:'Climb clinic fire ladder',x:-139,z:25,top:clinic.roofY,exitX:-133.5,exitZ:25,axis:'x',drawX:-137.7,drawZ:25});
  const commons=room({id:'riverside_hall',x:-126,z:78,w:26,d:18,h:5.1,front:'north',label:'RIVERSIDE COMMONS / ALL WELCOME',style:'hall'});
  cache(101,-130,81,.08,0,'Search commons storage');
  const westStore=room({id:'riverside_store',x:-154,z:52,w:18,d:24,h:4.5,front:'east',label:'HARDWARE / SALVAGE EXCHANGE',style:'workshop'});
  cache(102,-154,48,.08,0,'Search hardware stockroom');
  const laundry=room({id:'canal_laundry',x:-126,z:-4,w:20,d:16,h:4.6,front:'south',label:'CANAL STREET / LAUNDERETTE',style:'store'});
  cache(103,-131,-7,.08,3,'Search abandoned security locker');
  const workshop=room({id:'east_relay',x:118,z:-24,w:24,d:22,h:5.8,front:'west',label:'EAST RELAY / TELECOM MAINTENANCE',style:'workshop'});
  cache(104,122,-29,.08,0,'Search relay maintenance supplies');
  ladder({id:'relay_roof',name:'Climb telecom service ladder',x:132,z:-24,top:workshop.roofY,exitX:126.8,exitZ:-24,axis:'x',drawX:130.7,drawZ:-24});
  questObject('relay',118,-28,workshop.roofY,'Recover the rooftop relay module',mats.purple);cache(111,113,-29,workshop.roofY,5,'Search rooftop arsenal');
  // A rooftop mast makes the relay quest's elevation evident from the street.
  cylinder(.1,.18,4.5,121,workshop.roofY+2.25,-28,mats.metal,root);for(const y of [1.8,3.1])box(3,.08,.12,121,workshop.roofY+y,-28,mats.metal,root);
  const archive=room({id:'east_archive',x:151,z:-39,w:18,d:22,h:8.6,front:'south',label:'CIVIC ARCHIVE / RECORDS OFFICE',style:'archive'});
  cache(105,156,-37,.08,0,'Search archive mailroom');
  floor('archive_mezzanine',151,-44,16.5,7.2,4.2,mortar);
  for(const xx of [144.6,157.4])blocker(xx,-44,.3,6.5,3.9,2.03,mats.metal);
  // A broken front railing leaves a generous gap for the internal ladder landing.
  for(const xx of [145.2,156.8])blocker(xx,-40.4,4.4,.1,.68,4.54,mats.metal);
  ladder({id:'archive_mezzanine',name:'Climb to archive mezzanine',x:151,z:-38.2,top:4.2,exitX:151,exitZ:-43,axis:'z',drawX:151,drawZ:-40});
  cache(110,147,-44.5,4.2,4,'Search archive marksman locker');
  ladder({id:'archive_roof',name:'Climb archive fire ladder',x:162,z:-39,top:archive.roofY,exitX:157,exitZ:-39,axis:'x',drawX:160.7,drawZ:-39});
  const pump=room({id:'cistern_pumphouse',x:110,z:132,w:24,d:16,h:4.6,front:'north',label:'CISTERN / MUNICIPAL PUMP HOUSE',style:'workshop'});
  cache(106,116,135,.08,0,'Search pump house reserves');
  ladder({id:'pumphouse_roof',name:'Climb pump house ladder',x:96,z:132,top:pump.roofY,exitX:101.2,exitZ:132,axis:'x',drawX:97.3,drawZ:132});
  const market=room({id:'cistern_market',x:139,z:105,w:20,d:24,h:4.5,front:'west',label:'CISTERN MARKET / TRADE HALL',style:'store'});
  cache(107,141,109,.08,0,'Search market storeroom');
  // The near-start replacement occupies the old solid apartment footprint.
  const pharmacy=room({id:'bunker_pharmacy',x:-31,z:58,w:22,d:23,h:4.8,front:'south',label:'RIVER PHARMACY / SEARCH INSIDE',style:'clinic'});
  cache(109,-36,54,.08,0,'Search pharmacy emergency stock');
  ladder({id:'pharmacy_roof',name:'Climb pharmacy fire escape',x:-18,z:57,top:pharmacy.roofY,exitX:-23.2,exitZ:57,axis:'x',drawX:-19.3,drawZ:57});
  // A low loading deck provides a short, distinct mantle route.
  blocker(104,71,5,3,1.32,.66,wood);floor('loading_deck',104,71,5,3,1.4,wood);cache(112,104,70.4,1.4,0,'Search freight deck crate');
  ladder({id:'freight_mantle',name:'Scramble onto loading deck',x:104,z:74,bottom:0,top:1.4,exitX:104,exitZ:71.7,requires:'mantle',axis:'z',drawX:104,drawZ:72.65});

  function settlement(settlement,npcX,npcZ,name){
    const {x,z,id}=settlement;box(22,.055,18,x,.04,z,mortar,root);const plazaLines=[];for(let u=-10;u<=10;u+=2)plazaLines.push([x+u,.072,z,.025,.008,18]);for(let v=-8;v<=8;v+=2)plazaLines.push([x,.072,z+v,22,.008,.025]);instances(plazaLines,dark);
    for(const u of [-10,10]){cylinder(.08,.08,4,x+u,2,z-7,mats.metal,root);box(.7,.15,.4,x+u,4,z-7,warmLight,root);}
    const welcome=addSign(settlement.name.toUpperCase()+' / SAFE COURTYARD',x,4.15,z-7,15,.72,'#c4e8c3');
    // Raised vegetable beds remain outside the courtyard's main walking aisles.
    for(const u of [-8,8]){blocker(x+u,z+7,3.8,1.8,.45,.225,wood);box(3.5,.025,1.55,x+u,.46,z+7,soil,root);for(let j=-1;j<=1;j++){const plant=new THREE.Mesh(new THREE.IcosahedronGeometry(.28,0),mats.leaf);plant.position.set(x+u+j,.7,z+7);plant.scale.y=.6;root.add(plant);}}
    for(const u of [-8,8]){box(2.7,.12,.6,x+u,.7,z-4,wood,root);for(const leg of [-1,1])box(.12,.65,.5,x+u+leg, .33,z-4,mats.metal,root);}
    const tankX=settlement.water.x,tankZ=settlement.water.z+2.6;
    blocker(tankX,tankZ,2.9,2.9,2.8,1.4,blue);cylinder(1.55,1.55,.18,tankX,2.9,tankZ,mats.metal,root,16);box(.18,1.3,.18,tankX,.7,tankZ-1.6,mats.metal,root);box(.65,.12,.15,tankX,.6,tankZ-1.7,mats.cyan,root);
    addSign('CLEAN WATER / ASK THE KEEPER',tankX,3.4,tankZ-1.6,5.4,.55,'#c3e7df');
    addInteractable('settlementWater',settlement.water.x,settlement.water.z,{id,name:settlement.name+' water reserve',y:0,water:2,goldCost:12});
    createAlly(settlement.npcId,npcX,npcZ,name,false);
  }
  settlement(settlements[0],-128,53,'Ada · Riverside keeper');
  settlement(settlements[1],107,102,'Malik · Cistern keeper');
  // Long-distance environmental silhouettes; nothing forms a second invisible wall.
  const bollards=[],rubble=[];
  for(const [x,z] of [[-91,15],[-91,25],[91,-33],[91,-23],[14,89],[26,89]]){bollards.push([x,.48,z,.3,.96,.3]);}
  for(let i=0;i<160;i++){const x=rnd(-169,169),z=rnd(-166,169);if(Math.abs(x)<88&&Math.abs(z)<88)continue;if(buildings.some(b=>Math.abs(x-b.x)<b.w/2+4&&Math.abs(z-b.z)<b.d/2+4)||settlements.some(s=>Math.hypot(x-s.x,z-s.z)<s.radius))continue;rubble.push([x,.05,z,rnd(.1,.45),rnd(.04,.12),rnd(.15,.65),0,rnd(0,6.28)]);}
  instances(bollards,mats.metal,true);instances(rubble,mats.concrete,true);
  // World functions are only needed during construction; returned metadata is data-only.
  for(const building of buildings)delete building.world;
  return {buildings,platforms,climbRoutes,settlements,spawns};
}
