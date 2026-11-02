#!/usr/bin/env node
// Importa un mapa de Sauerbraten (y todo lo que referencia) desde la APK original
// de Sauerbraten-VR (dist/apk/sauerbraten-vr.apk, 1 GB con packages/ completo) a
// android/app/src/main/assets/packages/, copiando solo lo que falta.
//   node tools/desktop/import-map.mjs <mapa> [--apk ruta] [--dry]
// Analiza data/default_map_settings.cfg, data/default_map_models.cfg y
// packages/base/<mapa>.cfg (y los exec anidados): texturas, modelos (carpeta
// entera), skybox/nubes (6 caras), sonidos. Lo que no esté en la APK se lista.
// El skybox va guardado como variable dentro del .ogz: pásalo con --sky dir/nombre.
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const args = process.argv.slice(2);
const opt = (name) => (args.includes(name) ? args[args.indexOf(name) + 1] : null);
const map = args.find((a, i) => !a.startsWith('--') && (i === 0 || !args[i - 1].startsWith('--')));
if (!map) { console.error('Uso: import-map.mjs <mapa> [--apk ruta] [--sky dir/nombre] [--dry]'); process.exit(1); }
const apk = opt('--apk') ?? path.join(repo, 'dist', 'apk', 'sauerbraten-vr.apk');
const sky = opt('--sky');
// --deps fichero: salida de "mapdeps" en la build de escritorio (dep slot/model/sky).
// Con él solo se importan las texturas de los slots que el mapa usa de verdad, los
// modelos colocados y el skybox; los cfg se siguen leyendo por los exec, materiales y sonidos.
const depsFile = opt('--deps');
const dry = args.includes('--dry');
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
const tmp = path.join(repo, 'dist', 'import-tmp');

// índice de la APK (rutas assets/...)
const list = spawnSync('unzip', ['-Z1', apk], { encoding: 'utf8', maxBuffer: 64 << 20 });
if (list.status !== 0) { console.error('no pude listar la APK:', list.stderr); process.exit(1); }
const inApk = new Set(list.stdout.split(/\r?\n/).filter(Boolean));
const lower = new Map([...inApk].map((f) => [f.toLowerCase(), f]));
const findApk = (rel) => (inApk.has('assets/' + rel) ? 'assets/' + rel : lower.get(('assets/' + rel).toLowerCase()));

const wanted = new Set();  // rutas relativas a assets/ que hay que sacar de la APK
const missing = new Set(); // referenciadas pero no están ni en assets ni en la APK
const parsed = new Set();

function want(rel) {
  rel = rel.replace(/\\/g, '/').replace(/^\/+/, '');
  if (fs.existsSync(path.join(assets, rel))) return true;
  const hit = findApk(rel);
  if (hit) { wanted.add(hit.slice('assets/'.length)); return true; }
  return false;
}
function wantFile(rel) { // con extensión, o imagen sin ella
  if (/\.(jpg|png|jpeg|tga|dds|ogg|wav|cfg)$/i.test(rel)) { if (!want(rel)) missing.add(rel); return; }
  for (const ext of ['.jpg', '.png', '.jpeg']) if (want(rel + ext)) return;
  missing.add(rel + '.*');
}
function wantDir(rel) { // carpeta entera (modelos)
  const prefix = ('assets/' + rel.replace(/\/+$/, '') + '/').toLowerCase();
  let n = 0;
  for (const f of inApk) {
    if (!f.toLowerCase().startsWith(prefix) || f.endsWith('/')) continue;
    n++;
    if (!fs.existsSync(path.join(assets, f.slice(7)))) wanted.add(f.slice(7));
  }
  if (!n && !fs.existsSync(path.join(assets, rel))) missing.add(rel + '/');
}
const stripCmds = (s) => s.replace(/^(<[^>]*>)+/, '');
const pkg = (s) => { s = stripCmds(s).replace(/^"|"$/g, ''); return s.startsWith('packages/') ? s : 'packages/' + s; };
const skyFaces = (base) => { for (const s of ['_ft', '_bk', '_lf', '_rt', '_up', '_dn']) wantFile(base + s); };

