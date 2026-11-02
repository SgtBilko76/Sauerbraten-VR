// Reensambla capturas de pantalla que el visor manda por UDP en trozos y las
// guarda como PNG en dist/captures/. Usado por serve.mjs.
//   "sauvrimg begin <w> <h> <total> <id>"  (texto)  → empieza una captura
//   "SVRI" + id(1) + seq(2 LE) + RGB...      (binario) → trozo seq
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';

const crcTable = new Int32Array(256).map((_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c; });
function crc32(buf) { let c = -1; for (const b of buf) c = crcTable[(c ^ b) & 0xff] ^ (c >>> 8); return (c ^ -1) >>> 0; }
function chunk(type, data) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, 'ascii'), data]);
  const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
export function encodePNG(rgb, w, h) {
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) { raw[y * (w * 3 + 1)] = 0; rgb.copy(raw, y * (w * 3 + 1) + 1, y * w * 3, (y + 1) * w * 3); }
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
  return Buffer.concat([Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}

export function createCaptureSink(outdir, log = console.log) {
  fs.mkdirSync(outdir, { recursive: true });
  let cur = null; // { id, w, h, total, chunks: Map, timer }
  function finish() {
    if (!cur) return;
    const c = cur; cur = null; clearTimeout(c.timer);
    const size = c.chunks.size ? [...c.chunks.values()][0].length : 0;
    const rgb = Buffer.alloc(c.w * c.h * 3);
    let got = 0;
    for (let i = 0; i < c.total; i++) { const b = c.chunks.get(i); if (b) { b.copy(rgb, i * size); got++; } }
    const file = path.join(outdir, `capture-${new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19)}.png`);
    fs.writeFileSync(file, encodePNG(rgb, c.w, c.h));
    log(`captura ${c.w}x${c.h}: ${got}/${c.total} trozos → ${file}`);
  }
  return {
    text(line, rinfo) {
      const m = /^sauvrimg begin (\d+) (\d+) (\d+) (\d+)/.exec(line);
      if (!m) return false;
      finish();
      cur = { w: +m[1], h: +m[2], total: +m[3], id: +m[4], chunks: new Map(), timer: setTimeout(finish, 3000) };
      log(`captura entrante de ${rinfo.address}: ${cur.w}x${cur.h}, ${cur.total} trozos`);
      return true;
    },
    binary(msg) {
      if (msg.length < 7 || msg.toString('ascii', 0, 4) !== 'SVRI') return false;
      if (!cur || msg[4] !== cur.id) return true;
      const seq = msg.readUInt16LE(5);
      cur.chunks.set(seq, msg.subarray(7));
      clearTimeout(cur.timer); cur.timer = setTimeout(finish, 1500);
      if (cur.chunks.size >= cur.total) finish();
      return true;
    },
  };
}
