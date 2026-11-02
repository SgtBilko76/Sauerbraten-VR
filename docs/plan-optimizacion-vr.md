# Plan de optimización VR de SauVREngine

## 0. Objetivo rector (fijado por Killy, 2026-11-01)

El motor se compila y se hornea en el PC y se adapta a fondo al hardware de
Quest 3 y de la generación siguiente: esas dos son el objetivo. Se recorta
del código base todo lo que no haga falta (caminos de render de escritorio
antiguos, pasadas duplicadas, formatos sin uso), se actualiza lo que valga la
pena y se busca el máximo de polígonos, efectos y sombras que el visor aguante
a frame estable, pensando desde el principio en mundo abierto y online.

Consecuencias para el plan:

- **Herramientas en el PC primero**: build de escritorio (`tools/desktop/`)
  para hornear PVS, luces y lo que venga; el visor solo ejecuta.
- **Medir en el visor, decidir con cifras**: nada entra sin pasar por el
  panel `vrstats` y la tabla de la sección 6.
- **Renderer**: multiview, foveado, una sola pasada de culling por frame,
  sombras y efectos con presupuesto y LOD, no apagados por defecto.
- **Contenido**: cada mapa con PVS, lightmaps y LOD horneados desde el PC.
- **Arquitectura**: sectores de carga, simulación por distancia y servidor
  autoritativo (sección 4, fase 4) como diseño objetivo, no como añadido.

Estado: borrador vivo, 2026-11-01. Objetivo: que el motor nativo (Cube 2 +
OpenXR) rinda a 72 Hz estables en Quest 3 con margen para crecer hacia niveles
grandes, sectores y cooperativo de 1 a 4. Cada fase tiene una forma de medir;
nada se da por bueno sin cifras del panel `vrstats` en el visor.

## 1. Qué hace hoy el motor en VR (auditoría)

Leído en `android/app/src/main/cpp/vr_glue/` y `src/engine/`:

| Aspecto | Estado | Coste |
|---|---|---|
| Estéreo | `gl_drawframe()` completo **por ojo** (`sauerquest_vr_bootstrap.c`, bucle `for eye`) | Toda la CPU de escena ×2: culling, orden de VAs, modelos, partículas, HUD |
| Multiview | `GL_OVR_multiview2` se detecta pero **no se usa** (swapchain `arraySize = 1`) | Oportunidad grande de CPU, refactor de shaders |
| Agua | `waterreflect 1`, `waterrefract 1`, `maxreflect 2`: cada superficie re-renderiza la escena, por ojo | En duel7 hasta 6 renders de escena por frame |
| Sombras dinámicas | `shadowmap` forzado a 0 en Android (saturaba la GPU) | 0 |
| Glare / bloom | Desactivado en Android (crash en `gle::end()` vía agua) | 0 |
| Oclusión por hardware | `oqfrags`/`oqgeom` apagados en Android (GL_INVALID_ENUM en Adreno) | Se dibuja todo lo que pasa PVS + frustum |
| Resolución | `SS_MULTIPLIER 1.0`, MSAA off (probado 2x/4x: demasiado caro) | Base |
| Foveado fijo | **No existía** | Añadido en este plan (fase 0) |
| Niveles CPU/GPU | `XR_EXT_performance_settings` en BOOST | Ok |
| Refresco | `XR_FB_display_refresh_rate` enumerado, 72 Hz | Ok |
| Métricas | Solo fps del motor; nada de GPU | Añadido en fase 0 |
| Iluminación | Lightmaps horneados + `maxdynlights 3` | Barato |
| PVS del mapa | **duel7 no trae PVS** (`numpvs 0`; en upstream solo metl4 lo tiene) | Sin PVS ni oclusión, una pared no oculta nada: se dibuja todo el frustum |

Medición del usuario (2026-11-01, duel7, foveado 3): 41 fps / 23 ms mirando
al mapa e incluso a una pared; solo mejora en zonas con poca geometría. Encaja
con el coste fijo de dibujar todo el frustum dos veces más el agua.

Hecho en fase 0b: PVS automático en Android (`vrautopvs`, `src/engine/worldio.cpp`):
al cargar un mapa sin PVS se genera (celdas de 32) y se guarda como
`packages/base/<mapa>.pvs` con el crc del mapa; en cargas siguientes se lee.
Pestaña **opciones → vr** con todos los interruptores de prueba (agua, partículas,
luces, shaders, foveado, PVS) para medir desde el visor.

Conclusión: el motor repite trabajo por ojo y no tenía cómo saber si el límite
es CPU o GPU. Antes de tocar el renderer hay que medir.

## 2. Presupuestos iniciales (Quest 3, 72 Hz)

