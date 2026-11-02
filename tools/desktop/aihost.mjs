#!/usr/bin/env node
// Anfitrión de IA: la build de escritorio sin pantalla conectada al servidor
// dedicado como espectador con privilegio de admin, de modo que el servidor le
// asigna TODOS los bots (serveraihost 1 en aiman.h) y el visor solo los ve.
//   npm run aihost:lan -- [--bots 16] [--skill 60] [--host 192.168.100.21] [--port 28785] [--adminpass sauvr]
// Cada segundo el log trae "aihost t= bots= owned= ai_ms=" (bots que piensa y coste por frame).
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const exe = path.join(repo, 'bin64', 'sauerbraten.exe');
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
if (!fs.existsSync(exe)) { console.error('falta bin64/sauerbraten.exe: npm run desktop:build'); process.exit(1); }
const args = process.argv.slice(2);
const opt = (name, def) => (args.includes(name) ? args[args.indexOf(name) + 1] : def);
const bots = +opt('--bots', 16);
const skill = +opt('--skill', 60);
const port = +opt('--port', 28785);
const host = opt('--host', (() => { for (const a of Object.values(os.networkInterfaces())) for (const i of a ?? []) if (i.family === 'IPv4' && !i.internal) return i.address; return 'localhost'; })());
let adminpass = opt('--adminpass', null);
if (!adminpass) { // la misma que escribió server.mjs
  try { adminpass = /adminpass "([^"]*)"/.exec(fs.readFileSync(path.join(repo, 'bin64', 'server-home', 'server-init.cfg'), 'utf8'))?.[1]; } catch {}
  adminpass ??= 'sauvr';
}
const home = path.join(repo, 'dist', 'aihost');
fs.mkdirSync(home, { recursive: true });
fs.rmSync(path.join(home, 'log.txt'), { force: true });
// conectar → admin (setmaster) → espectador → añadir bots (addbot de un admin no tiene límite)
const script = `name aihost; aihost 1; connect ${host} ${port}; sleep 3000 [setmaster ${adminpass}; sleep 500 [spectator 1; sleep 1000 [loop i ${bots} [addbot ${skill}]]]]`;
const child = spawn(exe, ['-H', `-q${home}`, '-glog.txt', '-w320', '-h240', '-t0', '-x' + script], { cwd: assets, stdio: 'ignore' });
console.log(`anfitrión de IA → ${host}:${port}, ${bots} bots (skill ${skill}); log en ${path.join(home, 'log.txt')}`);
child.on('exit', (code) => { console.log(`aihost terminado (${code})`); process.exit(code ?? 0); });
setInterval(() => {
  let txt = ''; try { txt = fs.readFileSync(path.join(home, 'log.txt'), 'utf8'); } catch { return; }
  const ls = txt.split(/\r?\n/).filter((l) => l.startsWith('aihost ') && l.includes('rtt='));
  if (ls.length) console.log(new Date().toLocaleTimeString() + '  ' + ls[ls.length - 1]);
}, 5000);
process.on('SIGINT', () => { try { child.kill(); } catch {} process.exit(0); });
