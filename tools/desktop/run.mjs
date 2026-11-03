#!/usr/bin/env node
// Arranca la build de escritorio (bin64/sauerbraten.exe) con los assets de
// Android como cwd y un home aparte, para comparar iluminación con el visor.
//   npm run desktop:run [-- duel7]        (mapa por defecto: duel7)
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const exe = path.join(repo, 'bin64', 'sauerbraten.exe');
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
const home = path.join(repo, 'bin64', 'home');
const map = process.argv[2] || 'duel7';
if (!fs.existsSync(exe)) { console.error('falta bin64/sauerbraten.exe: npm run desktop:build'); process.exit(1); }
fs.mkdirSync(home, { recursive: true });
const child = spawn(exe, [`-q${home}`, '-glog.txt', '-w1600', '-h900', '-t0', `-xmap ${map}`], { cwd: assets, detached: true, stdio: 'ignore' });
child.unref();
console.log(`sauerbraten.exe lanzado (pid ${child.pid}) con map ${map}; cwd ${assets}; registro en ${home}/log.txt`);
