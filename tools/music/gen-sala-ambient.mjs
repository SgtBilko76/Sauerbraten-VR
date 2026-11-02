#!/usr/bin/env node
// Genera el loop ambiental de la sala VR (propio, sin licencia de terceros).
// Cuatro acordes (Am, F, C, G) en pads de senos con deriva lenta. El loop es
// circular: cada acorde pesa en una ventana con solape y los pesos se normalizan,
// así el final enlaza con el principio sin corte.
//   node tools/music/gen-sala-ambient.mjs [salida.wav]
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const out = process.argv[2] ?? path.join(repo, 'android', 'app', 'src', 'main', 'assets', 'packages', 'sala', 'sala.wav');

const SR = 22050;            // mono 16 bits: suficiente para un fondo, pesa poco
const CHORD_SEC = 12;        // segundos por acorde
const CHORDS = [
    [110.0, 164.8, 220.0, 261.6],   // Am
    [87.3, 130.8, 174.6, 220.0],    // F
    [130.8, 196.0, 261.6, 329.6],   // C
    [98.0, 146.8, 196.0, 246.9],    // G
];
const LOOP_SEC = CHORD_SEC * CHORDS.length;
const N = LOOP_SEC * SR;
// Cada frecuencia completa un número entero de ciclos en el loop: así el final
// y el principio coinciden en fase y no hay clic al repetir.
const whole = (f) => Math.round(f * LOOP_SEC) / LOOP_SEC;
const HALF = (CHORD_SEC / 2) + 3; // ventana de cada acorde: 12 s + 3 s de solape a cada lado
const TAU = Math.PI * 2;

// Distancia circular en segundos, para que el final y el principio se toquen.
const circDist = (a, b) => { const d = Math.abs(a - b) % LOOP_SEC; return Math.min(d, LOOP_SEC - d); };

const samples = new Float64Array(N);
let peak = 0;
for (let i = 0; i < N; i++) {
    const t = i / SR;
    let sum = 0, weight = 0;
    CHORDS.forEach((freqs, c) => {
        const center = c * CHORD_SEC + CHORD_SEC / 2;
        const d = circDist(t, center);
        if (d >= HALF) return;
        const w = Math.cos((Math.PI / 2) * (d / HALF)) ** 2;
        weight += w;
        let s = 0;
        freqs.forEach((f, k) => {
            const lfo = 0.85 + 0.15 * Math.sin(TAU * t / [8, 12, 16, 24][k] + k);  // periodos que dividen el loop
            const f1 = whole(f), f2 = whole(f * 1.003);
            s += lfo * (Math.sin(TAU * f1 * t) + 0.4 * Math.sin(TAU * f2 * t + k) + 0.12 * Math.sin(TAU * whole(f * 2) * t));
        });
        sum += w * s / freqs.length;
    });
    const v = weight > 0 ? sum / weight : 0;
    samples[i] = v;
    peak = Math.max(peak, Math.abs(v));
}
// Normaliza al 60 % del máximo para no saturar en el mezclador del visor.
const pcm = new Int16Array(N);
for (let i = 0; i < N; i++) pcm[i] = Math.round(samples[i] * (peak > 0 ? 0.6 / peak : 0) * 32767);

// Cabecera WAV (RIFF, PCM, 1 canal, 16 bits).
const dataBytes = N * 2;
const buf = Buffer.alloc(44 + dataBytes);
buf.write('RIFF', 0); buf.writeUInt32LE(36 + dataBytes, 4); buf.write('WAVE', 8);
buf.write('fmt ', 12); buf.writeUInt32LE(16, 16); buf.writeUInt16LE(1, 20); buf.writeUInt16LE(1, 22);
buf.writeUInt32LE(SR, 24); buf.writeUInt32LE(SR * 2, 28); buf.writeUInt16LE(2, 32); buf.writeUInt16LE(16, 34);
buf.write('data', 36); buf.writeUInt32LE(dataBytes, 40);
for (let i = 0; i < N; i++) buf.writeInt16LE(pcm[i], 44 + i * 2);

fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, buf);
console.log(`escrito ${path.relative(repo, out)}: ${LOOP_SEC}s, ${SR} Hz mono 16 bits, ${(buf.length / 1048576).toFixed(2)} MB`);
