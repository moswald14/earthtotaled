// Copy the actual game's design data without booting its DOM or changing saves.
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import {fileURLToPath,pathToFileURL} from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const legacy = path.resolve(root, '../earthtotaled/dist');
const game = fs.readFileSync(path.join(legacy, 'game.js'), 'utf8').replace(/\r\n/g,'\n');
const read = (name, end) => game.slice(game.indexOf(`const ${name}=`)+`const ${name}=`.length, game.indexOf(end, game.indexOf(`const ${name}=`)));
const {EXTRA_WEAPONS} = await import(pathToFileURL(path.join(legacy, 'arsenal.js')));
const {SKILL_NODES} = await import(pathToFileURL(path.join(legacy, 'skill-web.js')));
const weapons = vm.runInNewContext(read('weapons', ';\nconst chapters').trim(), {EXTRA_WEAPONS});
const chapters = vm.runInNewContext(read('chapters', ';\nconst story').trim());
const story = vm.runInNewContext(read('story', ';\nfunction initialState').trim());
const quests = vm.runInNewContext('('+read('quests', ';\nfunction createSurvivalWorld').trim()+')');
const data = {schema:1, source:'EARTHtotaled browser 4.1.0', weapons, chapters, story, skills:SKILL_NODES, quests:Object.values(quests)};
// Current Unreal canon uses candybar wrappers; retain IDs and the legacy numeric reward key.
data.skills = data.skills.map(p => ({...p, name:p.name.replace('Gold Trace','Wrapper Trace'), description:p.description.replace(/\bgold\b/g,'candybar wrappers')}));
data.quests = data.quests.map(q => ({...q, reward:q.reward.replace(/\bgold\b/g,'wrappers')}));
fs.mkdirSync(path.join(root, 'Content/Data'), {recursive:true});
fs.writeFileSync(path.join(root, 'Content/Data/design.json'), JSON.stringify(data, null, 2)+'\n');
console.log(`Preserved ${weapons.length} weapons, ${SKILL_NODES.length} perks, ${story.length} recordings, ${data.quests.length} quests.`);
