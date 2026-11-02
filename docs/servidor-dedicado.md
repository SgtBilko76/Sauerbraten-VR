# Servidor dedicado de SauVREngine: IA y online fuera del visor

Diseño (2026-11-02). Objetivo: que el Quest sea un cliente puro que solo
renderiza y manda su posición, y que la IA de los bots, el arbitraje de la
partida y los servicios online corran en un PC o en la nube. Es la base de
los 30+ jugadores o bots fluidos y del mundo abierto por sectores.

## 1. Cómo funciona hoy (lo que condiciona el diseño)

- **Servidor de partida** (`sauer_server`, `src/engine/server.cpp` +
  `src/fpsgame/server.cpp` con `-DSTANDALONE`): ENet/UDP, puerto 28785.
  Arbitra daño, ítems, modos, votos; **no carga el mapa** (sin octree, sin
  físicas) y reenvía posiciones (`N_POS`) entre clientes.
- **Bots**: el servidor los crea como clientes virtuales (`aiman.h`,
  `addai`) y a cada uno le asigna un **dueño** (`ownernum`), el cliente humano
  con menos bots (`findaiclient`). El dueño ejecuta `ai::think` (waypoints,
  sentidos, puntería, físicas con `raycube` contra el octree) y manda las
  posiciones del bot como si fueran suyas (`N_POS` con `N_FROMAI`). Si el dueño
  se va, el servidor reasigna (`reinitai`, `N_INITAI`).
- **Consecuencia**: en el visor, el tick de IA sube a 25-60 ms con 32 bots
  porque el Quest es el dueño de todos. No hay nada que optimizar en el visor
  que iguale a quitarle ese trabajo.

## 2. Arquitectura propuesta

```
 Quest (cliente puro)  ──ENet 28785──►  Servidor de partida  ◄──ENet──  Anfitrión de IA (headless)
   render + N_POS                        sauer_server (C++)               sauerbraten.exe -headless
   recibe N_POS de bots                  arbitraje, estado, modos          dueño de TODOS los bots
         │                                        │  stdin/stdout JSON             ai::think, físicas, waypoints
         │ HTTPS/WebSocket                        ▼
         └──────────────────────►  Servicio Go «sauvr-lobby»
                                   salas, cuentas, emparejamiento, telemetría,
                                   lanza y vigila instancias (partida + IA)
```

### 2.1 Servidor de partida: el C++ que ya existe

Se mantiene `sauer_server` tal cual (protocolo 260, ENet). Cambios mínimos:

- **Preferencia de dueño de IA**: `findaiclient` elige primero a los clientes
  marcados como anfitrión de IA (nuevo `N_AIHOST` o, más simple, privilegio
  `PRIV_ADMIN` + variable `aihostonly 1`), y solo si no hay ninguno reparte
  entre humanos como hoy. Un visor nunca será dueño mientras exista un host.
- **Límite de bots** por instancia (`serverbotlimit`) a 32-48 según mida el host.
- **Canal de control** por stdin/stdout en JSON (líneas): `{"cmd":"map","name":"ruebli"}`,
  `{"cmd":"addbots","n":32,"skill":70}`, `{"cmd":"kick"...}`, y eventos
  `{"ev":"join","cn":3,"name":"..."}`, `{"ev":"stats","fps":...}`. Es lo que
  orquesta Go sin tocar el protocolo del juego.

### 2.2 Anfitrión de IA: el cliente sin pantalla

Un **cliente headless** (`sauerbraten.exe -headless`, misma build de escritorio
`npm run desktop:build`) que carga el mapa de verdad (octree, waypoints `.wpt`,
físicas) y se conecta al servidor de partida como un cliente más, con privilegio
de host. Reutiliza `ai.cpp` íntegro: nada de la IA se reescribe.

Qué toca en el motor:

- Flag `-headless`: sin ventana SDL, sin GL, sin audio; bucle de `updateworld`
  + `ai::update` + `sendpositions` a tick fijo (50-60 Hz) con `sleep`.
  Puntos: `main.cpp` (init), `rendergl.cpp` (no llamar a `gl_drawframe`),
  `sound.cpp` (no init), `texture.cpp` (no cargar texturas: `loadworld` con
  `-headless` salta `preloadusedmapmodels`/lightmaps).
- `ai::update` ya reparte el pensamiento entre los bots que posee; en un PC
  32 bots a 60 Hz son <5 ms. Si hace falta, N bots por frame (round-robin).
- Varios hosts por instancia: el servidor reparte por `bots.length()` entre
  los hosts, así que dos procesos headless = el doble de bots sin tocar nada.

### 2.3 Servicio Go: `sauvr-lobby`

Aquí sí encaja Go (concurrencia barata, un binario, HTTP/WebSocket nativos):

- **API** HTTPS + WebSocket para el visor y el navegador: `POST /rooms`,
  `GET /rooms`, `POST /rooms/{id}/join` → devuelve `host:puerto` de la instancia.
- **Orquestación**: por sala lanza `sauer_server -p<puerto>` + `sauerbraten.exe
  -headless -connect`, los vigila por el canal JSON, los reinicia si caen,
  apaga la instancia cuando la sala se vacía. Puertos de un rango (28785-28799).
