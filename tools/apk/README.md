# APK de SauVREngine: compilar y bajarla al visor

Se compila con el NDK, la APK cae en `dist/apk/` (ignorado por git) y un
servidor HTTP la expone en la red local para bajarla desde el navegador del Quest.
Todo se ejecuta desde la raíz de este repo con `npm run`.

## Toolchain (todo en `C:/Android`, sin instaladores ni admin)

| Pieza | Ruta |
|---|---|
| JDK 17 (Microsoft OpenJDK zip) | `C:/Android/jdk-17.*` |
| cmdline-tools + `platforms;android-34`, `build-tools;34.0.0`, `platform-tools` | `C:/Android/Sdk` |
| NDK 25.1.8937393 (el que pide `android/app/build.gradle`) | `C:/Android/Sdk/ndk/25.1.8937393` |
| Gradle 8.7 (el repo no trae wrapper; AGP 8.2.1) | `C:/Android/gradle-8.7` |

`build-apk.mjs` exporta `JAVA_HOME`/`ANDROID_HOME` y escribe
`android/local.properties`; no hace falta tocar el entorno de Windows.

## Comandos

```bash
npm run apk:build              # assembleDebug → dist/apk/SauVREngine-debug-<fecha>-<sha>.apk (+ SauVREngine-latest.apk)
npm run apk:build -- --release # necesita android/keystore.properties
npm run apk:serve              # http://localhost:5230 y http://<IP LAN>:5230
```

En el visor: abrir `http://<IP del PC>:5230/`, tocar la APK, y luego
Archivos → Descargas → instalar (modo desarrollador / fuentes desconocidas).
El servidor soporta `Range`, así que el navegador del Quest reanuda si se corta.

## Assets

El repo versiona el subconjunto `android/app/src/main/assets/packages` (~107 MB,
mapa duel7). El pack completo (~1 GB) va en el release de upstream
(`SgtBilko76/Sauerbraten-VR`, `sauerbraten-vr.apk`); una copia está en
`dist/apk/` y se puede extraer de su `assets/` si hace falta todo el contenido.

## Identidad del motor

`src/shared/enginename.h` define `ENGINE_NAME`/`ENGINE_VERSION`. Se usa en la
portada (`renderbackground` en `src/engine/main.cpp`, texto bajo el logo), el
título de la ventana SDL, los errores fatales y el menú `about`
(`data/menus.cfg`, que hay que copiar a `android/app/src/main/assets/data/`).
El nombre de la app Android está en `android/app/src/main/res/values/strings.xml`.

## PVS precalculado

Los mapas de Sauerbraten casi nunca traen PVS y en Android no hay oclusión por
hardware, así que sin él se dibuja todo el frustum. El juego lo genera en segundo
plano al entrar en un mapa sin PVS y lo guarda como `packages/base/<mapa>.pvs`
en su almacenamiento interno. Para que la APK lo lleve ya calculado:

```bash
npm run apk:pull-pvs           # Quest por USB: copia los .pvs del visor a assets/packages/base/
npm run apk:build              # la siguiente APK los incluye y no vuelve a generarlos
```

El fichero lleva el crc del mapa: si el mapa cambia, se descarta y se regenera.

## versionCode y assets

`SauerQuestActivity` solo vuelve a extraer `assets/` al almacenamiento interno
cuando cambia el `versionCode`. Por eso `build-apk.mjs` pasa `-Pvcode` (minutos
desde 2026-01-01) y `-Pvname` (versión + sha) a Gradle en cada build: sin eso,
los cambios en `data/` o `packages/` no llegan al visor al actualizar la APK.

## Build de escritorio y horneado en el PC

`npm run desktop:build` compila el motor para Windows x64 con llvm-mingw
(`C:/Android/llvm-mingw-*`, zip sin instalador) y las SDL2 oficiales para
mingw (`C:/Android/dl/sdl/`), sin make: `tools/desktop/build.mjs` lee la lista
de objetos de `src/Makefile`, compila en paralelo y enlaza estático en
`bin64/sauerbraten.exe`. Vale como editor de mapas y para hornear.

`npm run pvs:bake -- duel7 [--cell 32]` arranca ese exe con los assets de
Android como cwd y el comando `pvsbake`, y deja `duel7.pvs` en
`android/app/src/main/assets/packages/base/`. duel7: 2243 celdas, 82 kB, <1 min.

