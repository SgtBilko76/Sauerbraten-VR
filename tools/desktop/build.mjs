#!/usr/bin/env node
// Build de escritorio (Windows x64) del motor con llvm-mingw, sin make.
// Sirve para hornear cosas en el PC (PVS, luces) y para editar mapas.
//   npm run desktop:build [-- --clean]
// Toolchain y dependencias, todo en C:/Android sin instalador:
//   llvm-mingw-*            clang++ / lld      (zip de mstorsjo/llvm-mingw)
//   dl/sdl/SDL2-*           SDL2, SDL2_image, SDL2_mixer (paquetes mingw oficiales)
//   dl/zlib/zlib-*          zlib compilado dentro del exe
// Salida: bin64/sauerbraten.exe + DLLs (bin64/ está en .gitignore).
import { spawn, spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const src = path.join(repo, 'src');
const objdir = path.join(src, 'build-win64');
const bindir = path.join(repo, 'bin64');
const clean = process.argv.includes('--clean');
const androidRoot = process.env.ANDROID_TOOLCHAIN ?? 'C:/Android';

function findDir(base, prefix) {
  const d = fs.readdirSync(base).find((n) => n.startsWith(prefix));
  if (!d) throw new Error(`No encuentro ${prefix}* en ${base}`);
  return path.join(base, d);
}
const llvm = findDir(androidRoot, 'llvm-mingw-');
const sdl = findDir(path.join(androidRoot, 'dl', 'sdl'), 'SDL2-2');
const sdlImage = findDir(path.join(androidRoot, 'dl', 'sdl'), 'SDL2_image-');
const sdlMixer = findDir(path.join(androidRoot, 'dl', 'sdl'), 'SDL2_mixer-');
const zlib = findDir(path.join(androidRoot, 'dl', 'zlib'), 'zlib-');
const cxx = path.join(llvm, 'bin', 'x86_64-w64-mingw32-clang++.exe');
const cc = path.join(llvm, 'bin', 'x86_64-w64-mingw32-clang.exe');
const sdlRoot = (p) => path.join(p, 'x86_64-w64-mingw32');

// Lista de objetos del cliente tal como la define src/Makefile.
const mk = fs.readFileSync(path.join(src, 'Makefile'), 'utf8');
const m = /CLIENT_OBJS=([\s\S]*?)\r?\n\r?\n/.exec(mk);
if (!m) throw new Error('CLIENT_OBJS no encontrado en src/Makefile');
const clientObjs = m[1].replace(/\\\r?\n/g, ' ').split(/\s+/).filter((t) => t.endsWith('.o'));
const cppSources = clientObjs.map((o) => o.replace(/\.o$/, '.cpp'));
const enetSources = fs.readdirSync(path.join(src, 'enet')).filter((f) => f.endsWith('.c')).map((f) => 'enet/' + f);
const zlibFiles = ['adler32', 'compress', 'crc32', 'deflate', 'infback', 'inffast', 'inflate', 'inftrees', 'trees', 'uncompr', 'zutil', 'gzclose', 'gzlib', 'gzread', 'gzwrite'];

const includes = [
  '-I' + path.join(sdlRoot(sdl), 'include', 'SDL2'),
  '-I' + path.join(sdlRoot(sdlImage), 'include', 'SDL2'),
  '-I' + path.join(sdlRoot(sdlMixer), 'include', 'SDL2'),
  '-I' + zlib,
  '-Ishared', '-Iengine', '-Ifpsgame', '-Ienet/include',
];
const cxxflags = ['-O2', '-fsigned-char', '-fno-exceptions', '-fno-rtti', '-DWIN32', '-D_WIN32_WINNT=0x0601', '-w', '-std=gnu++11', ...includes];
const cflags = ['-O2', '-DWIN32', '-DHAS_SOCKLEN_T=1', '-w', '-Ienet/include', '-I' + zlib];

if (clean) fs.rmSync(objdir, { recursive: true, force: true });
fs.mkdirSync(objdir, { recursive: true });
fs.mkdirSync(bindir, { recursive: true });

const jobs = [];
for (const s of cppSources) jobs.push({ tool: cxx, flags: cxxflags, srcFile: path.join(src, s), obj: path.join(objdir, s.replace(/[\/\\]/g, '_') + '.o') });
for (const s of enetSources) jobs.push({ tool: cc, flags: cflags, srcFile: path.join(src, s), obj: path.join(objdir, s.replace(/[\/\\]/g, '_') + '.o') });
for (const z of zlibFiles) jobs.push({ tool: cc, flags: cflags, srcFile: path.join(zlib, z + '.c'), obj: path.join(objdir, 'zlib_' + z + '.o') });

const headerStamp = Math.max(...['shared', 'engine', 'fpsgame'].flatMap((d) => fs.readdirSync(path.join(src, d)).filter((f) => f.endsWith('.h')).map((f) => fs.statSync(path.join(src, d, f)).mtimeMs)));
const pending = jobs.filter((j) => {
  if (!fs.existsSync(j.obj)) return true;
  const o = fs.statSync(j.obj).mtimeMs;
  return o < fs.statSync(j.srcFile).mtimeMs || (j.tool === cxx && o < headerStamp);
});
console.log(`> ${pending.length} de ${jobs.length} objetos por compilar (${os.cpus().length} hilos)`);

let failed = 0;
async function runJob(j) {
  return new Promise((resolve) => {
    const p = spawn(j.tool, [...j.flags, '-c', j.srcFile, '-o', j.obj], { cwd: src });
    let err = '';
    p.stderr.on('data', (d) => (err += d));
    p.on('close', (code) => {
      if (code !== 0) { failed++; console.error(`\n--- ${path.relative(src, j.srcFile)} falló:\n${err.split('\n').slice(0, 40).join('\n')}`); }
      else process.stdout.write('.');
      resolve();
    });
  });
}
const t0 = Date.now();
const queue = [...pending];
await Promise.all(Array.from({ length: Math.max(1, os.cpus().length) }, async () => { while (queue.length) await runJob(queue.shift()); }));
console.log();
if (failed) { console.error(`${failed} ficheros fallaron`); process.exit(1); }

const exe = path.join(bindir, 'sauerbraten.exe');
const libs = ['-L' + path.join(sdlRoot(sdl), 'lib'), '-L' + path.join(sdlRoot(sdlImage), 'lib'), '-L' + path.join(sdlRoot(sdlMixer), 'lib'),
  '-lmingw32', '-lSDL2main', '-lSDL2', '-lSDL2_image', '-lSDL2_mixer', '-lopengl32', '-lws2_32', '-lwinmm',
  // SDL estática (por -static) tira de estas librerías de sistema
  '-lsetupapi', '-lole32', '-loleaut32', '-limm32', '-lversion', '-lgdi32', '-luser32', '-lshell32', '-ladvapi32', '-luuid', '-lrpcrt4'];
const link = spawnSync(cxx, ['-O2', '-static', '-mwindows', '-o', exe, ...jobs.map((j) => j.obj), ...libs], { cwd: src, encoding: 'utf8' });
if (link.status !== 0) { console.error(link.stderr); process.exit(1); }
for (const [root, dll] of [[sdl, 'SDL2.dll'], [sdlImage, 'SDL2_image.dll'], [sdlMixer, 'SDL2_mixer.dll']])
  fs.copyFileSync(path.join(sdlRoot(root), 'bin', dll), path.join(bindir, dll));
console.log(`OK: ${exe} (${(fs.statSync(exe).size / 1e6).toFixed(1)} MB) en ${((Date.now() - t0) / 1000).toFixed(0)} s`);
