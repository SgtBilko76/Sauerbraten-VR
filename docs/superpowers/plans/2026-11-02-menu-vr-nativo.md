# Menú VR nativo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Pantalla `vrhub` para elegir mapa (duel7/ruebli) y configurar bots, habilidad y confort, arrancable al iniciar el visor, con botón al menú clásico.

**Architecture:** Solo cubescript. Una GUI nueva en `data/menus/vrhub.cfg` que reutiliza `GUI__StartBotMatchWithMap` (ya en `data/menus.cfg`) y las variables de `botmatch`. El arranque se engancha en cubescript: la GUI se abre antes del primer frame del bucle de menús, así que `menus.cpp:801` (`showgui("main")` si `guistack` está vacío) no se dispara. Sin cambios en C++, shaders ni render.

**Tech Stack:** cubescript (`newgui`, `guilist`, `guibutton`, `guislider`, `guiimage`), JS de Node para los scripts de build existentes (no se añade código nuevo).

**Spec:** `docs/superpowers/specs/2026-11-02-menu-vr-nativo-design.md`

## Global Constraints

- Ningún cambio en `src/`, `bin64/` ni shaders.
- Solo mapas `duel7` y `ruebli` en la rotación de `VRHub_Maps`.
- Solo modo FFA con bots (`mode 0`, vía `GUI__StartBotMatchWithMap`).
- Rangos: bots 1-32, habilidad 50-101, `vrbright` 0-100, `vrfoveation` 0-3, `vrstatsscale` 1-3 (los mismos que `GUI_Options_VR`).
- Cada cambio en `data/` se replica en `android/app/src/main/assets/data/` (copia manual; ver `tools/apk/README.md`).

## Review Focus

1. Pulsar flechas de mapa con la lista de un solo elemento: el índice debe quedarse en 0 sin error.
2. Pulsar "Jugar" estando ya conectado a un servidor: debe mostrar el aviso de `GUI__StartBotMatchWithMap` y no cargar nada.
3. Habilidad mínima mayor que la máxima (y al revés): el slider contrario debe corregirse, como en `botmatch`.
4. Miniatura de un mapa sin `.jpg` en `packages/base/`: la GUI no debe fallar; se muestra la imagen de respaldo `data/cube.png` (la que ya usan `armyofone` y `privatestansauer`).
5. Arrancar con `vrhub` abierto y luego pulsar ESC (`togglemainmenu`): debe abrirse `main`, no quedar la pantalla vacía.

Cada línea se prueba en la tarea que posee el código (ver Tareas 1 y 3).

---

### Task 1: GUI `vrhub` y sus variables

**Files:**
- Create: `data/menus/vrhub.cfg`
- Test: `tools/menus/check-vrhub.mjs` (estructura) y verificación manual en consola (ver Step 2)

**Interfaces:**
- Consumes: `GUI__StartBotMatchWithMap <mapa>` (`data/menus.cfg`), `GUI_BotCount`, `GUI_MinSkill`, `GUI_MaxSkill` (`data/menus.cfg`, bloque `botmatch`), `vrbright`, `vrfoveation`, `vrstatsscale` (`src/engine/rendergl.cpp`).
- Produces: `VRHub_Maps` (lista de mapas, `duel7 ruebli`), `VRHub_MapIndex` (índice actual, 0 al abrir), `VRHub_Comfort` (0/1, despliega confort), comando `VRHub_Play` (llama a `GUI__StartBotMatchWithMap` con el mapa actual).

- [ ] **Step 1: Escribir las variables y la GUI**

Declarar con `nodebug [ defvar ... ]` las variables nuevas, siguiendo el estilo de `botmatch` en `data/menus.cfg`. `newgui "vrhub"` con:
- miniatura grande (`guiimage` a `packages/base/<mapa>.jpg`, respaldo `data/cube.png`), nombre debajo, flechas que cambian `VRHub_MapIndex` con envoltura;
- columna de partida: texto "Bots FFA", sliders de bots y habilidad con la corrección cruzada de `botmatch`;
- pie: botón grande "JUGAR" (`VRHub_Play`), botón "Confort" (alterna `VRHub_Comfort` y muestra `vrbright`, `vrfoveation`, `vrstatsscale` con sus rangos), botón "Menú clásico" (`showgui "main"`).

Usar `guiscale` alto y separación amplia para el láser del mando.

- [ ] **Step 2: Comprobar que carga sin errores**

