#!/usr/bin/env node
// Genera el mapa "hell" (infierno de metal, lava y roca) para SauVREngine.
// Escribe dos ficheros en android/app/src/main/assets/packages/base/:
//   hell.ogz  octree real del motor (MAPVERSION 33, ver src/engine/worldio.cpp)
//   hell.cfg  texturas propias, cielo y niebla (se ejecuta tras default_map_settings.cfg)
//   node tools/desktop/gen-hell-map.mjs [--seed n]
// Determinista: el mismo seed produce el mismo mapa. Los vóxeles se dibujan en una
// rejilla de 8 unidades y se pliegan en octree: una región uniforme es un solo cubo.
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const outDir = path.join(repo, 'android', 'app', 'src', 'main', 'assets', 'packages', 'base');
const seedArg = process.argv.indexOf('--seed');
const seed = seedArg > 0 ? Number(process.argv[seedArg + 1]) : 666;

// ---- formato (src/engine/world.h, worldio.cpp, octa.h, shared/iengine.h, shared/ents.h)
const MAPVERSION = 33;
const WORLD = 1024;                       // worldsize, potencia de 2
const CELL = 8;                           // lado del cubo más pequeño, en unidades
const G = WORLD / CELL;                   // celdas por lado de la rejilla
const OCTSAV_CHILDREN = 0, OCTSAV_EMPTY = 1, OCTSAV_SOLID = 2;
const MAT_LAVA = 2 << 2;                  // MAT_LAVA con MATF_VOLUME_SHIFT = 2
const LAVA_MAT = MAT_LAVA | 3;            // matlava 3 (ow/lava_c.jpg) de default_map_settings.cfg
const ET = { LIGHT: 1, PLAYERSTART: 3, SHELLS: 8, BULLETS: 9, ROCKETS: 10, RIFLEROUNDS: 11,
    GRENADES: 12, CARTRIDGES: 13, HEALTH: 14, HEALTHBOOST: 15, GREENARMOUR: 16, YELLOWARMOUR: 17, QUADDAMAGE: 18 };

// Código de vóxel -> slot de textura (orden de los slots en hell.cfg)
const V = { AIR: 0, ROCK: 1, CLIFF: 2, METAL: 3, RAIL: 4, BRICK: 5, LAVA: 6, FLOOR: 7 };
const SLOT = [1, 2, 3, 4, 5, 6, 1, 7];

// ---- geometría del mapa, en celdas (x, y, z); z=FLOOR_Z es el suelo pisable
const FLOOR_Z = 5;                        // celda sólida superior del suelo; aire desde 6
const C = G / 2;                          // centro del mapa
const FS = 28;                            // lado de cada fortaleza
const FORTS = [
    { x0: 8, y0: 8, door: 'E' }, { x0: 92, y0: 8, door: 'W' },
    { x0: 8, y0: 92, door: 'E' }, { x0: 92, y0: 92, door: 'W' },
];
const CHANNELS = [[80, 118, 40, 43], [40, 43, 80, 118], [5, 47, 84, 87], [84, 87, 5, 47]];
const BRIDGES = [[22, 105, 62, 66], [62, 66, 22, 105], [98, 102, 36, 47],
    [80, 91, 20, 24], [34, 49, 100, 104], [20, 24, 80, 91]];

const rnd = mulberry32(seed);
const ri = (a, b) => a + Math.floor(rnd() * (b - a + 1));

