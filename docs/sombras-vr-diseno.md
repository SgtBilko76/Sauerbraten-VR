# Sombras reales en VR: diseño desde cero

Rama: `sombras-vr-diseno` (worktree limpio desde `719377b`). Objetivo: una sombra real
que no dependa de la cabeza, no parpadee y no deje cortes en las superficies.
Mientras tanto el visor usa blobs (sombras falsas) en `feat/menu-vr-nativo`.

## 1. Lo que aprendimos (causas probadas)

Cada punto es un síntoma que vimos y la causa encontrada en el código.

| Síntoma | Causa | Dónde |
|---|---|---|
| La sombra gira al mover el casco | El mapa usaba `camera1->yaw/pitch`, que en VR son los de la cabeza | `dorender`, `calcshadowmapbb` |
| Cortes en líneas rectas sobre paredes | Las superficies se marcan como receptoras por bloque de vértices (`va->shadowed`); el borde de la caja divide bloques | `isshadowmapreceiver` |
| Parpadeo de medio mapa | El criterio de receptor dependía del número de casters de cada frame | `isshadowmapreceiver` |
| Caja de un frame anterior | `visiblecubes()` marca receptores antes de que el pase de sombras calcule la caja | `gl_drawframe` |
| Rectángulos sin desenfocar | El desenfoque y el scissor solo trabajan en teselas con casters | `rendertarget.h`, `addblurtiles` |
| Estado roto después del pase | El pase deja el framebuffer y el viewport por defecto, no los de la vista | `rendertarget::render` |
| Superficies con sombra de otros emisores | La unidad de textura 7 era compartida y no se soltaba | `shader.cpp`, `pushshadowmap` |
| Sombra de las paredes sobre sí mismas (acné) | `smdepthpeel` guarda la profundidad del mundo | `rendershadowmapreceivers` |
| Círculo negro persistente, deformado al girar | Pendiente de confirmar: puede ser blob o la caja | — |
| Saltos de la caja de 200 a 900 unidades | Pendiente: probablemente un pase que cambia la cámara | — |

Lo que sí funciona y no hay que tocar: los blobs, el filtro de quién proyecta
(jugadores, bots y objetos), el arma en primera persona fuera del mapa.

## 2. Requisitos

1. **Independiente de la cabeza**: ninguna decisión de sombra depende de yaw o pitch de la cámara.
2. **Un solo marco**: la matriz, las cajas de casters y los tests de receptor usan el mismo
   objeto `ShadowFrame`, calculado una vez por frame antes de cualquier visibilidad.
3. **Estable**: con el jugador quieto y los casters quietos, la sombra no cambia de un frame al
   siguiente (ni una superficie cambia de marca).
4. **Aislado del pase**: el pase de sombras guarda y restaura todo el estado de OpenGL (framebuffer,
   viewport, scissor, máscaras, blend, cull, depth, unidades de textura) y usa su propia unidad.
5. **Compatible con multiview**: se dibuja una vez por frame, antes del pase de los dos ojos.
6. **Sin tiles ni scissor**: el desenfoque se sustituye por PCF (filtrado de vecinos), así no hay
   sectores.
7. **Sin sombra propia del mundo**: solo proyectan jugadores, bots y objetos. Las paredes reciben.
8. **Degradación limpia**: fuera del radio, el visor usa blobs.

## 3. Arquitectura propuesta

### 3.1 ShadowFrame (una vez por frame)

```
struct ShadowFrame {
    vec focus;        // centro en el mundo: posición del jugador + offset del sol, ajustado a rejilla
    vec dir;          // dirección del sol (fija por mapa, sin cambiar con la vista)
    float radius;     // mitad del lado de la caja, en unidades (≈ 5 m, a calibrar)
    float texel;      // 2*radius / resolución: tamaño de un texel en el mundo
    matrix4 matrix;   // mundo -> espacio de sombra
};
```

