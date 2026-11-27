// Local authoring companion. The game installs only the JSON the player approves.
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawn, spawnSync } = require('node:child_process');
const { createInterface } = require('node:readline/promises');

function locate(name) {
  const search = spawnSync('where.exe', [name], { encoding: 'utf8', windowsHide: true });
  return (search.stdout || '').trim().split(/\r?\n/).filter(Boolean);
}
function findCodex() {
  const [native] = locate('codex.exe');
  if (native) return [native];
  for (const shim of locate('codex.cmd')) {
    const cli = path.join(path.dirname(shim), 'node_modules', '@openai', 'codex', 'bin', 'codex.js');
    if (fs.existsSync(cli)) return [process.execPath, cli];
  }
  throw Error('Install the official Codex CLI or use Open ChatGPT in browser in the game.');
}
function run(command, args, cwd, input) {
  return new Promise((resolve, reject) => {
    const child = spawn(command[0], [...command.slice(1), ...args], {
      cwd, stdio: [input === undefined ? 'inherit' : 'pipe', 'inherit', 'inherit'], shell: false
    });
    child.on('error', reject);
    child.on('exit', code => code === 0 ? resolve() : reject(Error('Codex did not finish. The workshop also supports browser authoring.')));
    if (input !== undefined) {
      child.stdin.on('error', error => { if (error.code !== 'EPIPE') reject(error); });
      child.stdin.end(input, 'utf8');
    }
  });
}
async function main() {
  console.log('EARTHtotaled mod workshop');
  const command = findCodex();
  const cwd = fs.mkdtempSync(path.join(os.tmpdir(), 'earthtotaled-mod-'));
  const status = spawnSync(command[0], [...command.slice(1), 'login', 'status'], { encoding: 'utf8', cwd });
  if (status.status !== 0) {
    console.log('Choose Sign in with ChatGPT in the official browser window.');
    await run(command, ['login'], cwd);
  } else if (!/chatgpt/i.test((status.stdout || '') + (status.stderr || ''))) {
    throw Error('Codex currently uses a different sign-in method. Choose Open ChatGPT in browser to use your personal ChatGPT account, or switch your account in Codex.');
  }
  const terminal = createInterface({ input: process.stdin, output: process.stdout });
  let idea;
  try { idea = (await terminal.question('What should players find, and where? ')).trim(); }
  finally { terminal.close(); }
  if (!idea) return;
  const result = path.join(cwd, 'proposal.json');
  const brief = fs.readFileSync(path.join(__dirname, 'AI-BRIEF.txt'), 'utf8');
  await run(command, ['exec', '--sandbox', 'read-only', '--skip-git-repo-check', '--ephemeral',
    '--output-schema', path.join(__dirname, 'mod.schema.json'), '-o', result, '-'], cwd,
    brief + '\nReturn only JSON. Do not use tools or access files. My idea follows.\n' + idea + '\n');
  const proposal = fs.readFileSync(result, 'utf8').trim();
  JSON.parse(proposal);
  console.log('\n' + proposal + '\n\nSaved in ' + result);
  const clip = spawnSync('clip.exe', [], { input: Buffer.from('\ufeff' + proposal, 'utf16le'), windowsHide: true });
  console.log(clip.status === 0 ? 'Copied. Choose Preview clipboard in the workshop.' : 'Open the saved file, copy its JSON, and choose Preview clipboard in the workshop.');
}
main().catch(error => { console.error(error.message); process.exitCode = 1; });