function mulberry32(a) {
    return () => {
        a = (a + 0x6D2B79F5) | 0;
        let t = Math.imul(a ^ (a >>> 15), 1 | a);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

// ---- rejilla de vóxeles
const grid = new Uint8Array(G * G * G);
const at = (x, y, z) => (z * G + y) * G + x;
const inside = (x, y, z) => x >= 0 && y >= 0 && z >= 0 && x < G && y < G && z < G;
const get = (x, y, z) => (inside(x, y, z) ? grid[at(x, y, z)] : V.ROCK);

function box(x0, x1, y0, y1, z0, z1, v) {
    for (let z = Math.max(z0, 0); z <= Math.min(z1, G - 1); z++)
        for (let y = Math.max(y0, 0); y <= Math.min(y1, G - 1); y++)
            for (let x = Math.max(x0, 0); x <= Math.min(x1, G - 1); x++) grid[at(x, y, z)] = v;
}

// marco de grosor t alrededor de una caja en el plano XY
function frame(x0, x1, y0, y1, t, z0, z1, v) {
    for (let y = y0; y <= y1; y++)
        for (let x = x0; x <= x1; x++)
            if (x - x0 < t || x1 - x < t || y - y0 < t || y1 - y < t) box(x, x, y, y, z0, z1, v);
}

// ---- lago central con borde irregular
const lakeR = (ang) => 19 + 2.5 * Math.sin(ang * 5) + 1.5 * Math.sin(ang * 3 + 1);
function inLake(x, y) {
    const dx = x + 0.5 - C, dy = y + 0.5 - C;
    return Math.hypot(dx, dy) <= lakeR(Math.atan2(dy, dx));
}

// 1. suelo de roca con pisada de metal/roca a FLOOR_Z
box(0, G - 1, 0, G - 1, 0, FLOOR_Z - 1, V.ROCK);
box(0, G - 1, 0, G - 1, FLOOR_Z, FLOOR_Z, V.FLOOR);

// 2. acantilado exterior abierto arriba (se ve el cielo)
frame(0, G - 1, 0, G - 1, 3, 6, 22, V.CLIFF);

// 3. lago de lava hundido: roca en z 0..1, lava en z 2..4, aire desde 5
for (let y = 0; y < G; y++)
    for (let x = 0; x < G; x++)
        if (inLake(x, y)) { box(x, x, y, y, 2, 4, V.LAVA); box(x, x, y, y, FLOOR_Z, FLOOR_Z, V.AIR); }

// isla central de metal (los puentes la cruzan)
for (let y = C - 3; y <= C + 3; y++)
    for (let x = C - 3; x <= C + 3; x++)
        if ((x - C) ** 2 + (y - C) ** 2 <= 9) { box(x, x, y, y, 2, 4, V.ROCK); box(x, x, y, y, FLOOR_Z, FLOOR_Z, V.METAL); }

// 4. ríos de lava en trincheras: mismas capas que el lago
for (const [xa, xb, ya, yb] of CHANNELS) {
    box(xa, xb, ya, yb, FLOOR_Z, FLOOR_Z, V.AIR);
    box(xa, xb, ya, yb, 2, 4, V.LAVA);
}

// 5. puentes de metal a ras de suelo con barandilla en los lados largos
for (const [xa, xb, ya, yb] of BRIDGES) {
    box(xa, xb, ya, yb, FLOOR_Z, FLOOR_Z, V.METAL);
    if (xb - xa >= yb - ya) { box(xa, xb, ya, ya, 6, 7, V.RAIL); box(xa, xb, yb, yb, 6, 7, V.RAIL); }
    else { box(xa, xa, ya, yb, 6, 7, V.RAIL); box(xb, xb, ya, yb, 6, 7, V.RAIL); }
}

// 6. fortalezas de ladrillo con tabique, techo con claraboya y patio de lava
for (const f of FORTS) {
    const x0 = f.x0, y0 = f.y0, x1 = x0 + FS - 1, y1 = y0 + FS - 1;
    box(x0, x1, y0, y1, FLOOR_Z, FLOOR_Z, V.METAL);
    box(x0, x1, y0, y1, 6, 17, V.AIR);
    frame(x0, x1, y0, y1, 2, 6, 17, V.BRICK);
    box(x0, x1, y0, y1, 18, 18, V.METAL);
    box(x0 + 11, x0 + 16, y0 + 11, y0 + 16, 18, 18, V.AIR);
    box(x0 + 11, x0 + 16, y0 + 11, y0 + 16, 2, 4, V.LAVA);
    box(x0 + 11, x0 + 16, y0 + 11, y0 + 16, FLOOR_Z, FLOOR_Z, V.AIR);
    if (f.door === 'E') box(x1 - 1, x1, y0 + 12, y0 + 15, 6, 12, V.AIR);
    else box(x0, x0 + 1, y0 + 12, y0 + 15, 6, 12, V.AIR);
    box(x0 + 14, x0 + 14, y0 + 2, y1 - 2, 6, 17, V.RAIL);
    box(x0 + 14, x0 + 14, y0 + 12, y0 + 15, 6, 17, V.AIR);
    for (const [px, py] of [[5, 5], [21, 5], [5, 21], [21, 21]]) box(x0 + px, x0 + px + 1, y0 + py, y0 + py + 1, 6, 17, V.METAL);
}

// 7. torres de roca dispersas (solo sobre suelo limpio, con margen)
function cleanFloor(x, y, m) {
    for (let dy = -m; dy <= m; dy++)
        for (let dx = -m; dx <= m; dx++)
            if (get(x + dx, y + dy, FLOOR_Z) !== V.FLOOR) return false;
    return true;
}
let spires = 0;
for (let t = 0; t < 400 && spires < 36; t++) {
    const x = ri(7, G - 8), y = ri(7, G - 8), r = ri(1, 2), h = ri(4, 12);
    if (!cleanFloor(x, y, r + 1)) continue;
    for (let dy = -r; dy <= r; dy++)
        for (let dx = -r; dx <= r; dx++)
            if (dx * dx + dy * dy <= r * r + 0.5) box(x + dx, x + dx, y + dy, y + dy, 6, 6 + h - 1, V.CLIFF);
    spires++;
}

// ---- entidades (coordenadas en unidades: celda * CELL + CELL/2)
const ents = [];
const unit = (c) => c * CELL + CELL / 2;
const FEET_Z = (FLOOR_Z + 1) * CELL;      // altura del suelo pisable en unidades
const add = (type, x, y, z, a = []) => ents.push({ type, o: [x, y, z], a: [...a, 0, 0, 0, 0, 0].slice(0, 5) });

// celda pisable y despejada en un radio, sin lava cerca
function freeSpot(x, y, r) {
    for (let dy = -r; dy <= r; dy++)
        for (let dx = -r; dx <= r; dx++) {
            const f = get(x + dx, y + dy, FLOOR_Z);
            if (f !== V.FLOOR && f !== V.METAL) return false;
            for (let z = FLOOR_Z + 1; z <= FLOOR_Z + 3; z++) if (get(x + dx, y + dy, z) !== V.AIR) return false;
        }
    for (let dy = -2; dy <= 2; dy++)
        for (let dx = -2; dx <= 2; dx++)
            for (let z = 2; z <= FLOOR_Z; z++) if (get(x + dx, y + dy, z) === V.LAVA) return false;
    return true;
}
const yawToCenter = (x, y) => {
    const ux = C - x, uy = C - y, n = Math.hypot(ux, uy) || 1;
    return Math.round((Math.atan2(-ux / n, uy / n) * 180 / Math.PI + 360) % 360);
};

// salidas de jugador: 2 en cada fortaleza y 10 repartidas por la arena
const spawnCells = [];
for (const f of FORTS) {
    for (const [dx, dy] of [[4, 8], [24, 24]]) spawnCells.push([f.x0 + dx, f.y0 + dy]);
}
for (let t = 0, n = 0; t < 4000 && n < 10; t++) {
    const x = ri(8, G - 9), y = ri(8, G - 9);
    if (spawnCells.some(([sx, sy]) => Math.hypot(sx - x, sy - y) < 14)) continue;
    if (!freeSpot(x, y, 1)) continue;
    spawnCells.push([x, y]);
    n++;
}
for (const [x, y] of spawnCells) add(ET.PLAYERSTART, unit(x), unit(y), FEET_Z, [yawToCenter(x, y), 0, 0, 0, 0]);

// objetos: rotación de armas y vida, cuádruple daño en el centro
const itemCycle = [ET.SHELLS, ET.BULLETS, ET.ROCKETS, ET.HEALTH, ET.RIFLEROUNDS, ET.GRENADES,
    ET.CARTRIDGES, ET.HEALTHBOOST, ET.GREENARMOUR, ET.YELLOWARMOUR];
add(ET.QUADDAMAGE, unit(C), unit(C), FEET_Z + 2);
for (let t = 0, n = 0; t < 4000 && n < 34; t++) {
    const x = ri(6, G - 7), y = ri(6, G - 7);
    if (!freeSpot(x, y, 1)) continue;
    add(itemCycle[n % itemCycle.length], unit(x), unit(y), FEET_Z + 2);
    n++;
}

// luces: anillo de lava alrededor del lago, una en cada sala y una en el centro
for (let i = 0; i < 8; i++) {
    const a = (i / 8) * Math.PI * 2;
    add(ET.LIGHT, unit(C + Math.cos(a) * 27), unit(C + Math.sin(a) * 27), 60, [260, 255, 120, 40]);
}
for (const f of FORTS) {
    add(ET.LIGHT, unit(f.x0 + 7), unit(f.y0 + 20), 100, [220, 255, 90, 30]);
    add(ET.LIGHT, unit(f.x0 + 22), unit(f.y0 + 7), 100, [220, 255, 90, 30]);
}
add(ET.LIGHT, unit(C), unit(C), 90, [300, 255, 70, 20]);

// ---- escritura binaria (little endian, como savec/save_world)
class Writer {
    constructor() { this.buf = Buffer.alloc(1 << 22); this.n = 0; }
    ensure(k) {
        if (this.n + k <= this.buf.length) return;
        const nb = Buffer.alloc(Math.max(this.buf.length * 2, this.n + k));
        this.buf.copy(nb, 0, 0, this.n);
        this.buf = nb;
    }
    u8(v) { this.ensure(1); this.buf[this.n++] = v & 0xFF; }
    u16(v) { this.ensure(2); this.buf.writeUInt16LE(v & 0xFFFF, this.n); this.n += 2; }
    i16(v) { this.ensure(2); this.buf.writeInt16LE(v, this.n); this.n += 2; }
    i32(v) { this.ensure(4); this.buf.writeInt32LE(v, this.n); this.n += 4; }
    f32(v) { this.ensure(4); this.buf.writeFloatLE(v, this.n); this.n += 4; }
    str(s) { this.ensure(s.length); this.buf.write(s, this.n, 'latin1'); this.n += s.length; }
    bytes() { return this.buf.subarray(0, this.n); }
}

const w = new Writer();
let leaves = 0, lava = 0;

// región uniforme -> código; si no lo es, -1
function uniform(x, y, z, s) {
    const v = grid[at(x, y, z)];
    for (let k = z; k < z + s; k++)
        for (let j = y; j < y + s; j++)
            for (let i = x; i < x + s; i++) if (grid[at(i, j, k)] !== v) return -1;
    return v;
}

// formato de cubo hoja: flags, 6 texturas, material si lo hay (savec en worldio.cpp)
function leaf(v) {
    leaves++;
    let flags = OCTSAV_SOLID, mat = -1;
    if (v === V.AIR) flags = OCTSAV_EMPTY;
    else if (v === V.LAVA) { flags = 0x40 | OCTSAV_EMPTY; mat = LAVA_MAT; lava++; }
    w.u8(flags);
    for (let f = 0; f < 6; f++) w.u16(SLOT[v]);
    if (mat >= 0) w.u16(mat);
}

// octant i: x en bit 0, y en bit 1, z en bit 2 (ivec(i, co, size) en shared/geom.h)
function emit(x, y, z, s) {
    const v = uniform(x, y, z, s);
    if (v >= 0) { leaf(v); return; }
    w.u8(OCTSAV_CHILDREN);
    const h = s >> 1;
    for (let i = 0; i < 8; i++) emit(x + (i & 1) * h, y + ((i >> 1) & 1) * h, z + ((i >> 2) & 1) * h, h);
}

// cabecera octaheader: 4 bytes de magia + 9 enteros
w.str('OCTA');
w.i32(MAPVERSION); w.i32(40); w.i32(WORLD); w.i32(ents.length);
w.i32(0); w.i32(0); w.i32(0); w.i32(0); w.i32(0);
// nombre del juego, datos de entidad y de juego (vacíos), MRU de texturas (vacío)
w.u8(3); w.str('fps\0');
w.u16(0); w.u16(0); w.u16(0);
// entidades: vec o, attr1..5, type, reserved (24 bytes)
for (const e of ents) {
    e.o.forEach((v) => w.f32(v));
    e.a.forEach((v) => w.i16(v));
    w.u8(e.type); w.u8(0);
}
// vslots: ninguno. octree: los 8 octantes raíz, cada uno de WORLD/2 unidades
for (let i = 0; i < 8; i++) emit((i & 1) * (G / 2), ((i >> 1) & 1) * (G / 2), ((i >> 2) & 1) * (G / 2), G / 2);

fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, 'hell.ogz'), zlib.gzipSync(w.bytes()));