## Importar otro mapa de Sauerbraten a la APK

La APK original (`dist/apk/sauerbraten-vr.apk`, 1 GB) trae los ~330 mapas y
todos los packages. Para meter uno sin arrastrar el resto:

```
npm run desktop:build                       # una vez: bin64/sauerbraten.exe con mapstats/mapdeps
# 1) dependencias reales (slots usados, mapmodels colocados, skybox): desde la build de escritorio
#    bin64/sauerbraten.exe -q<home> -glog.txt -t0 "-xmap ruebli; sleep 5000 [mapdeps; quit]"  → grep "^dep " log.txt > ruebli.deps
node tools/desktop/import-map.mjs ruebli --deps ruebli.deps   # copia solo eso de la APK a assets/ (ruebli: 220 ficheros, 16 MB)
node tools/desktop/expand-cfg.mjs ruebli                      # mapas sin cfg: slots por defecto expandidos para poder añadir relieve
powershell -File tools/desktop/gen-normals.ps1 -List difusas.txt -Root assets/packages -Strength 1.0
node tools/desktop/add-normals-cfg.mjs assets/packages base/ruebli.cfg
# 2) hornear en el PC: luz (lightprecision 16, lighterror 4, bumperror 2, blurlms 1, blurskylight 1; calclight; savemap) y PVS
npm run pvs:bake -- ruebli --cell 32
# 3) data/lists.cfg: solo los mapas empaquetados (menú); versionCode nuevo → assets re-extraídos en el visor
```

`mapstats` imprime triángulos/vértices del mundo (duel7 32k, ruebli 196k, skycastle-r 384k).

## Servidor dedicado en la LAN (docs/servidor-dedicado.md, paso 1)

```
npm run server:build   # bin64/sauer_server.exe (SERVER_OBJS con -DSTANDALONE + vcpp/mingw.rc; sin él aborta "failed loading icon")
npm run server:lan     # arranca en UDP 28785 (info 28786), home en bin64/server-home; -- --name/--clients/--port
```

En el visor: menú principal → **lan connect** (descubrimiento por broadcast) o
`/connect <IP del PC> 28785`. Comprobado con la build de escritorio
(`-xconnect localhost`). Windows Firewall ya tiene regla para sauer_server.exe
(si la borras, el primer arranque vuelve a pedirla). Los bots que se añadan
desde el visor los piensa el visor hasta el paso 2 (anfitrión de IA headless).

## Clientes simulados (carga de red real contra el servidor)

```
npm run sim:lan -- --n 8 --host 192.168.100.21 --seconds 120 --skill 60
```

Lanza N procesos de `bin64/sauerbraten.exe -H` (sin pantalla: ventana oculta,
sin dibujar ni sonar, 60 ticks/s) con `simplayer 1`: cada uno es un cliente ENet
de verdad cuyo jugador lo mueve la IA de los bots (waypoints, ítems, disparos).
Cada segundo escriben `sim t= map= cn= rtt= loss= tx= rx= pos= …` en
`dist/sim/home-N/log.txt`; el lanzador imprime la tabla cada 5 s y deja
`dist/sim/resumen.json` (rtt medio y máximo, pérdida, kB/s por cliente y total).
Medido en el PC: ~1 kB/s de subida y ~2 kB/s de bajada por cliente, 22 ms de RTT.
Trampa: `ai::navigate` solo asigna nodos si algún jugador tiene `aitype != AI_NONE`,
por eso el simulado se marca como AI_BOT.

## Anfitrión de IA (los bots del servidor se piensan en el PC)

```
npm run server:lan                       # server-init.cfg lleva adminpass y serveraihost 1
npm run aihost:lan -- --bots 16          # headless admin+espectador: el servidor le asigna todos los bots
npm run sim:lan -- --n 4                 # (opcional) clientes de carga
```

Con `serveraihost 1`, si hay un cliente con privilegio de admin, solo los admins
son dueños válidos de bots (`validaiclient` en aiman.h) y al llegar uno se
reasignan los que tuviera cualquier otro cliente. El visor nunca piensa bots.
Medido: 16 bots en el anfitrión cuestan 0,3 ms por frame; cada cliente recibe
~11 kB/s con 16 bots + 4 jugadores. Log: `dist/aihost/log.txt` (`aihost bots= owned= ai_ms=`).