- `focus.xy` se ajusta a múltiplos de `texel`: al moverse, la caja salta en pasos de un texel,
  nunca de forma continua.
- La caja **no** lee yaw ni pitch. Se centra en el jugador, no en la mirada.
- Se calcula en `gl_drawframe` antes de `visiblecubes()`.
- Todo lo demás lee este objeto: `calcshadowmapbb`, `isshadowmapcaster`, `isshadowmapreceiver`.

### 3.2 Pase de sombras

- Un FBO propio, resolución 512, formato único (decidido al arrancar, sin cambiar a mitad).
- Guardar y restaurar el estado de OpenGL completo (ver requisito 4).
- Casters: solo jugadores, bots y objetos (el filtro actual de `rendermodel`).
- Sin blur: el filtrado se hace en el receptor.
- Sin peel de superficies del mundo.

### 3.3 Receptor (shader)

- Todas las superficies del mundo usan la variante con sombra cuando el mapa está activo.
  No hay marca por bloque de vértices ni criterio por frame.
- Muestreo PCF 3×3 con pesos fijos.
- Sesgo de profundidad fijo más un término según el ángulo entre normal y sol (pendiente).
- Fuera de la caja: sombra 0 (luz completa). Un borde de transición de ~10 % de la caja.
- Oscuridad constante (un parámetro del mapa o del preset), sin fundidos por frame.

### 3.4 Fallback

- Fuera del radio: blobs bajo los actores (como ahora).
- Bajo techo: no hay sombra del sol; la decisión se toma por zona del mapa en la carga, no por un
  rayo por frame (el rayo causaba parpadeos).

## 4. Qué no reutilizar del trabajo anterior

- Los interruptores de diagnóstico son útiles, pero los parches de `shadowmap.cpp`, `shader.cpp`
  y `renderva.cpp` se rehacen desde cero, no se traen.
- El `dorender` de la rama `feat/menu-vr-nativo` es la base de referencia para la matriz, pero
  sin yaw ni pitch.

## 5. Telemetría de aceptación

Campos ya existentes o a reponer en `sauvr map=`:

| Campo | Meta |
|---|---|
| `shflip` | 0 con casters y cámara quietos; < 20 al caminar |
| `shjump` | ≤ 1 texel (0.4 u a 512 px, radio 96) por frame |
| `smerr` | 0 |
| `shadow` (CPU ms) | ≤ 0.6 |
| `gpu` | ≤ 11 ms en equilibrio |
| `p95` | ≤ 16 ms |

Pruebas manuales:
- **Casco quieto, giro de 360°** en el patio: las sombras del suelo no cambian.
- **Casco quieto, bot pasando**: solo cambia la sombra del bot.
- **Suelo plano**: sin cortes rectos ni acné.
- **Bajo techo**: sin sombra del sol, sin parpadeo.

## 6. Hitos

- **M0 (hecho)**: visor en blobs, estable. Referencia para comparar.
- **M1**: `ShadowFrame` único, cálculo antes de la visibilidad, estado del pase aislado.
  Criterio: `shjump` ≤ 1 texel, `smerr` 0.
- **M2**: receptor con todas las superficies en la variante con sombra y PCF.
  Criterio: `shflip` 0 con casters quietos.
- **M3**: formato fijo, sin peel, sesgo. Criterio: suelo plano sin acné al quedarse quieto.
- **M4**: oscuridad constante, fallback a blobs, zona bajo techo por carga.
- **M5**: validación por mapa (patio, interior, mapas grandes y pequeños).

## 7. Preguntas abiertas

1. Radio de la sombra: ¿5 m reales? Hay que medir la escala del motor (altura del jugador en
   unidades) en un mapa de prueba.
2. ¿La oscuridad es la misma en todos los mapas o se configura por mapa?
3. ¿Los bots lejanos deben tener blob o nada?
4. ¿Resolver primero el círculo negro (¿blob o caja?) antes de M1, o integrarlo en M1?