// ---- lava: los lava_c.jpg de default_map_settings.cfg no están en los assets, así que
// se genera una textura de lava periódica (tileable) en packages/hell/lava.png
const LAVA_PNG = path.join(path.dirname(outDir), 'hell', 'lava.png');
const LAVA_SIZE = 256;
const CRC_TABLE = Array.from({ length: 256 }, (_, n) => {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
    return c >>> 0;
});
function crc32(buf) {
    let c = 0xFFFFFFFF;
    for (const b of buf) c = CRC_TABLE[(c ^ b) & 0xFF] ^ (c >>> 8);
    return (c ^ 0xFFFFFFFF) >>> 0;
}
function pngChunk(type, data) {
    const len = Buffer.alloc(4), crc = Buffer.alloc(4);
    len.writeUInt32BE(data.length);
    const td = Buffer.concat([Buffer.from(type, 'latin1'), data]);
    crc.writeUInt32BE(crc32(td));
    return Buffer.concat([len, td, crc]);
}
function encodePng(size, rgb) {
    const raw = Buffer.alloc((size * 3 + 1) * size);
    for (let y = 0; y < size; y++) rgb.copy(raw, y * (size * 3 + 1) + 1, y * size * 3, (y + 1) * size * 3);
    const ihdr = Buffer.alloc(13);
    ihdr.writeUInt32BE(size, 0); ihdr.writeUInt32BE(size, 4);
    ihdr[8] = 8; ihdr[9] = 2;             // 8 bits por canal, RGB
    return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), pngChunk('IHDR', ihdr),
        pngChunk('IDAT', zlib.deflateSync(raw)), pngChunk('IEND', Buffer.alloc(0))]);
}
// ruido de valor periódico: la rejilla da la vuelta, así la textura se repite sin costuras
function noiseLayer(size, cells, rand) {
    const grid = Float32Array.from({ length: cells * cells }, () => rand());
    const out = new Float32Array(size * size), s = cells / size;
    for (let y = 0; y < size; y++)
        for (let x = 0; x < size; x++) {
            const fx = x * s, fy = y * s, x0 = Math.floor(fx), y0 = Math.floor(fy);
            const tx = fx - x0, ty = fy - y0, sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty);
            const ax = x0 % cells, ay = y0 % cells, bx = (ax + 1) % cells, by = (ay + 1) % cells;
            const top = grid[ay * cells + ax] + (grid[ay * cells + bx] - grid[ay * cells + ax]) * sx;
            const bot = grid[by * cells + ax] + (grid[by * cells + bx] - grid[by * cells + ax]) * sx;
            out[y * size + x] = top + (bot - top) * sy;
        }
    return out;
}
const LAVA_STOPS = [[0, [40, 4, 0]], [0.35, [150, 20, 0]], [0.6, [255, 90, 10]], [0.85, [255, 190, 60]], [1, [255, 250, 190]]];
function lavaColour(t) {
    for (let i = 1; i < LAVA_STOPS.length; i++) {
        const [t1, c1] = LAVA_STOPS[i];
        if (t <= t1) {
            const [t0, c0] = LAVA_STOPS[i - 1], k = (t - t0) / (t1 - t0);
            return c0.map((v, j) => Math.round(v + (c1[j] - v) * k));
        }
    }
    return LAVA_STOPS[LAVA_STOPS.length - 1][1];
}
{
    const lrand = mulberry32(seed + 1);
    const n = new Float32Array(LAVA_SIZE * LAVA_SIZE);
    for (const [cells, weight] of [[4, 0.5], [8, 0.25], [16, 0.15], [32, 0.1]]) {
        const layer = noiseLayer(LAVA_SIZE, cells, lrand);
        for (let i = 0; i < n.length; i++) n[i] += layer[i] * weight;
    }
    const rgb = Buffer.alloc(LAVA_SIZE * LAVA_SIZE * 3);
    for (let i = 0; i < n.length; i++) {
        const veins = (1 - Math.abs(n[i] * 2 - 1)) ** 3;   // grietas brillantes donde el ruido pasa por 0.5
        rgb.set(lavaColour(veins), i * 3);
    }
    fs.mkdirSync(path.dirname(LAVA_PNG), { recursive: true });
    fs.writeFileSync(LAVA_PNG, encodePng(LAVA_SIZE, rgb));
}