- **Cuentas y persistencia**: SQLite al principio (una tabla de jugadores, una
  de partidas, una de métricas); cambiar a Postgres cuando haya más de un nodo.
- **Telemetría**: recibe las líneas `sauvr …` que hoy manda el visor por UDP
  (`tools/apk/serve.mjs`) y las guarda por sala; endpoint `/telemetry`.
- **Sectores (fase 4)**: una instancia por sector del mundo abierto; el lobby
  conoce la malla de sectores y entrega al cliente el `host:puerto` del sector
  destino al cruzar la frontera (traspaso = desconectar y reconectar con el
  estado del jugador en el lobby).

Estructura sugerida: `server/go/` con `cmd/sauvr-lobby`, paquetes
`rooms`, `orchestrator`, `accounts`, `telemetry`; `go.mod` propio. Sin cgo.

## 3. Protocolo y red

- El visor sigue usando ENet (ya compilado en la APK) contra el servidor de
  partida: cero cambios de protocolo para jugar.
- El lobby habla HTTP/WebSocket; el cliente nativo del visor solo necesita un
  `GET` para resolver la sala (puede ir en el menú de servidores actual:
  `connect host puerto`).
- Internet: el servidor de partida necesita UDP abierto (o un VPS). El lobby
  va detrás de TLS. Para empezar, todo en el PC de la LAN como hoy la APK.

## 4. Presupuestos y medidas

| Pieza | Objetivo | Cómo se mide |
|---|---|---|
| Visor | tick < 2 ms sin IA propia; 72 fps con 32 bots | `tick=` en la telemetría |
| Host de IA | 32 bots a 60 Hz, < 8 ms por tick | log del headless (`sauvr ai bots= ms=`) |
| Servidor | 40 clientes, < 50 kB/s por cliente | contadores ENet (`enet_peer` stats) |
| Lobby | 100 salas por nodo | métricas HTTP del propio Go |

## 5. Plan por pasos (cada uno probable en la LAN)

1. **Servidor de partida en Windows**: compilar `sauer_server.exe` con la
   toolchain de escritorio (`tools/desktop/build.mjs` gana un objetivo
   `server`). Conectar el visor por IP desde el menú. Medir con 0 bots.
2. **Cliente headless**: `-headless` en la build de escritorio; conectar como
   host de IA; `findaiclient` con preferencia de host. Añadir 32 bots desde el
   host y medir el `tick=` del visor (debe bajar a < 2 ms).
3. **Canal JSON** en servidor y host; script node que lanza ambos (`npm run
   server:lan`). Documentar en `tools/apk/README.md`.
4. **Lobby en Go**: salas y orquestación en LAN; el menú del visor lista las
   salas del lobby.
5. **Cuentas, telemetría, persistencia** en el lobby; APK release.
6. **Sectores**: una instancia por sector y traspaso.

## 6. Riesgos

- El headless necesita los mismos assets que el visor (mapas, waypoints): se
  sirven desde `android/app/src/main/assets`, como ya hace la build de escritorio.
- Un mapa sin `.wpt` navega solo por los waypoints que van soltando los jugadores
  (`dropwaypoints`): los bots parecen torpes. La APK original trae 312 `.wpt`;
  `import-map.mjs` los copia con el mapa (duel7 y ruebli ya los llevan) y para
  mapas nuevos se generan jugando en el PC y guardando con `savewaypoints`.
- La latencia bot→visor pasa por el servidor (dos saltos en LAN, < 5 ms): no se
  nota; en internet depende del VPS.
- Trampas: un cliente humano que se conecte sin host se convertiría en dueño de
  bots; `aihostonly 1` evita que el visor herede IA.

## 7. Estado (2026-11-02)

- Paso 1 hecho: `npm run server:build` / `npm run server:lan`; el visor jugó en el
  servidor (telemetría `net=192.168.100.21:28785`, 20 ms de RTT).
- Paso 2, mitad hecha: build sin pantalla (`-H`) y **clientes simulados**
  (`npm run sim:lan -- --n 8`): cada uno es un cliente ENet real movido por la IA
  de los bots. Medido con 8 en el PC: RTT 15 ms, 1,2 kB/s de subida y 4,1 kB/s de
  bajada por cliente (el servidor emite 33 kB/s en total), sin pérdida, y se matan
  entre ellos. Falta la otra mitad: el anfitrión de IA (el mismo headless como
  dueño de los bots del servidor, con la preferencia de dueño en `findaiclient`).
- Paso 2 completo: **anfitrión de IA** (`npm run aihost:lan -- --bots 16`): headless
  con admin (setmaster con la adminpass de server-init.cfg) y espectador; el servidor
  (serveraihost 1) solo da bots a admins y reasigna los ajenos. Medido: 16 bots,
  owned=16, 0,3 ms de IA por frame en el PC; los simulados reportan owned=0; cada
  cliente recibe 11 kB/s. Pendiente: probar con el visor dentro (tick= debe quedar < 2 ms con 32 bots).
