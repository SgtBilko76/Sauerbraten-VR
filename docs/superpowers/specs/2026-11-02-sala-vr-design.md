# Sala VR previa a la partida

Fecha: 2026-11-02. Estado: borrador pendiente de revisión.
Relación: sustituye al arranque de `vrhub` (ver `2026-11-02-menu-vr-nativo-design.md`).
La GUI `vrhub` (tarea 1 del plan, commit `6d0db6a` tras el rebase) se reutiliza dentro de la sala.

## 1. Objetivo

Al arrancar el visor, el jugador está en una sala: suelo de rejilla, luz cenital,
música de fondo y botones flotantes frente a él. Desde ahí elige mapa y lo básico
de una partida de bots. Pulsar el botón del mando abre o cierra el panel de botones.

Éxito: arranque → sala con música y panel abierto en menos de 5 s; elegir mapa y
JUGAR carga la partida sin salir de la sala previa.

## 2. Principio: sin render nuevo

Respeta la regla de no tocar el render del motor. La sala se construye con lo que
el motor ya tiene:

| Elemento | Cómo se consigue | Qué toca |
|---|---|---|
| Suelo de rejilla | Geometría del mapa de la sala (cubos con textura de rejilla) | Solo datos del mapa |
| Luz cenital | Valor de luz del mapa (skylight) o luz fija del mapa | Solo datos del mapa |
| Música de fondo | Comando `music "<archivo>"` (`src/engine/sound.cpp`) | Ninguno; ya existe |
| Botones flotantes | GUI 3D del motor (`src/engine/3dgui.cpp`, modo `gui2d` apagado) | Solo cubescript |
| Abrir/cerrar con el botón | Enlace del mando a `togglemainmenu` (ya existe en `defaults.cfg`) | Solo cubescript |

No hay shaders, ni pases de render, ni cambios en `src/engine/rendergl.cpp`.

## 3. Pantallas y flujo

1. Arranque: se carga el mapa `sala` (nuevo, editado en el PC).
2. La música empieza en bucle al entrar.
3. El panel de botones aparece frente al jugador (distancia y altura fijas).
4. Botones: "Elegir mapa" (carrusel de `vrhub`: duel7, ruebli), bots y habilidad
   (sliders de `GUI_BotCount`, `GUI_MinSkill`, `GUI_MaxSkill`), "Jugar" (llama a
   `GUI__StartBotMatchWithMap`), y "Menú clásico" (`showgui "main"`).
5. El botón del mando alterna el panel (abrir/cerrar).

## 4. Decisiones y supuestos

- **La sala es un mapa aparte** (`sala`), no una parte de duel7. Motivo: se edita
  una vez en el PC y se distribuye como cualquier mapa empaquetado.
- **Música:** loop ambiental propio, generado con `tools/music/gen-sala-ambient.mjs`
  (48 s, 22050 Hz mono 16 bits WAV, en `packages/sala/sala.wav`). Sin licencia de
  terceros. Las pistas CC0 de OpenGameArt quedan como alternativa, pero cada
  archivo necesita revisar su licencia. El WAV queda fuera de git
  (`.gitignore` excluye `android/app/src/main/assets/packages/`).
- **Panel de botones:** la GUI 3D se coloca con la distancia que ya usa `vrhub` y
  `guipushdist`. Supuesto a comprobar: la GUI 3D se queda fija frente al jugador y
  no se cierra sola por distancia (`menuautoclose`, `3dgui.cpp`).
- **Luz cenital:** se prueba primero con skylight del mapa; si no se ve bien en el
  visor, una luz fija del mapa. Ninguna de las dos cambia el render.
- **Sin persistencia:** como en `vrhub`, las selecciones viven en sesión.

## 5. Pruebas

- Estructura: `tools/menus/check-vrhub.mjs` se amplía para comprobar que el mapa
  `sala` y la llamada a `music` existen.
- Engine (PC): `npm run desktop:run` abre la sala, suena la música, el panel aparece
  y el botón del mando lo alterna (simulado con la tecla de `togglemainmenu`).
- Visor: arranque en la sala, FPS sin cambio respecto a `vrhub` solo, y telemetría
  `sauvr map=sala`.

## 6. Riesgos

- Un mapa nuevo debe construirse en el editor del PC (no hay editor en el visor).
- La GUI 3D en VR puede verse borrosa a distancia o taparse con la geometría;
  se ajusta distancia y altura en el visor.
- Rendimiento de un mapa extra con rejilla: se mide con la telemetría.

## 7. Preguntas abiertas

- **Música:** resuelta con un loop propio (ver §4). Si prefieres una pista externa,
  hay que indicar cuál y revisar su licencia.
- **Tamaño de la sala:** ¿pequeña (un solo espacio) o amplia (con pasillo al mapa)?
