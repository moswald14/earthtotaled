// An original interconnected progression map. A node needs any owned neighbor
// named in `requires`; the caller owns XP, points, saves, and atomic purchases.
export const SKILL_NODES = [
  {id:'survivor',name:'The Survivor',branch:'Origin',x:340,y:170,cost:0,description:'You made it out of the bunker. Everything begins here.',requires:[]},
  {id:'climb',name:'Climbing Grip',branch:'Mobility',x:245,y:125,cost:1,description:'Unlock ordinary rooftop access. Approach a marked ladder and press E to climb automatically.',requires:['survivor']},
  {id:'mantle',name:'Over the Edge',branch:'Mobility',x:155,y:80,cost:1,description:'Unlock advanced exterior climbing routes marked “Needs Over the Edge.” Press E at their marked access points to climb onto the roof.',requires:['climb']},
  {id:'climber2',name:'Iron Fingers',branch:'Mobility',x:65,y:60,cost:2,description:'Climb 40% faster. A quicker ascent leaves less time exposed to the machines below.',requires:['mantle']},
  {id:'parkour',name:'Light Footed',branch:'Mobility',x:125,y:190,cost:2,description:'Move 12% faster and take 25% less fall damage.',requires:['climb','vitality']},
  {id:'marksman',name:'Clean Shot',branch:'Combat',x:435,y:125,cost:1,description:'All weapons deal 15% more damage.',requires:['survivor']},
  {id:'reload',name:'Practiced Hands',branch:'Combat',x:525,y:80,cost:1,description:'Reload every weapon 20% faster.',requires:['marksman','pressure']},
  {id:'steady',name:'Dead Calm',branch:'Combat',x:615,y:60,cost:2,description:'All weapons deal another 15% of base damage. Make each cartridge count.',requires:['reload']},
  {id:'pressure',name:'Keep Moving',branch:'Combat',x:555,y:190,cost:2,description:'Move 6% faster and reload 10% faster.',requires:['reload','scavenger']},
  {id:'vitality',name:'Second Wind',branch:'Survival',x:245,y:245,cost:1,description:'Increase maximum health by 25.',requires:['survivor','parkour','endurance']},
  {id:'hydration',name:'Water Wise',branch:'Survival',x:155,y:300,cost:1,description:'Hydration drains 20% more slowly. Water is still essential to stay alive.',requires:['vitality','hardy']},
  {id:'medic',name:'Field Medicine',branch:'Survival',x:65,y:340,cost:2,description:'Medkits restore 25 additional health.',requires:['hydration']},
  {id:'hardy',name:'Weathered',branch:'Survival',x:235,y:350,cost:2,description:'Increase maximum health by another 25 and take 20% less fall damage.',requires:['hydration','endurance']},
  {id:'scavenger',name:'Sharp Eyes',branch:'Salvage',x:435,y:245,cost:1,description:'Gain 20% more scrap and gold from scavenging. This never adds water to robot drops.',requires:['survivor','pressure','endurance']},
  {id:'prospector',name:'Gold Trace',branch:'Salvage',x:525,y:300,cost:1,description:'Gain another 20% of base scrap and gold from scavenging.',requires:['scavenger','quartermaster']},
  {id:'salvager',name:'Make It Last',branch:'Salvage',x:615,y:340,cost:2,description:'Gain another 15% of base scrap and gold, and restore 10 more health per medkit.',requires:['prospector']},
  {id:'quartermaster',name:'Well Prepared',branch:'Salvage',x:445,y:350,cost:2,description:'Reload 10% faster and gain another 10% of base scrap and gold.',requires:['prospector','endurance']},
  {id:'endurance',name:'Long Road',branch:'Survival',x:340,y:305,cost:2,description:'Move 5% faster and reduce hydration drain by a further 10%. Connects the survival and salvage paths.',requires:['vitality','scavenger','hardy','quartermaster']}
];

const NODE_BY_ID = new Map(SKILL_NODES.map(node=>[node.id,node]));
const BRANCH_COLORS = {Origin:'#e5ead1',Mobility:'#94d9e6',Combat:'#efa47c',Survival:'#b8d887',Salvage:'#d7b5ed'};
const knownSet = values => new Set(Array.isArray(values)?values.filter(id=>NODE_BY_ID.has(id)):[]);

export function canUnlockSkill(id,unlocked,points) {
  const node=NODE_BY_ID.get(id),owned=knownSet(unlocked);
  return !!node && id!=='survivor' && !owned.has(id) && Number.isFinite(points)
    && points>=node.cost && node.requires.some(neighbor=>owned.has(neighbor));
}