Run: `npm run desktop:run`, luego en la consola `showgui "vrhub"`.
Expected: se abre la GUI sin mensajes de error de cubescript; `echo $VRHub_MapIndex` devuelve `0`.

- [ ] **Step 3: Commit**

```bash
git add data/menus/vrhub.cfg
git commit -m "feat(vr): GUI vrhub para elegir mapa y configuración básica"
```

---

### Task 2: Enganchar `vrhub` al arranque y al menú

**Files:**
- Modify: `data/menus.cfg` (añadir `exec "data/menus/vrhub.cfg"` después del bloque `botmatch`, que define `GUI_BotCount`/`GUI_MinSkill`/`GUI_MaxSkill`; y el arranque de la GUI al final de la sección del menú principal)

**Interfaces:**
- Consumes: `vrhub` (Task 1).
- Produces: `togglemainmenu` sigue igual para el menú clásico; `vrhubstart` (comando nuevo) abre `vrhub`.

- [ ] **Step 1: Añadir el `exec` y el comando de arranque**

Insertar `exec "data/menus/vrhub.cfg"` después del bloque `botmatch` de `data/menus.cfg`, y definir `vrhubstart = [showgui "vrhub"]`.

- [ ] **Step 2: Verificar el arranque en PC**

Run: `npm run desktop:run`, con `vrhubstart` ejecutado al cargar (desde `autoexec.cfg` de la copia de trabajo, sin commitear ese cambio).
Expected: al arrancar aparece `vrhub`, no `main`. Pulsar ESC abre `main` (`togglemainmenu`).

- [ ] **Step 3: Commit**

```bash
git add data/menus.cfg
git commit -m "feat(vr): arranque con vrhub y acceso al menú clásico"
```

**Fallback si el arranque sigue mostrando `main`:** el orden entre `showgui` de cubescript y `menus.cpp:801` no está verificado en el visor. Si el PC no lo resuelve, parar y pedir permiso para un cambio de una línea en `src/engine/menus.cpp:801` (contradice el alcance del spec, así que no se hace sin confirmación).

---

### Task 3: Copiar a los assets del visor y verificar en el Quest

**Files:**
- Create: `android/app/src/main/assets/data/menus/vrhub.cfg` (copia de Task 1)
- Modify: `android/app/src/main/assets/data/menus.cfg` (copia de Task 2)

**Interfaces:**
- Consumes: Tasks 1 y 2.
- Produces: APK con la UI nueva.

- [ ] **Step 1: Copiar los dos archivos**

```bash
cp data/menus/vrhub.cfg android/app/src/main/assets/data/menus/vrhub.cfg
cp data/menus.cfg android/app/src/main/assets/data/menus.cfg
```

Expected: `diff` vacío entre `data/` y `android/app/src/main/assets/data/` para esos dos archivos.

- [ ] **Step 2: Construir y servir la APK**

Run: `npm run apk:build` y `npm run apk:serve`. El `versionCode` lo pone `build-apk.mjs`, así que el visor re-extrae los assets.
Expected: APK nueva en `dist/apk/`; `apk:serve` la sirve en `:5230`.

- [ ] **Step 3: Verificar en el Quest**

Instalar la APK, arrancar el visor y revisar la telemetría (`sauvr cfg build=…`).
Expected: `vrhub` aparece al arrancar; recorrer mapas y pulsar JUGAR carga `duel7` y `ruebli` en FFA con bots; `tick=` y `fps` no cambian respecto al menú clásico.

- [ ] **Step 4: Commit**

```bash
git add android/app/src/main/assets/data/menus/vrhub.cfg android/app/src/main/assets/data/menus.cfg
git commit -m "feat(vr): sincroniza vrhub con los assets del visor"
```

---

## Self-review

1. **Cobertura del spec:** GUI y carrusel (Task 1), modo y bots (Task 1), confort (Task 1), JUGAR y menú clásico (Task 1), arranque (Task 2), sincronización (Task 3), pruebas del spec (Task 3). Lo que el spec deja fuera (C++, otros modos, HTML) no aparece en ninguna tarea.
2. **Consistencia de nombres:** `VRHub_Maps`, `VRHub_MapIndex`, `VRHub_Comfort`, `VRHub_Play`, `vrhubstart` se usan igual en todas las tareas.
3. **Review Focus:** los cinco casos tienen dueño (Task 1, excepto el 5 que depende de Task 2).
4. **Proporción:** el plan no contiene el código cubescript completo; describe la GUI y sus variables para que el implementador escriba el cuerpo.
