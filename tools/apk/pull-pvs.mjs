#!/usr/bin/env node
// Saca del Quest (por USB, adb) los ficheros <mapa>.pvs que el juego generó en
// segundo plano y los copia a android/app/src/main/assets/packages/base/, para
// que la siguiente APK los lleve precalculados y no tenga que generarlos.
//   npm run apk:pull-pvs            # todos los .pvs que haya en el visor
//   npm run apk:pull-pvs -- duel7   # solo ese mapa
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const adb = process.env.ADB ?? 'C:/Android/Sdk/platform-tools/adb.exe';
const pkg = 'org.sauvrengine.vr';
const dest = path.join(repo, 'android', 'app', 'src', 'main', 'assets', 'packages', 'base');
const only = process.argv[2];

function sh(args, opts = {}) {
  const r = spawnSync(adb, args, { encoding: 'buffer', maxBuffer: 1 << 28, ...opts });
  if (r.status !== 0) throw new Error(`adb ${args.join(' ')} falló: ${r.stderr?.toString() ?? ''}`);
  return r.stdout;
}

const devices = sh(['devices']).toString().split('\n').filter((l) => /\tdevice$/.test(l.trim()));
if (!devices.length) { console.error('No hay ningún Quest conectado por USB (adb devices). Acepta el aviso de depuración en el visor.'); process.exit(1); }

// La app es debuggable: run-as permite leer su almacenamiento interno.
const list = sh(['shell', `run-as ${pkg} sh -c 'cd files/packages/base 2>/dev/null && ls *.pvs 2>/dev/null'`]).toString().trim().split(/\s+/).filter(Boolean);
const wanted = only ? list.filter((f) => f === `${only}.pvs`) : list;
if (!wanted.length) { console.error(`El visor no tiene ningún .pvs${only ? ` para ${only}` : ''}. Entra al mapa y espera a que el panel diga "pvs celdas N".`); process.exit(1); }

fs.mkdirSync(dest, { recursive: true });
for (const f of wanted) {
  const data = sh(['exec-out', `run-as ${pkg} cat files/packages/base/${f}`]);
  const out = path.join(dest, f);
  fs.writeFileSync(out, data);
  console.log(`${f}: ${(data.length / 1024).toFixed(1)} kB → ${out}`);
}
console.log('Listo: npm run apk:build para una APK con el PVS precalculado.');