export function getPerkBonuses(unlocked) {
  const owned=knownSet(unlocked);
  const bonus={maxHealth:100,damage:1,waterRate:1,reloadRate:1,moveRate:1,
    climbRate:1,lootRate:1,healBonus:0,fallRate:1,canClimb:false,canMantle:false};
  if(owned.has('climb'))bonus.canClimb=true;
  if(owned.has('mantle'))bonus.canMantle=true;
  if(owned.has('climber2'))bonus.climbRate*=1.4;
  if(owned.has('parkour')){bonus.moveRate*=1.12;bonus.fallRate*=.75;}
  if(owned.has('marksman'))bonus.damage+=.15;
  if(owned.has('reload'))bonus.reloadRate*=.8;
  if(owned.has('steady'))bonus.damage+=.15;
  if(owned.has('pressure')){bonus.moveRate*=1.06;bonus.reloadRate*=.9;}
  if(owned.has('vitality'))bonus.maxHealth+=25;
  if(owned.has('hydration'))bonus.waterRate*=.8;
  if(owned.has('medic'))bonus.healBonus+=25;
  if(owned.has('hardy')){bonus.maxHealth+=25;bonus.fallRate*=.8;}
  if(owned.has('scavenger'))bonus.lootRate+=.2;
  if(owned.has('prospector'))bonus.lootRate+=.2;
  if(owned.has('salvager')){bonus.lootRate+=.15;bonus.healBonus+=10;}
  if(owned.has('quartermaster')){bonus.reloadRate*=.9;bonus.lootRate+=.1;}
  if(owned.has('endurance')){bonus.moveRate*=1.05;bonus.waterRate*=.9;}
  return bonus;
}

/** Render into the existing overlay panel. onUnlock(id) must return false on
 * failure, or a fresh {unlocked,points,xp,nextXP} snapshot after a purchase.
 * The callback may be asynchronous. The returned update(snapshot) retains the
 * current selected node. All controls are removed when the panel is replaced.
 */
