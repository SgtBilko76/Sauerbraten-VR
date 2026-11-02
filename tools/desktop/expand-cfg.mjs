#!/usr/bin/env node
// Genera packages/base/<mapa>.cfg para un mapa que no tiene cfg propio (usa los
// slots de data/default_map_settings.cfg): copia la sección de texturas de ese
// fichero con los "exec" de package.cfg expandidos en línea, en el mismo orden,
// para que los índices de slot no cambien y add-normals-cfg.mjs pueda añadir
// relieve solo a este mapa.
//   node tools/desktop/expand-cfg.mjs <mapa>
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
const map = process.argv[2];
if (!map) { console.error('Uso: expand-cfg.mjs <mapa>'); process.exit(1); }
const out = path.join(assets, 'packages', 'base', map + '.cfg');
if (fs.existsSync(out)) { console.error(out + ' ya existe; no lo piso'); process.exit(1); }
let execs = 0;
function expand(rel, depth = 0) {
  const src = fs.readFileSync(path.join(assets, rel), 'utf8').split(/\r?\n/);
  const res = [];
  for (const line of src) {
    const m = /^\s*exec\s+"?([^"\s]+)"?/.exec(line.replace(/\/\/.*$/, ''));
    if (m && depth < 4 && fs.existsSync(path.join(assets, m[1]))) { execs++; res.push(`// --- exec ${m[1]} (expandido) ---`, ...expand(m[1], depth + 1), `// --- fin ${m[1]} ---`); }
    else res.push(line);
  }
  return res;
}
const def = fs.readFileSync(path.join(assets, 'data', 'default_map_settings.cfg'), 'utf8').split(/\r?\n/);
const start = def.findIndex((l) => /^\s*texturereset\b/.test(l));
if (start < 0) { console.error('no encuentro texturereset en default_map_settings.cfg'); process.exit(1); }
const body = [];
for (const line of def.slice(start)) {
  const m = /^\s*exec\s+"?([^"\s]+)"?/.exec(line.replace(/\/\/.*$/, ''));
  if (m && fs.existsSync(path.join(assets, m[1]))) { execs++; body.push(`// --- exec ${m[1]} (expandido) ---`, ...expand(m[1], 1), `// --- fin ${m[1]} ---`); }
  else body.push(line);
}
const header = [
  `// ${map}.cfg generado por tools/desktop/expand-cfg.mjs (SauVREngine): los slots de`,
  `// data/default_map_settings.cfg con sus package.cfg expandidos, en el mismo orden.`,
  `// Así este mapa puede llevar mapas de normales propios sin tocar los demás.`,
  '',
];
fs.writeFileSync(out, header.concat(body).join('\n') + '\n');
const slots = body.filter((l) => /^\s*texture\s+0\b/.test(l)).length;
console.log(`${path.relative(repo, out)}: ${slots} slots, ${execs} exec expandidos, ${body.length} líneas`);
