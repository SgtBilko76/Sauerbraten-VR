#!/usr/bin/env node
// Comprobación estructural de data/menus/vrhub.cfg: que existen la GUI, las
// variables y los enlaces que el plan promete. No ejecuta cubescript; el
// comportamiento se verifica en el motor (npm run desktop:run) y en el visor.
//   node tools/menus/check-vrhub.mjs
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const file = path.join(repo, 'data', 'menus', 'vrhub.cfg');
const failures = [];
const expect = (ok, what) => { if (!ok) failures.push(what); };

if (!fs.existsSync(file)) {
  console.error(`FAIL: falta ${path.relative(repo, file)}`);
  process.exit(1);
}
const src = fs.readFileSync(file, 'utf8');

expect(/newgui\s+"vrhub"/.test(src), 'newgui "vrhub" no definida');
expect(/VRHub_Maps\s+"duel7 ruebli"/.test(src), 'VRHub_Maps no es "duel7 ruebli"');
expect(/VRHub_MapIndex/.test(src), 'falta VRHub_MapIndex');
expect(/VRHub_Comfort/.test(src), 'falta VRHub_Comfort');
expect(/VRHub_Play/.test(src), 'falta VRHub_Play');
expect(/GUI__StartBotMatchWithMap/.test(src), 'no llama a GUI__StartBotMatchWithMap');
expect(/showgui\s+"main"/.test(src), 'falta el botón al menú clásico (showgui "main")');
for (const v of ['GUI_BotCount', 'GUI_MinSkill', 'GUI_MaxSkill', 'vrbright', 'vrfoveation', 'vrstatsscale']) {
  expect(src.includes(v), `no usa ${v}`);
}
expect(/packages\/base\/.*\.jpg/.test(src), 'no carga la miniatura packages/base/<mapa>.jpg');
expect(/data\/cube\.png/.test(src), 'no tiene la imagen de respaldo data/cube.png');

if (failures.length) {
  console.error(`FAIL vrhub.cfg (${failures.length}):`);
  for (const f of failures) console.error(`  - ${f}`);
  process.exit(1);
}
console.log('OK vrhub.cfg: estructura correcta');
