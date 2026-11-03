#!/usr/bin/env node
// Hornea el PVS de uno o varios mapas en el PC con el build de escritorio y
// deja <mapa>.pvs en android/app/src/main/assets/packages/base/, listo para
// que la siguiente APK lo lleve dentro.
//   npm run pvs:bake -- duel7 [otro_mapa ...] [--cell 32]
// Ejecuta bin64/sauerbraten.exe con el directorio de assets como cwd y el
// comando de consola `pvsbake <mapa> <celda>` (src/engine/worldio.cpp), que
// carga el mapa, genera el PVS, guarda el fichero y sale.
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const args = process.argv.slice(2);
const ci = args.indexOf('--cell');
const cell = ci >= 0 ? Number(args[ci + 1]) : 32;
const maps = args.filter((a, i) => !a.startsWith('--') && (ci < 0 || i !== ci + 1));
if (!maps.length) { console.error('Uso: npm run pvs:bake -- <mapa> [...] [--cell 32]'); process.exit(1); }

const exe = path.join(repo, 'bin64', 'sauerbraten.exe');
if (!fs.existsSync(exe)) { console.error(`Falta ${exe}: npm run desktop:build`); process.exit(1); }
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
const home = path.join(repo, 'bin64', 'home');
fs.mkdirSync(home, { recursive: true });

for (const map of maps) {
  const out = path.join(assets, 'packages', 'base', `${map}.pvs`);
  console.log(`> ${map}: celdas de ${cell}`);
  const t0 = Date.now();
  // -q: home aparte para no ensuciar assets con config.cfg; -x: script inicial.
  // Los ficheros en modo escritura van al home (-q), no al cwd: el .pvs sale en home/packages/base.
  const r = spawnSync(exe, [`-q${home}`, '-glog.txt', '-w1280', '-h720', '-t0', `-xpvsbake ${map} ${cell}`], { cwd: assets, encoding: 'utf8', timeout: 30 * 60 * 1000 });
  if (r.error) { console.error(r.error.message); process.exit(1); }
  const baked = path.join(home, 'packages', 'base', `${map}.pvs`);
  const log = path.join(home, 'log.txt');
  if (fs.existsSync(baked)) {
    fs.copyFileSync(baked, out);
    const line = fs.existsSync(log) ? fs.readFileSync(log, 'utf8').split('\n').filter((l) => /view cells|pvs/.test(l)).pop() : '';
    console.log(`  OK ${out} (${(fs.statSync(out).size / 1024).toFixed(1)} kB, ${((Date.now() - t0) / 1000).toFixed(0)} s) ${line ?? ''}`);
  } else {
    console.error(`  no se generó ${baked}; últimas líneas de ${log}:`);
    if (fs.existsSync(log)) console.error(fs.readFileSync(log, 'utf8').split('\n').slice(-15).join('\n'));
    process.exit(1);
  }
}
