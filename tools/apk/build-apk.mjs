#!/usr/bin/env node
// Compila la APK de este repo y la deja en dist/apk/, la carpeta que sirve
// serve.mjs. Uso (desde la raíz del repo):
//   npm run apk:build [-- --release] [--out dist/apk]
// Toolchain (sin instalador, todo en C:/Android): JDK 17, cmdline-tools,
// platforms;android-34, build-tools;34.0.0, ndk;25.1.8937393, Gradle 8.7.
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

const args = process.argv.slice(2);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 && args[i + 1] ? args[i + 1] : d; };
const release = args.includes('--release');
const src = path.resolve(opt('--src', repo));
const out = path.resolve(opt('--out', process.env.APK_DIR ?? path.join(repo, 'dist', 'apk')));
const androidRoot = process.env.ANDROID_TOOLCHAIN ?? 'C:/Android';
const jdk = fs.readdirSync(androidRoot).find((d) => d.startsWith('jdk-17'));
const gradleDir = fs.readdirSync(androidRoot).find((d) => d.startsWith('gradle-'));
if (!jdk || !gradleDir) { console.error(`Falta JDK o Gradle en ${androidRoot}`); process.exit(1); }
const JAVA_HOME = path.join(androidRoot, jdk);
const ANDROID_HOME = path.join(androidRoot, 'Sdk');
const gradle = path.join(androidRoot, gradleDir, 'bin', 'gradle.bat');
const androidDir = path.join(src, 'android');

fs.writeFileSync(path.join(androidDir, 'local.properties'), `sdk.dir=${ANDROID_HOME.split(path.sep).join('/')}\n`);
const env = { ...process.env, JAVA_HOME, ANDROID_HOME, ANDROID_SDK_ROOT: ANDROID_HOME, PATH: `${path.join(JAVA_HOME, 'bin')};${process.env.PATH}` };
const task = release ? 'assembleRelease' : 'assembleDebug';
console.log(`> gradle ${task} en ${androidDir}`);
const t0 = Date.now();
const shaPre = spawnSync('git', ['rev-parse', '--short', 'HEAD'], { cwd: src, encoding: 'utf8' }).stdout.trim() || 'nogit';
const vcode = Math.floor((Date.now() - Date.UTC(2026, 0, 1)) / 60000); // minutos desde 2026-01-01: crece en cada build
const vname = '0.1.' + vcode + '-' + shaPre;
console.log('> versionCode ' + vcode + ' versionName ' + vname);
const r = spawnSync(gradle, [task, '--no-daemon', '--console=plain', '-Pvcode=' + vcode, '-Pvname=' + vname], { cwd: androidDir, env, stdio: 'inherit', shell: true });
if (r.status !== 0) { console.error(`gradle falló (${r.status})`); process.exit(r.status ?? 1); }

const apkDir = path.join(androidDir, 'app', 'build', 'outputs', 'apk', release ? 'release' : 'debug');
const built = fs.readdirSync(apkDir).find((f) => f.endsWith('.apk'));
if (!built) { console.error(`No hay APK en ${apkDir}`); process.exit(1); }
const sha = spawnSync('git', ['rev-parse', '--short', 'HEAD'], { cwd: src, encoding: 'utf8' }).stdout.trim() || 'nogit';
const stamp = new Date().toISOString().slice(0, 16).replace(/[-:T]/g, '').replace(/(\d{8})(\d{4})/, '$1-$2');
fs.mkdirSync(out, { recursive: true });
const dest = path.join(out, `SauVREngine-${release ? 'release' : 'debug'}-${stamp}-${sha}.apk`);
fs.copyFileSync(path.join(apkDir, built), dest);
const latest = path.join(out, `SauVREngine-latest.apk`);
fs.copyFileSync(path.join(apkDir, built), latest);
const mb = (fs.statSync(dest).size / 1e6).toFixed(1);
console.log(`APK lista: ${dest} (${mb} MB) en ${((Date.now() - t0) / 1000).toFixed(0)} s; también ${latest}`);