// ---- cfg: texturas propias (slots 0 y 1 fijos), cielo y niebla
const cfg = `// hell.cfg generado por tools/desktop/gen-hell-map.mjs (SauVREngine).
// Infierno de metal, lava y roca: suelo de roca, lago y ríos de lava, puentes de
// metal, fortalezas de ladrillo con patio de lava y la torre central.
// Se ejecuta después de data/default_map_settings.cfg.

// lava propia: los lava_c.jpg de default_map_settings.cfg no están en los assets
matlava 3 ["hell/lava.png" 0 0 0 1] ["hell/lava.png" 0 0 0 1]

skybox "skyboxes/turbulent"
fog 1200
fogcolour 0x3A0E08
ambient 0x2A1410

texturereset
    setshader "stdworld"
        texture 0 "textures/sky.png"                       // [0] cielo fijo
        texture 0 "textures/default.png"                   // [1] textura por defecto
    setshader bumpworld
        texture 0 "textures/terrain_soc/rock_brown3.jpg"   // [2] roca
        texture n "textures/terrain_soc/rock_brown3_n.png"
        texture 0 "textures/nieb/rock01.jpg"               // [3] acantilado
        texture n "textures/nieb/rock01_n.png"
        texture 0 "textures/medieval_soc/metal2.jpg"       // [4] metal
        texture n "textures/medieval_soc/metal2_n.png"
        texture 0 "mitaman/mm-metalbase1.jpg"              // [5] barandillas y tabiques
        texture n "mitaman/mm-metalbase1_n.png"
        texture 0 "ik2k/ik_brick_6464c.jpg"                // [6] ladrillo de fortaleza
        texture n "ik2k/ik_brick_6464c_n.png"
        texture 0 "textures/terrain_soc/rock_grey2.jpg"    // [7] suelo pisable
        texture n "textures/terrain_soc/rock_grey2_n.png"
`;
fs.writeFileSync(path.join(outDir, 'hell.cfg'), cfg);

// ---- resumen y vista cenital (norte arriba) para comprobar la distribución
console.log(`hell: ${ents.length} entidades, ${leaves} cubos hoja (${lava} de lava), ${spires} torres, ${w.n} bytes sin comprimir`);
console.log('vista cenital (~ lava, = metal, % ladrillo, # roca/acantilado, . suelo, espacio = aire):');
for (let y = G - 1; y >= 0; y -= 4) {
    let line = '';
    for (let x = 0; x < G; x += 4) {
        let ch = ' ';
        for (let z = 22; z >= 0; z--) {
            const v = get(x, y, z);
            if (v === V.AIR) continue;
            ch = { [V.LAVA]: '~', [V.METAL]: '=', [V.RAIL]: '=', [V.BRICK]: '%', [V.CLIFF]: '#', [V.ROCK]: '#', [V.FLOOR]: '.' }[v];
            break;
        }
        line += ch;
    }
    console.log(line);
}