function readCfg(rel) {
  const local = path.join(assets, rel);
  if (fs.existsSync(local)) return fs.readFileSync(local, 'utf8');
  const hit = findApk(rel);
  if (!hit) return null;
  fs.mkdirSync(tmp, { recursive: true });
  spawnSync('unzip', ['-o', '-q', apk, hit, '-d', tmp], { encoding: 'utf8' });
  wanted.add(hit.slice('assets/'.length));
  return fs.readFileSync(path.join(tmp, hit), 'utf8');
}
function parseCfg(rel) {
  if (parsed.has(rel)) return;
  parsed.add(rel);
  const src = readCfg(rel);
  if (src == null) { missing.add(rel); return; }
  for (let line of src.split(/\r?\n/)) {
    line = line.replace(/\/\/.*$/, '').trim();
    if (!line) continue;
    const toks = line.match(/"[^"]*"|\[[^\]]*\]|\S+/g) ?? [];
    const cmd = (toks[0] ?? '').toLowerCase();
    const arg = (i) => (toks[i] ?? '').replace(/^"|"$/g, '').replace(/^\[|\]$/g, '');
    if (cmd === 'exec') parseCfg(arg(1).replace(/^\/+/, ''));
    else if (cmd === 'texture' && toks.length >= 3) { if (!depsFile) wantFile(pkg(arg(2))); }
    else if (cmd === 'autograss' || cmd === 'cloudlayer' || cmd === 'altcloudlayer') wantFile(pkg(arg(1)));
    else if (cmd === 'mapmodel' || cmd === 'mmodel') { if (!depsFile) { const n = cmd === 'mapmodel' && toks.length >= 5 ? arg(4) : arg(1); if (n) wantDir('packages/models/' + n); } }
    else if (cmd === 'loadsky' || cmd === 'skybox' || cmd === 'cloudbox' || cmd === 'altcloudbox') { if (!depsFile) { const b = pkg(arg(1)); if (b !== 'packages/') skyFaces(b); } }
    else if (cmd === 'mapsound') wantFile('packages/sounds/' + arg(1));
    else if (cmd === 'matwater' || cmd === 'matlava' || cmd === 'matglass') for (let i = 2; i < toks.length; i++) { const a = arg(i).split(/\s+/)[0]; if (/\.(jpg|png)$/i.test(a)) wantFile(pkg(a)); }
  }
}

// el mapa en sí
if (!findApk('packages/base/' + map + '.ogz') && !fs.existsSync(path.join(assets, 'packages/base/' + map + '.ogz'))) {
  console.error('la APK no trae packages/base/' + map + '.ogz'); process.exit(1);
}
for (const ext of ['.ogz', '.cfg', '.jpg', '.png']) want('packages/base/' + map + ext);
parseCfg('data/default_map_settings.cfg');
parseCfg('data/default_map_models.cfg');
parseCfg('packages/base/' + map + '.cfg');
if (sky) skyFaces(pkg(sky));
if (depsFile) {
  for (const line of fs.readFileSync(depsFile, 'utf8').split(/\r?\n/)) {
    const t = line.trim().split(/\s+/);
    if (t[0] !== 'dep') continue;
    if (t[1] === 'slot') for (const ref of t.slice(3)) { const name = ref.replace(/^[a-z]:/, ''); if (name) wantFile(pkg(name)); }
    else if (t[1] === 'model' && t[2]) wantDir('packages/models/' + t[2]);
    else if (t[1] === 'sky' && t[2]) skyFaces(pkg(t[2]));
  }
}

const files = [...wanted].sort();
console.log(`${map}: ${files.length} ficheros a importar de la APK, ${missing.size} referencias sin fuente`);
if (missing.size) console.log('  sin fuente: ' + [...missing].slice(0, 30).join(', ') + (missing.size > 30 ? ' …' : ''));
if (dry) { for (const f of files) console.log('  ' + f); fs.rmSync(tmp, { recursive: true, force: true }); process.exit(0); }
let bytes = 0;
if (files.length) {
  fs.mkdirSync(tmp, { recursive: true });
  for (let i = 0; i < files.length; i += 150) { // lotes: la línea de comandos de Windows es corta
    const batch = files.slice(i, i + 150).map((f) => 'assets/' + f);
    const r = spawnSync('unzip', ['-o', '-q', apk, ...batch, '-d', tmp], { encoding: 'utf8' });
    if (r.status !== 0 && r.status !== 11) console.error('unzip:', r.stderr || r.status);
  }
  for (const f of files) {
    const src = path.join(tmp, 'assets', f), dst = path.join(assets, f);
    if (!fs.existsSync(src)) { console.error('  no salió de la APK: ' + f); continue; }
    fs.mkdirSync(path.dirname(dst), { recursive: true });
    fs.copyFileSync(src, dst);
    bytes += fs.statSync(dst).size;
  }
  fs.rmSync(tmp, { recursive: true, force: true });
}
console.log(`copiados ${files.length} ficheros (${(bytes / 1048576).toFixed(1)} MB) a assets/`);
const byDir = {};
for (const f of files) { const d = f.split('/').slice(0, 2).join('/'); byDir[d] = (byDir[d] || 0) + 1; }
console.log('  ' + Object.entries(byDir).map(([d, n]) => `${d}: ${n}`).join('  '));
