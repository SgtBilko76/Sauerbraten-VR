import { test } from 'node:test';
import assert from 'node:assert/strict';
import { transform } from '../tools/textures/detail-pack.mjs';
const manifest = { textures: [{source:'old.jpg', replacement:'new.jpg', logical:[128,128]}] };
const original = '// texture 0 "old.jpg"\r\ntexture 0 "old.jpg" 1 4 8 0.5 // floor\r\ntexture n "old.jpg"\r\ntexture 0 "other.jpg"\r\n';
test('preserves normal maps, slot arguments, comments and line endings', () => {
  const result = transform(original, manifest, 'enhanced');
  assert.equal(result, original.replace('\r\ntexture 0 "old.jpg"', '\r\ntexture 0 "<logical:128,128>new.jpg"'));
  assert.equal(transform(result, manifest, 'original'), original);
  assert.equal(transform(result, manifest, 'enhanced'), result);
});
test('rejects invalid modes and dimensions', () => {
  assert.throws(() => transform(original, manifest, 'invalid'));
  assert.throws(() => transform(original, {textures:[{...manifest.textures[0],logical:[0,128]}]}, 'enhanced'));
});
