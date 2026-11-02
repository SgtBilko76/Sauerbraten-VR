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
