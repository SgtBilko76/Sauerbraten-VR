#!/usr/bin/env node
// Arranca el servidor dedicado (bin64/sauer_server.exe) en la red local para que
// el visor se conecte por IP o por el botón "lan connect" del menú principal
// (el servidor contesta al descubrimiento por broadcast en el puerto 28786).
//   npm run server:lan [-- --name "SauVREngine LAN" --clients 16 --port 28785]
// Pasos del diseño: docs/servidor-dedicado.md (paso 1).
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const exe = path.join(repo, 'bin64', 'sauer_server.exe');
if (!fs.existsSync(exe)) { console.error('falta bin64/sauer_server.exe: npm run server:build'); process.exit(1); }
const args = process.argv.slice(2);
const opt = (name, def) => (args.includes(name) ? args[args.indexOf(name) + 1] : (process.env['npm_config_' + name.replace(/^--/, '')] ?? def)); // npm se come los --flags: también npm_config_*
const name = opt('--name', 'SauVREngine LAN');
const clients = opt('--clients', '16');
const port = opt('--port', '28785');
const home = path.join(repo, 'bin64', 'server-home');
fs.mkdirSync(home, { recursive: true });
// server-init.cfg: sin rotación de mapas el servidor arranca con mapa vacío (rotatemap →
// changemap("")) y el visor entra "a la nada". Se regenera en cada arranque.
const maps = opt('--maps', 'duel7 ruebli');
const adminpass = opt('--adminpass', 'sauvr'); // la usa aihost.mjs (setmaster) para ser dueño de los bots
fs.writeFileSync(path.join(home, 'server-init.cfg'), [
  '// generado por tools/desktop/server.mjs',
  `serverdesc "${name}"`,
  'maprotationreset',
  `maprotation "ffa" "${maps}"`,
  `maprotation "ctf" "${maps.split(/\s+/).filter((m) => m !== 'duel7').join(' ') || maps}"`,
  'serverbotlimit 32',
  `adminpass "${adminpass}"`,
  'serveraihost 1',
  '',
].join('\n'));

const ips = [];
for (const addrs of Object.values(os.networkInterfaces())) for (const a of addrs ?? []) if (a.family === 'IPv4' && !a.internal) ips.push(a.address);
console.log(`Servidor dedicado "${name}": puerto UDP ${port} (info ${+port + 1}), hasta ${clients} clientes`);
console.log(`  en el visor: menú principal → "lan connect", o /connect ${ips[0] ?? '<IP>'} ${port}`);
console.log(`  registro: ${path.join(home, 'server-log.txt')}; rotación ffa: ${maps}`);
// -q: home propio (server-init.cfg, log); sin -m: no se registra en el master público.
const child = spawn(exe, [`-q${home}`, '-gserver-log.txt', `-n${name}`, `-c${clients}`, `-j${port}`, '-m'], { cwd: repo, stdio: 'inherit' });
child.on('exit', (code) => { console.log(`servidor terminado (${code})`); process.exit(code ?? 0); });
