#!/usr/bin/env node
// Añade "texture n <difusa>_n.png" a los slots de un mapa cuya textura difusa
// tenga un mapa de normales generado (gen-normals.ps1), y pone esos slots bajo
// "setshader bumpworld" restaurando el shader original para los demás.
//   node tools/desktop/add-normals-cfg.mjs <packages> base/duel7.cfg
import fs from 'node:fs';
import path from 'node:path';

const [root, cfgRel] = process.argv.slice(2);
if (!root || !cfgRel) { console.error('Uso: add-normals-cfg.mjs <packages> <mapa.cfg relativo>'); process.exit(1); }
const cfgPath = path.join(root, cfgRel);
const src = fs.readFileSync(cfgPath, 'utf8');
const nl = src.includes('\r\n') ? '\r\n' : '\n';
const lines = src.split(/\r?\n/);
const out = [];
let cur = 'stdworld';        // shader que el cfg tiene activo en este punto
let overridden = false;      // hemos metido un setshader bumpworld que hay que deshacer
let added = 0, restored = 0;
const isBump = (s) => /^bump/.test(s);

for (const line of lines) {
  const ss = /^\s*setshader\s+(\S+)/.exec(line);
  if (ss) { cur = ss[1]; overridden = false; out.push(line); continue; }
  const m = /^(\s*)texture\s+0\s+("([^"]+)"|(\S+))(.*)$/.exec(line);
  if (!m) { out.push(line); continue; }
  const indent = m[1], tex = m[3] ?? m[4], rest = m[5] ?? '';
  const nrel = tex.replace(/\.[^./]+$/, '') + '_n.png';
  const hasN = fs.existsSync(path.join(root, nrel));
  if (hasN && !isBump(cur)) {
    if (!overridden) { out.push(indent + 'setshader bumpworld // SauVREngine: relieve generado'); overridden = true; }
    out.push(line);
    out.push(`${indent}texture n "${nrel}"${rest}`);
    added++;
  } else if (hasN) {
    out.push(line);
    out.push(`${indent}texture n "${nrel}"${rest}`);
    added++;
  } else {
    if (overridden) { out.push(indent + 'setshader ' + cur); overridden = false; restored++; }
    out.push(line);
  }
}
fs.copyFileSync(cfgPath, cfgPath + '.bak');
fs.writeFileSync(cfgPath, out.join(nl));
console.log(`${cfgRel}: ${added} slots con normal map, ${restored} restauraciones de shader; copia en ${cfgRel}.bak`);
