#!/usr/bin/env node
// Clientes simulados contra el servidor dedicado: N procesos de la build de
// escritorio sin pantalla (-H), cada uno un cliente ENet real cuyo jugador lo
// mueve la IA de los bots (simplayer 1). Cada segundo leen su log y muestran
// ping, pérdida, bytes por segundo y estado; al acabar, resumen en dist/sim/.
//   npm run sim:lan -- [--n 8] [--host 192.168.100.21] [--port 28785] [--seconds 120] [--skill 60]
// Requisito: npm run desktop:build (bin64/sauerbraten.exe) y el servidor en marcha (npm run server:lan).
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const exe = path.join(repo, 'bin64', 'sauerbraten.exe');
const assets = path.join(repo, 'android', 'app', 'src', 'main', 'assets');
if (!fs.existsSync(exe)) { console.error('falta bin64/sauerbraten.exe: npm run desktop:build'); process.exit(1); }
const args = process.argv.slice(2);
const opt = (name, def) => (args.includes(name) ? args[args.indexOf(name) + 1] : (process.env['npm_config_' + name.replace(/^--/, '')] ?? def)); // npm se come los --flags: también npm_config_*
const n = +opt('--n', 4);
const host = opt('--host', (() => { for (const a of Object.values(os.networkInterfaces())) for (const i of a ?? []) if (i.family === 'IPv4' && !i.internal) return i.address; return 'localhost'; })());
const port = +opt('--port', 28785);
const seconds = +opt('--seconds', 120);
const skill = +opt('--skill', 60);
const outdir = path.join(repo, 'dist', 'sim');
fs.mkdirSync(outdir, { recursive: true });

const sims = [];
for (let i = 1; i <= n; i++) {
  const home = path.join(outdir, 'home-' + i);
  fs.mkdirSync(home, { recursive: true });
  fs.rmSync(path.join(home, 'log.txt'), { force: true });
  // -q home propio (config y log por cliente), -w/-h mínimos, -t0 ventana (oculta por -H)
  const script = `name sim-${i}; simskill ${skill}; simplayer 1; connect ${host} ${port}`;
  const child = spawn(exe, ['-H', `-q${home}`, '-glog.txt', '-w320', '-h240', '-t0', '-x' + script], { cwd: assets, stdio: 'ignore' });
  sims.push({ i, home, child, last: null, lines: [] });
}
console.log(`${n} clientes simulados → ${host}:${port} durante ${seconds} s (skill ${skill}); logs en ${outdir}`);

const parse = (line) => { const o = {}; for (const t of line.trim().split(/\s+/).slice(1)) { const k = t.indexOf('='); if (k > 0) o[t.slice(0, k)] = t.slice(k + 1); } return o; };
function poll() {
  for (const s of sims) {
    let txt = ''; try { txt = fs.readFileSync(path.join(s.home, 'log.txt'), 'utf8'); } catch { continue; }
    const ls = txt.split(/\r?\n/).filter((l) => l.startsWith('sim ') && l.includes('deaths=')); // solo líneas completas
    if (ls.length) { s.last = parse(ls[ls.length - 1]); s.lines = ls; }
  }
  const rows = sims.map((s) => s.last ? `sim-${s.i}: cn ${s.last.cn} ${s.last.map || '-'} st ${s.last.st} rtt ${s.last.rtt} ms perdida ${s.last.loss}% tx ${(s.last.tx / 1024).toFixed(1)} rx ${(s.last.rx / 1024).toFixed(1)} kB/s frags ${s.last.frags}/${s.last.deaths}` : `sim-${s.i}: conectando…`);
  console.log(new Date().toLocaleTimeString() + '\n  ' + rows.join('\n  '));
}
const timer = setInterval(poll, 5000);

function finish() {
  clearInterval(timer);
  poll();
  const summary = sims.map((s) => {
    const rows = s.lines.map(parse).filter((r) => r.cn && +r.cn >= 0);
    const avg = (k) => (rows.length ? rows.reduce((a, r) => a + (+r[k] || 0), 0) / rows.length : 0);
    const max = (k) => rows.reduce((a, r) => Math.max(a, +r[k] || 0), 0);
    return { sim: s.i, muestras: rows.length, rtt_med: +avg('rtt').toFixed(1), rtt_max: max('rtt'), perdida_med: +avg('loss').toFixed(2), tx_kbs: +(avg('tx') / 1024).toFixed(2), rx_kbs: +(avg('rx') / 1024).toFixed(2), frags: rows.length ? +rows[rows.length - 1].frags : 0, deaths: rows.length ? +rows[rows.length - 1].deaths : 0 };
  });
  const total = { clientes: n, host, port, seconds, rtt_med: +(summary.reduce((a, r) => a + r.rtt_med, 0) / n).toFixed(1), tx_kbs_total: +summary.reduce((a, r) => a + r.tx_kbs, 0).toFixed(2), rx_kbs_total: +summary.reduce((a, r) => a + r.rx_kbs, 0).toFixed(2) };
  fs.writeFileSync(path.join(outdir, 'resumen.json'), JSON.stringify({ total, clientes: summary }, null, 2));
  console.log('\nresumen:', JSON.stringify(total));
  console.table(summary);
  for (const s of sims) { try { s.child.kill(); } catch {} }
  setTimeout(() => process.exit(0), 500);
}
setTimeout(finish, seconds * 1000);
process.on('SIGINT', finish);