export function renderSkillWeb({container,unlocked=['survivor'],points=0,xp=0,nextXP=100,onUnlock,onClose}) {
  if(!container)throw new Error('A skill-web container is required.');
  let data={unlocked:[...unlocked],points,xp,nextXP};
  let selected='climb',busy=false,destroyed=false,message='';
  const edges=[];const seen=new Set();
  for(const node of SKILL_NODES)for(const neighbor of node.requires){
    if(!NODE_BY_ID.has(neighbor))continue;
    const key=[node.id,neighbor].sort().join(':');
    if(!seen.has(key)){seen.add(key);edges.push([NODE_BY_ID.get(neighbor),node]);}
  }
  const stateOf=(node,owned)=>owned.has(node.id)?'owned':node.requires.some(id=>owned.has(id))?'available':'locked';
  const esc=value=>String(value).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  function paint(){
    if(destroyed)return;
    const owned=knownSet(data.unlocked),node=NODE_BY_ID.get(selected)||SKILL_NODES[0];
    const status=stateOf(node,owned),affordable=canUnlockSkill(node.id,data.unlocked,data.points);
    const progress=data.nextXP>0?Math.max(0,Math.min(100,data.xp/data.nextXP*100)):0;
    const nodeStatus=status==='owned'?'ACQUIRED':status==='available'?'CONNECTED / AVAILABLE':'UNCONNECTED';
    container.innerHTML=`<section class="skill-web" aria-label="Lifeline skill web">
      <header class="skill-header"><div><div class="eyebrow">MARA VENN / PROGRESSION</div><h2>Lifeline.</h2><p>Choose the survivor you become.</p></div><div class="skill-currency"><b>${Math.max(0,Math.floor(Number(data.points)||0))}</b><span>SKILL POINTS</span></div></header>
      <div class="skill-xp"><span>FIELD EXPERIENCE</span><span>${Math.floor(Number(data.xp)||0)} / ${Math.floor(Number(data.nextXP)||0)} XP</span><i><b style="width:${progress}%"></b></i></div>
      <div class="skill-map-scroll" aria-label="Skill branches; scroll sideways on small screens">
        <svg class="skill-map" viewBox="0 0 680 390" aria-label="Connected skill constellation">
          <defs><radialGradient id="skillGlow"><stop stop-color="#b7d78a" stop-opacity=".12"/><stop offset="1" stop-color="#b7d78a" stop-opacity="0"/></radialGradient></defs>
          <ellipse cx="340" cy="185" rx="285" ry="160" fill="url(#skillGlow)"/>
          <ellipse cx="340" cy="185" rx="284" ry="159" class="skill-orbit"/>
          <ellipse cx="340" cy="185" rx="174" ry="107" class="skill-orbit"/>
          <text x="180" y="30" class="skill-branch" fill="${BRANCH_COLORS.Mobility}">MOBILITY</text><text x="470" y="30" class="skill-branch" fill="${BRANCH_COLORS.Combat}">COMBAT</text>
          <text x="140" y="380" class="skill-branch" fill="${BRANCH_COLORS.Survival}">SURVIVAL</text><text x="510" y="380" class="skill-branch" fill="${BRANCH_COLORS.Salvage}">SALVAGE</text>
          ${edges.map(([a,b])=>{const both=owned.has(a.id)&&owned.has(b.id),one=owned.has(a.id)||owned.has(b.id);return `<line x1="${a.x}" y1="${a.y}" x2="${b.x}" y2="${b.y}" class="skill-edge ${both?'skill-edge--owned':one?'skill-edge--open':''}"/>`;}).join('')}
          ${SKILL_NODES.map(n=>{const s=stateOf(n,owned),isSelected=n.id===selected;return `<g class="skill-node skill-node--${s} ${isSelected?'skill-node--selected':''}" data-skill="${n.id}" role="button" tabindex="0" aria-label="${esc(n.name)}. ${s==='owned'?'Acquired':s==='available'?'Connected':'Locked'}. ${n.cost} skill point${n.cost===1?'':'s'}." aria-pressed="${isSelected}" style="--branch:${BRANCH_COLORS[n.branch]}" transform="translate(${n.x} ${n.y})"><circle class="skill-selection" r="24"/><circle class="skill-node-ring" r="${n.id==='survivor'?21:17}"/><text class="skill-node-symbol" text-anchor="middle" y="5">${s==='owned'?'✓':n.id==='climb'?'↟':n.id==='survivor'?'◈':n.cost}</text><text class="skill-node-label" text-anchor="middle" y="${n.id==='survivor'?37:33}">${esc(n.name)}</text></g>`;}).join('')}
        </svg>
      </div>
      <div class="skill-legend"><span class="skill-legend-owned">● ACQUIRED</span><span class="skill-legend-open">○ CONNECTED</span><span class="skill-legend-locked">○ LOCKED</span><span>SELECT A NODE TO INSPECT</span></div>
      <article class="skill-detail" aria-live="polite"><div><small style="color:${BRANCH_COLORS[node.branch]}">${esc(node.branch.toUpperCase())} / ${nodeStatus}</small><h3>${esc(node.name)}</h3><p>${esc(node.description)}</p><p class="skill-requirement">${status==='owned'?'This ability is part of your survivor.':`Connect through any one: ${node.requires.map(id=>esc(NODE_BY_ID.get(id)?.name||id)+(owned.has(id)?' ✓':'')).join(' · ')}.`}</p></div><button class="primary skill-unlock" type="button" ${!affordable||busy?'disabled':''}>${busy?'UNLOCKING…':status==='owned'?'ACQUIRED':status==='locked'?'CONNECT FIRST':data.points<node.cost?'NEED '+node.cost+' POINTS':'UNLOCK · '+node.cost+' POINT'+(node.cost===1?'':'S')}</button></article>
      <div class="skill-message" role="status">${esc(message)}</div>
      <footer class="skill-footer"><span>Defeat machines, scavenge supplies, and finish quests to earn XP.</span><button type="button" class="secondary skill-close">BACK TO THE RUINS <span>→</span></button></footer>
    </section>`;
    const groups=container.querySelectorAll('[data-skill]');
    groups.forEach(el=>{
      const select=()=>{selected=el.getAttribute('data-skill');message='';paint();container.querySelector(`[data-skill="${selected}"]`)?.focus();};
      el.addEventListener('click',select);
      el.addEventListener('keydown',event=>{
        if(event.key==='Enter'||event.key===' '){event.preventDefault();event.stopPropagation();select();}
        if(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(event.key)){
          event.preventDefault();event.stopPropagation();
          const current=NODE_BY_ID.get(el.getAttribute('data-skill'));
          const candidates=SKILL_NODES.filter(n=>n!==current&&({ArrowLeft:n.x<current.x,ArrowRight:n.x>current.x,ArrowUp:n.y<current.y,ArrowDown:n.y>current.y}[event.key]));
          candidates.sort((a,b)=>Math.hypot(a.x-current.x,a.y-current.y)-Math.hypot(b.x-current.x,b.y-current.y));
          if(candidates[0]){selected=candidates[0].id;message='';paint();container.querySelector(`[data-skill="${selected}"]`)?.focus();}
        }
      });
    });
    container.querySelector('.skill-close').addEventListener('click',()=>{if(!busy&&typeof onClose==='function')onClose();});
    container.querySelector('.skill-unlock').addEventListener('click',async()=>{
      if(busy||!canUnlockSkill(selected,data.unlocked,data.points)||typeof onUnlock!=='function')return;
      const id=selected;busy=true;message='';paint();
      try{
        const result=await onUnlock(id);
        if(destroyed)return;
        if(result&&typeof result==='object'&&Array.isArray(result.unlocked)){
          data={...data,...result,unlocked:[...result.unlocked]};
          message=knownSet(data.unlocked).has(id)?NODE_BY_ID.get(id).name+' acquired.':'The purchase was not applied.';
        }else message='No points spent. Select a connected skill and try again.';
      }catch{message='The skill could not be unlocked. Your game controls its saved points.';}
      finally{busy=false;paint();}
    });
  }
  paint();
  return {update(snapshot){if(destroyed)return;data={...data,...snapshot,unlocked:[...(snapshot.unlocked||data.unlocked)]};paint();},destroy(){destroyed=true;}};
}
