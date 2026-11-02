# Menú VR nativo: selección de mapa y configuración básica

Fecha: 2026-11-02. Estado: aprobado.

## 1. Objetivo

Sustituir, dentro del visor, la entrada principal (menú de PC con listas y texto
pequeño) por una pantalla pensada para VR: elegir mapa con miniaturas grandes y
configurar lo básico (modo, bots, nivel de habilidad, confort visual) en pocos
pasos, con botones grandes apuntables con el mando.

Éxito: desde el arranque en el visor, un jugador puede empezar una partida con
un mapa elegido en menos de 4 interacciones, sin salir al menú clásico.

## 2. Alcance

Dentro:

- Una nueva GUI cubescript `vrhub` (en `data/menus/vrhub.cfg`, nuevo archivo,
  enlazado desde `data/menus.cfg`).
- Carrusel de mapas con miniatura (`packages/base/<mapa>.jpg`, mismo recurso que
  `armyofone`/`privatestansauer`) y flechas para cambiar de mapa.
- Selector de modo básico: solo partidas de bots (FFA, la única que el visor ha
  verificado con `GUI__StartBotMatchWithMap`).
- Bots (cantidad 1-32) y habilidad mínima/máxima: mismas variables que
  `botmatch` (`GUI_BotCount`, `GUI_MinSkill`, `GUI_MaxSkill`).
- Confort: brillo extra (`vrbright`), foveado fijo (`vrfoveation`) y tamaño del
  panel de métricas (`vrstatsscale`), con los mismos rangos que `GUI_Options_VR`.
- Botón "Jugar" que llama a `GUI__StartBotMatchWithMap <mapa>` (ya existente en
  `data/menus.cfg`).
- Botón "Menú clásico" que abre `main` tal cual, para no perder nada.

Fuera (no se hace en esta fase):

- Ningún cambio en C++, shaders, render, PVS, IA ni red.
- HTML/JS, WebView o control externo.
- Modos distintos de FFA, campaña, servidores, partidas online y edición.
- Rediseño de `options` o `main` (se conservan).

## 3. Pantallas

Una sola pantalla `vrhub`, con tres zonas:

1. **Mapa (grande, centro):** miniatura a `guiscale` alto, nombre del mapa debajo
   y flechas izquierda/derecha. El índice se guarda en una variable
   `VRHub_MapIndex` sobre la lista `VRHub_Maps` (`duel7 ruebli`, la rotación
   verificada en `maprotation`; ampliable a los mapas empaquetados cuando estén
   verificados).
2. **Partida (columna derecha):** modo fijo "Bots FFA" (texto, sin radio), bots
   con slider 1-32, habilidad mínima y máxima con sliders 50-101 y la misma
   corrección cruzada que `botmatch`.
3. **Pie:** botón grande "JUGAR" (acción principal), "Confort" (despliega
   vrbright, vrfoveation, vrstatsscale) y "Menú clásico".

Todos los botones con `guiscale` grande y separación amplia para apuntar con el
láser del mando.

## 4. Flujo

- Al abrir `vrhub`: el índice de mapa vuelve a 0 si la lista cambió.
- Flecha: cambia `VRHub_MapIndex` con envoltura circular y actualiza la miniatura.
- Jugar: si `ingamemp` ya hay partida, muestra el mismo aviso que
  `GUI__StartBotMatchWithMap` y no hace nada; si no, llama
  `GUI__StartBotMatchWithMap <mapa actual>`.
- Menú clásico: `showgui "main"`.

## 5. Decisiones y supuestos (a confirmar)

- **Dónde aparece:** se muestra al arrancar el visor en lugar de `main`. Supuesto:
  el arranque actual llama a `togglemainmenu`; hay que comprobar la llamada de
  arranque en `android/` antes de cambiarla. Si no es posible, `vrhub` se abre
  desde el botón "menú" del mando.
- **Solo FFA con bots:** no se añaden otros modos. Motivo: el resto de modos y
  el multijugador no son el objetivo de esta pantalla.
- **Mapas:** solo `duel7` y `ruebli` en la rotación. Motivo: ya están verificados
  en el visor (ver `docs/TRASPASO.md`). El resto de mapas empaquetados se añadirá
  cuando se midan.
- **Sin persistencia nueva:** las selecciones viven en variables de sesión.
  Guardar preferencias sería un cambio posterior.

## 6. Archivos que se tocan

- `data/menus/vrhub.cfg` (nuevo): la GUI y sus variables.
- `data/menus.cfg`: `exec` del archivo nuevo y el cambio del menú de arranque.
- `android/app/src/main/assets/data/...`: copia sincronizada con `data/` (los
  assets del visor se copian; los cambios hay que replicarlos y comprobar que se
  re-extraen, ver `TRASPASO.md`).

No se tocan `src/`, `bin64/` ni shaders.

## 7. Pruebas

- Compilación de cfg: arrancar `desktop:run` con los assets y comprobar que no
  hay errores de cubescript en la consola.
- Navegación: abrir `vrhub` en PC y en el visor; recorrer mapas, sliders y
  botones; comprobar que "Jugar" carga `duel7` y `ruebli` en modo FFA con bots.
- Telemetría: `sauvr map=…` y `sauvr cfg build=…` para confirmar mapa y build en
  el visor tras el cambio de assets.
- Rendimiento: `fps` y `tick=` no deben cambiar respecto al menú clásico (la GUI
  no toca render).

## 8. Revisión del propio spec

- Placeholders: no quedan TBD. El punto de arranque está marcado como supuesto a
  comprobar.
- Consistencia: el alcance excluye C++, y los archivos listados no tocan `src/`.
- Ambigüedad: "mapas verificados" se define como `duel7` y `ruebli`.
