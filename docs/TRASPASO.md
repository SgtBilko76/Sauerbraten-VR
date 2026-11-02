# Traspaso SauVREngine (2026-11-02)

Estado del fork para que cualquier agente o persona siga desde aquí. Objetivo
rector (ver `plan-optimizacion-vr.md`): compilar y hornear en el PC, adaptar el
motor a fondo al Quest 3 y siguientes, máximo de polígonos, efectos y sombras
con 72 fps estables, mundo abierto online con 30+ jugadores o bots fluidos.

## Cómo se trabaja

```
npm run apk:build        # APK debug (versionCode por build → el visor re-extrae assets)
npm run apk:serve        # http://<IP LAN>:5230 con UN enlace (la última APK); telemetría UDP 5231
npm run desktop:build    # bin64/sauerbraten.exe (llvm-mingw, sin make) para hornear y comparar
npm run desktop:run      # abre el PC con los assets de Android (comparar iluminación)
npm run pvs:bake -- <mapa> --cell 32
```

Todo lo demás está en `tools/apk/README.md` (toolchain, trampas, importar mapas).
Regla del proyecto: tras cada APK nueva, reiniciar `apk:serve`.

## Lo que ya funciona en el visor (verificado con telemetría y capturas)

- Multiview (un pase para los dos ojos), 69-72 fps con 16 bots en duel7, GPU 7-9 ms.
- Color igual que el PC (sin doble gamma), envmaps y minimapa válidos en GLES.
- Agua con reflejo compartido entre ojos; PVS precalculado en el PC y dentro del .ogz.
- Relieve sutil (normal maps generados, fuerza 1.0) y lightmaps rehorneados en el PC.
- Sin sombras del jugador (`vrplayershadow 0`); shadowmap dinámico apagado de verdad.
- Telemetría: `sauvr map=…` cada segundo, `sauvr cfg build=…`, `sauvr boot`, `sauvr crash`,
  capturas automáticas (`dist/captures`), línea de estado en el HUD (build, PC, envíos).
- Mapas empaquetados: duel7 (32k tris) y ruebli (196k tris, horneado a máxima calidad).

## Pendiente, por orden de valor

1. **IA para 30+ bots** (la fase que manda ahora). Con 32 bots el tick sube a 25-60 ms y
   las partículas a 5-13 ms. Plan: repartir `ai::think` entre frames (round-robin, cada
   bot piensa cada N frames según distancia al jugador), bajar la frecuencia de ruta y
   sentidos de bots lejanos, presupuesto de partículas por frame en Android
   (`renderparticles`, ya recortado en distancia, falta un tope duro), y medir con la
   telemetría (`tick=`, `parts=`). Objetivo: 32 bots a 72 fps.
2. **Verificar ruebli en el visor**: fps, tris visibles y aspecto (AO del cielo, relieve).
   Si no llega a 72 fps, bajar `maxparticledistance`, mirar `wvis`/`pvs` y la distancia de
   mapmodels (`maxmodelradiusdistance`).
3. **Foveado con multiview**: XR_FB_foveation puede no aplicarse al swapchain array;
   comprobar con `fovn=` en la telemetría y, si no, probar foveación por capa.
4. **Sondas de envmap**: `env=-1 n=0` en la línea cfg quiere decir que duel7/ruebli no
   tienen entidades envmap; añadirlas en el PC (editor) donde haya metal o agua.
5. **Mundo por sectores y servidor dedicado** (fase 4 del plan): diseño en
   `plan-optimizacion-vr.md`; nada implementado todavía.
6. **Cuerpo completo con IK** para el jugador: cuando exista, `vrplayershadow 1` y revisar
   `renderavatar`/`drawhudmodel` (el arma sigue a la mano derecha, `androidgetaim`).
7. **Más mapas**: candidatos medidos (tris del mundo) fc5 190k, lost 185k, asgard 177k
   (sin skylight: añadirle uno suave para tener AO), fortress 131k, mill 116k. Flujo en
   `tools/apk/README.md`; no hornear a `lightprecision 16` en mapas de 2048 (239 texturas
   de lightmap; a 32 quedan 63).
8. **APK release** firmada (`android/keystore.properties`) y reducción de tamaño
   (la debug pesa 151 MB con dos mapas; los modelos de jugador son 99 MB).

## Trampas que ya costaron tiempo

- GLES rechaza GL_RGB5/RGB10/RGB16 (texturas vacías): `glesinternalformat()` en texture.cpp.
- `gl_checkextensions` fuerza `shadowmap=1` en GL>=3.0; en Android no (y la variable no persiste).
- Los assets solo se re-extraen si cambia versionCode; `data/` y `assets/data/` son copias.
- El Quest cambia de IP al despertar: el servidor contesta `sauvrack` y el visor pasa a unicast.
- Sin `python` en el PATH: scripts en node; los heredocs de bash rompen `\\` en regex JS.
- Mapas sin cfg usan los slots de `default_map_settings.cfg`: `expand-cfg.mjs` los replica en
  orden para poder añadir normales solo a ese mapa.
