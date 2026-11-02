import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

export function transform(source, manifest, mode) {
  if (!['original', 'enhanced'].includes(mode)) throw new Error('Expected original or enhanced');
  const mappings = new Map();
  for (const t of manifest.textures) {
    if (!t.logical.every(n => Number.isInteger(n) && n > 0 && n <= 16384)) throw new Error('Invalid logical size');
    const enhanced = `<logical:${t.logical.join(',')}>${t.replacement}`;
    const target = mode === 'enhanced' ? enhanced : t.source;
    mappings.set(t.source, target); mappings.set(enhanced, target);
  }
  // Only diffuse slots: preserve slot order, normals, rotation, scale and comments.
  return source.replace(/^(\s*texture\s+0\s+")([^"\r\n]+)(")/gm,
    (_, before, name, after) => before + (mappings.get(name) ?? name) + after);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
  const root = path.join(repo, 'android/app/src/main/assets/packages');
  const manifest = JSON.parse(fs.readFileSync(path.join(repo, 'tools/textures/detail-pack.json')));
  const mode = process.argv[2];
  if (!['original', 'enhanced'].includes(mode)) throw new Error('Usage: node tools/textures/detail-pack.mjs original|enhanced');
  // Validate all dependencies before changing any map.
  for (const t of manifest.textures) for (const key of ['source', 'replacement']) {
    const p = path.resolve(root, t[key]);
    if (!p.startsWith(root + path.sep) || !fs.existsSync(p)) throw new Error(`Missing/invalid ${t[key]}`);
  }
  const edits = manifest.maps.map(relative => {
    const p = path.resolve(root, relative);
    if (!p.startsWith(root + path.sep)) throw new Error('Invalid map');
    return [p, transform(fs.readFileSync(p, 'utf8'), manifest, mode)];
  });
  for (const [p, content] of edits) fs.writeFileSync(p, content);
  console.log(`${mode}: ${edits.length} map configurations; reload the map / rebuild APK.`);
}