| Recurso | Objetivo | Cómo se lee en `vrstats` |
|---|---|---|
| Frame | 13,9 ms | `fps` y `ms` |
| GPU app | ≤ 10–11 ms | `gpu app` (rojo si ≥ 12,5) |
| CPU app | ≤ 9–10 ms | `cpu app` (rojo si ≥ 12,5) |
| Frames repetidos | 0 sostenidos | `stale` |
| Triángulos visibles | 500–800 k por frame (ambos ojos) | `tris` |
| Draw calls | 200–300 por ojo de partida | `draw` |

Referencia de Meta para Quest 3: 1,3–1,8 M de triángulos y 200–1000 draw calls
según simulación. Son techos, no metas.

## 3. Fases

### Fase 0 · Medir y victorias sin tocar el renderer (hecha en este commit)

- `vrstats` (0/1/2) en el plano del HUD anclado a la cabeza: fps, ms, tris,
  verts, draw calls, lotes, y del runtime: GPU app/compositor, CPU app, uso de
  CPU/GPU, frames repetidos, latencia, foveado, Hz. Fuente:
  `XR_META_performance_metrics` en `vr_glue/VrPerf.c`.
- Foveado fijo `XR_FB_foveation` (bin escalado, GLES): `/vrfoveation 0..3`,
  `/vrfoveationdynamic 0/1`, por defecto alto y dinámico. Slider en opciones →
  HUD. Esperable: 10–25 % menos tiempo de GPU en escenas con shaders de agua y
  partículas; en escenas simples el beneficio puede ser nulo.
- Medición pendiente en el visor (duel7, partida con bots): anotar `gpu app`,
  `cpu app`, `tris`, `draw` con foveado 0 y 3.

### Fase 1 · Recortes de contenido por ojo (bajo riesgo)

Orden por ganancia esperada / esfuerzo:

1. **Agua**: medir con `/waterreflect 0` y con `/reflectsize 7`. Si la GPU
   manda, dejar reflejos en envmap (`waterenvmap`) y mantener refracción.
   Opción intermedia: renderizar la reflexión una sola vez por frame y
   compartirla entre ojos (el error de paralaje en un plano de agua es poco
   visible).
2. **Partículas**: `maxparticledistance` y tamaño en pantalla; las de duel7
   aparecen en las notas del port como causa de picos.
3. **Modelos**: LOD por distancia para jugadores/bots (md5 con `mdllod`?), y
   `maxdynlights` 1–2.
4. **Texturas**: `texreduce` 1 para mapas comunitarios grandes; medir memoria.

### Fase 2 · Compartir trabajo de CPU entre ojos (medio)

- Cull una vez por frame con un frustum que una los dos ojos
  (`visiblecubes`, `findvisiblemms`, orden de VAs) y reutilizar la lista en el
  segundo ojo. Hoy `gl_drawframe` lo recalcula todo por ojo.
- Animación de modelos: calcular huesos una vez por frame (hoy puede
  repetirse por ojo según caché de `animstate`).
- Partículas: actualización una vez, render dos.
- Meta: `cpu app` baja ~30–40 % en escenas con muchos bots.

### Fase 3 · Multiview (alto)

- Swapchain con `arraySize 2` y `GL_OVR_multiview_multisampled_render_to_texture`.
- Shaders: `gl_ViewID_OVR` y matrices por vista en todos los programas de
  `glsl.cfg` (el generador de shaders del motor lo hace factible: un prefijo
  común). Es un cambio transversal; solo tras la fase 2 y con cifras que
  demuestren que seguimos limitados por CPU de envío.

### Fase 4 · Mundo por sectores (diseño)

De la conversación de diseño (mundo abierto, contrato hexagonal, servidor):

- Separar región de diseño (hex), sector de carga y zona de simulación.
- Sauerbraten carga un `.ogz` entero: para sectores hace falta un formato
  propio de "mundo" que una varios mapas o un octree por sector con carga en
  hilo aparte y subida a GPU repartida entre frames.
- Primer prototipo: 3 sectores conectados, un recorrido que obligue a cargar y
  descargar, medir memoria y tirones (`stale` y picos de `cpu app`).
- Servidor dedicado: reutilizar la red original (ENet) para el estado
  autoritativo; el visor nunca espera a la red para cabeza y manos.

## 4. Protocolo de medición

1. Mapa duel7, partida contra 4 bots, 60 s mirando a la zona de agua.
2. Anotar `fps`, `gpu app`, `cpu app`, `stale`, `tris`, `draw` con cada cambio.
3. Un cambio a la vez; se queda si baja el tiempo del recurso limitante sin
   subir el otro por encima del presupuesto.
4. Las cifras se registran en este documento (sección 5).

## 5. Mediciones

| Fecha | Build | Escena | fps | gpu app | cpu app | stale | tris | draw | Notas |
|---|---|---|---|---|---|---|---|---|---|
| 2026-11-01 | f316ee9 | duel7 | "medio" (sin cifras) | — | — | — | — | — | antes de vrstats |
| 2026-11-01 | 3db2294 | duel7, mirando mapa o pared | 41 | — | — | — | — | — | foveado 3; faltan gpu/cpu app |
