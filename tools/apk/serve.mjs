#!/usr/bin/env node
// Servidor mínimo para bajar APKs al visor desde su navegador.
//   npm run apk:serve [-- carpeta] [--port 5230] [--host 0.0.0.0]
// Carpeta por defecto: dist/apk del repo (o APK_DIR). Sirve todos los
// *.apk de la carpeta con índice, Content-Disposition y soporte de Range
// (el navegador del Quest reanuda descargas grandes).
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import dgram from 'node:dgram';
import { createCaptureSink } from './capture.mjs';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

const args = process.argv.slice(2);
const opt = (name, def) => {
  const i = args.indexOf(name);
  return i >= 0 && args[i + 1] ? args[i + 1] : def;
};
const dir = path.resolve(args.find((a) => !a.startsWith('--') && !/^\d+$/.test(a)) ?? process.env.APK_DIR ?? path.join(repo, 'dist', 'apk'));
const port = Number(opt('--port', process.env.PORT ?? 5230));
const host = opt('--host', '0.0.0.0');

const fmt = (n) => (n >= 1e9 ? (n / 1e9).toFixed(2) + ' GB' : n >= 1e6 ? (n / 1e6).toFixed(1) + ' MB' : n + ' B');

function listApks() {
  if (!fs.existsSync(dir)) return [];
  return fs
    .readdirSync(dir)
    .filter((f) => f.toLowerCase().endsWith('.apk'))
    .map((f) => ({ name: f, size: fs.statSync(path.join(dir, f)).size }));
}

function lanIps() {
  const out = [];
  for (const [, addrs] of Object.entries(os.networkInterfaces())) {
    for (const a of addrs ?? []) if (a.family === 'IPv4' && !a.internal) out.push(a.address);
  }
  return out;
}

function index(res, req) {
  const apks = listApks();
  const rows = apks.length
    ? apks
        .map(
          (a) =>
            `<li><a href="/apk/${encodeURIComponent(a.name)}" download>${a.name}</a> <small>(${fmt(a.size)})</small></li>`,
        )
        .join('')
    : '<li><em>No hay APKs en la carpeta todavía (descarga en curso o carpeta vacía).</em></li>';
  const html = `<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>APKs</title>
<style>body{font:18px/1.5 system-ui;margin:2rem;background:#111;color:#eee}a{color:#7cf;font-size:1.4em}li{margin:1rem 0}</style>
<h1>APKs para el visor</h1><p>Carpeta: <code>${dir}</code></p><ul>${rows}</ul>
<p><small>Tras bajarla: Archivos del Quest → Descargas → tocar la APK → instalar (hace falta modo desarrollador o permitir apps de fuentes desconocidas).</small></p>`;
  res.writeHead(200, { 'content-type': 'text/html; charset=utf-8', 'cache-control': 'no-store' });
  res.end(html);
}

function serveApk(req, res, name) {
  const file = path.join(dir, path.basename(decodeURIComponent(name)));
  if (!file.toLowerCase().endsWith('.apk') || !fs.existsSync(file)) {
    res.writeHead(404, { 'content-type': 'text/plain' });
    return res.end('no existe');
  }
  const size = fs.statSync(file).size;
  const headers = {
    'content-type': 'application/vnd.android.package-archive',
    'content-disposition': `attachment; filename="${path.basename(file)}"`,
    'accept-ranges': 'bytes',
    'cache-control': 'no-store',
  };
  let start = 0;
  let end = size - 1;
  const range = /^bytes=(\d*)-(\d*)$/.exec(req.headers.range ?? '');
  if (range) {
    if (range[1]) start = Number(range[1]);
    if (range[2]) end = Number(range[2]);
    if (!range[1] && range[2]) {
      start = Math.max(0, size - Number(range[2]));
      end = size - 1;
    }
    if (start > end || start >= size) {
      res.writeHead(416, { 'content-range': `bytes */${size}` });
      return res.end();
    }
    headers['content-range'] = `bytes ${start}-${end}/${size}`;
  }
  headers['content-length'] = end - start + 1;
  res.writeHead(range ? 206 : 200, headers);
  if (req.method === 'HEAD') return res.end();
  const t0 = Date.now();
  const stream = fs.createReadStream(file, { start, end });
  stream.pipe(res);
  res.on('close', () => {
    stream.destroy();
    console.log(`${req.socket.remoteAddress} ${range ? '206' : '200'} ${path.basename(file)} ${fmt(end - start + 1)} en ${((Date.now() - t0) / 1000).toFixed(1)} s`);
  });
}

// Telemetría del visor: el motor manda por UDP broadcast (puerto 5231) una línea
// por segundo; se imprime, se guarda en memoria (GET /telemetry?n=60) y en
// dist/telemetry.log para analizarla después.
const telemetry = [];
const telemetryLog = path.join(repo, 'dist', 'telemetry.log');
const capture = createCaptureSink(path.join(repo, 'dist', 'captures'));
const udp = dgram.createSocket({ type: 'udp4', reuseAddr: true });
udp.on('message', (msg, rinfo) => {
  if (capture.binary(msg)) return;
  if (capture.text(msg.toString(), rinfo)) return;
  const line = `${new Date().toISOString().slice(11, 19)} ${rinfo.address} ${msg.toString().trim()}`;
  telemetry.push(line);
  if (telemetry.length > 3600) telemetry.shift();
  console.log(line);
  fs.appendFile(telemetryLog, line + '\n', () => {});
});
udp.on('error', (e) => console.error('telemetría UDP:', e.message));
udp.bind(5231, '0.0.0.0');

const server = http.createServer((req, res) => {
  const url = new URL(req.url ?? '/', 'http://x');
  console.log(new Date().toLocaleTimeString(), req.socket.remoteAddress, req.method, url.pathname, req.headers.range ?? '');
  if (url.pathname === '/' || url.pathname === '/index.html') return index(res, req);
  if (url.pathname === '/telemetry') {
    const n = Number(url.searchParams.get('n') ?? 60);
    res.writeHead(200, { 'content-type': 'text/plain; charset=utf-8', 'cache-control': 'no-store' });
    return res.end(telemetry.slice(-n).join('\n') + '\n');
  }
  if (url.pathname === '/apks.json') {
    res.writeHead(200, { 'content-type': 'application/json' });
    return res.end(JSON.stringify(listApks()));
  }
  const m = /^\/apk\/(.+)$/.exec(url.pathname);
  if (m) return serveApk(req, res, m[1]);
  res.writeHead(404, { 'content-type': 'text/plain' });
  res.end('404');
});

server.listen(port, host, () => {
  console.log(`APK server: carpeta ${dir}`);
  console.log('  telemetría: UDP 5231 → GET /telemetry?n=60 y dist/telemetry.log');
  console.log(`  local:  http://localhost:${port}/`);
  for (const ip of lanIps()) console.log(`  red:    http://${ip}:${port}/   (abrir en el navegador del visor)`);
  for (const a of listApks()) console.log(`  - ${a.name} (${fmt(a.size)})`);
});
