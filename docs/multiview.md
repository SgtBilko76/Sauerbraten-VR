# Multiview en SauVREngine: investigación y diseño

Estado: diseño aprobado para implementar (2026-11-02). Objetivo: dibujar los dos
ojos en una sola pasada de la CPU. Según la telemetría con 12 bots, las fases
que se duplican por ojo suman unos 15 ms por frame (geometría 5, modelos 6,
partículas 1–7, HUD 3); con multiview esa parte se ejecuta una vez y el
coste de CPU de dibujo baja a la mitad. La GPU (7–12 ms, 55–80 %) tiene margen
para absorber el trabajo de vértices duplicado que multiview le pasa.

## 1. Qué dice la extensión

Fuente: especificación `GL_OVR_multiview` / `GL_OVR_multiview2` (Khronos),
cabeceras del NDK 25 (`GLES2/gl2ext.h`, `GLES3/gl32.h`).

- El framebuffer multiview se crea con `glFramebufferTextureMultiviewOVR(target,
  attachment, texture, level, baseViewIndex, numViews)`; **solo acepta texturas
  2D array**. Color y profundidad deben tener el mismo número de vistas.
- El vertex shader declara `#extension GL_OVR_multiview2 : require` y
  `layout(num_views = 2) in;` y lee `gl_ViewID_OVR` (uint). Con `multiview2`
  cualquier salida del vertex shader puede depender de la vista (en
  `multiview` a secas solo `gl_Position`). `gl_ViewID_OVR` también existe en
  el fragment shader.
- **Regla clave**: dibujar con un programa cuyo `num_views` no coincide con
  el del framebuffer da `GL_INVALID_OPERATION`. Es decir, un shader
  multiview no puede dibujar en un framebuffer normal, ni al revés.
- `glReadPixels`, `glBlitFramebuffer` y `glCopyTex*` desde un framebuffer
  multiview dan `GL_INVALID_FRAMEBUFFER_OPERATION`. `glClear` limpia todas
  las vistas. Las consultas de tiempo son indefinidas con multiview.
- Quest 3: GLES 3.2, así que hay `glCopyImageSubData` y texturas array sin
  problema. El glue ya detecta `GL_OVR_multiview2` y
  `GL_OVR_multiview_multisampled_render_to_texture` (`glExtensions.multi_view`).

## 2. Cómo lo hace el runtime de Meta

- Swapchain OpenXR con `arraySize = 2` (una sola swapchain para los dos ojos);
  cada imagen es una `GL_TEXTURE_2D_ARRAY` de dos capas. Las vistas de la
  `XrCompositionLayerProjection` apuntan a la misma swapchain con
  `subImage.imageArrayIndex = 0` y `1`.
- La capa de menú (cilindro/quad) que usa `FrameBuffer[0]` con
  `imageArrayIndex 0` sigue funcionando: lee la capa 0.
- **Riesgo documentado**: en un hilo de Khronos dos desarrolladores reportan
  que el foveado fijo (`XR_FB_foveation`) en GLES funciona con swapchain array
  renderizando capa a capa, pero **no cuando se dibuja con multiview** (la
  densidad queda en 1.0). No hay resolución publicada. Lo aceptamos: hoy el
  límite es la CPU y la GPU tiene margen; se medirá `gpu=` antes y después.
- Los samples de Meta con OpenGL ES usan una swapchain por ojo (sin
  multiview); el glue de este port deriva de ahí.

## 3. Qué toca en el motor (auditoría)

| Pieza | Hoy | Con multiview |
|---|---|---|
| Bucle VR (`sauerquest_vr_bootstrap.c`) | `for eye: set_eye; drawframe` | un `drawframe` con los dos ojos |
| Framebuffer (`TBXR_Common.c`) | 2 swapchains 2D, depth renderbuffer | 1 swapchain array(2), depth `TEXTURE_2D_ARRAY`, `glFramebufferTextureMultiviewOVR` |
| Shaders (`data/glsl.cfg`) | 33× `uniform mat4 camprojmatrix` + `gl_Position = camprojmatrix * vvertex`; 4× `skymatrix`; 3× `hudmatrix` | reescritura automática al compilar: `mat4 X[2]` y `X[gl_ViewID_OVR]` |
| Parámetros globales (`texture.h`) | `GlobalShaderParamState.fval[32]`; `flush()` sube `size` elementos | dos mat4 = 32 floats, `size` = 2 llega solo de `glGetActiveUniform` |
| Cámara (`rendergl.cpp`) | `cammatrix` por ojo con offset IPD; `projmatrix` asimétrica por ojo | `cammatrix` central + `eyeoffset[2]` (traslación IPD en espacio de vista) + `eyeproj[2]`; `camprojmatrix[i] = eyeproj[i] · eyeoffset[i] · cammatrix` en `setcamprojmatrix()` |
| Cull (`visiblecubes`, `setvfcP`) | frustum del ojo actual | frustum central con la unión de los tan de ambos ojos (conservador) |
| HUD anclado (`androidsethudmatrixheadlocked`) | usa `projmatrix` del ojo | `hudmatrix` tiene un modo: 0 2D plano (ambas vistas iguales), 1 anclado (`eyeproj[i]·inv(proj)·hud`), 2 mundo (`camproj[i]·inv(camproj)·hud`) |
| Cielo (`rendersky.cpp`) | `LOCALPARAM(skymatrix, proj·cam')` | dos matrices por ojo |
| Agua (`water.cpp`) | reflejo/refracción en texturas 2D, ya compartidas entre ojos | texturas array de 2 capas renderizadas con multiview (reflejo correcto por ojo gratis); el shader del agua muestrea `sampler2DArray` con `gl_ViewID_OVR` |
| Envmaps (`drawcubemap` + `glCopyTexImage2D`) | renderiza en el framebuffer principal y copia | framebuffer de trabajo array(2) + `glCopyImageSubData` capa 0 → cara del cubo |
| Minimapa (`drawminimap` + `glCopyTexSubImage2D`) | ídem | ídem |
| depthfx / glare / shadowmap / postfx | depthfx activo, resto apagado en Android | depthfx apagado en Android (partículas suaves no compensan); resto ya apagado |
| Model preview (`DRAWTEX_MODELPREVIEW`) | renderiza al framebuffer principal | sin uso en los menús actuales; se desactiva en Android |
| Captura (`glReadPixels` en `screenshot`) | lee el framebuffer | se lee desde un FBO no multiview que adjunta la capa 0 |

Cosas que **no** cambian: todo lo que usa la cámara central en CPU (posición
para luz y niebla, billboards de partículas, picking) sigue igual; el error
de usar el centro en vez de cada ojo es de 3 cm y no se aprecia.

## 4. Decisión de integración

**Un solo programa por shader, con `num_views = 2`, y todos los destinos de
render son arrays de 2 capas.** Alternativa descartada: compilar dos versiones
de cada programa (multiview y normal) y elegir por pase; obliga a duplicar en
`Shader` las cachés de uniformes (`globalparams`, `localparams`,
`uniformlocs`) y a tocar cientos de líneas de `shader.cpp`. Con los destinos
en array, los pases fuera de pantalla cuestan dos capas pequeñas (256² el
agua) y la lógica del motor no cambia.

Orden de implementación (cada paso compila y se puede probar):

1. **Glue**: swapchain array, FBO multiview, `TBXR_prepareEyeBuffer(0)` una vez,
   `imageArrayIndex` por vista, flag `vrmultiview` (fichero en el home que
   escribe el motor; el glue lo lee al arrancar) y protección anticrash: si el
   arranque anterior no llegó a 100 frames, el siguiente arranca sin multiview.
2. **Motor**: `android_sauer_set_eyes()` con los dos offsets y fovs;
   `setcamprojmatrix()` produce `camprojmatrix[2]`; reescritura de shaders en
   `compileglslshader()`; `GlobalShaderParam::set2()`; modos de `hudmatrix`;
   `skymatrix` doble; frustum unión; `gl_drawframe` una vez.
3. **Pases fuera de pantalla**: agua, envmaps, minimapa, captura; depthfx y
   model preview apagados en Android.
4. **Telemetría**: contador de `glGetError` por frame y vista actual; medir
   `geom`, `models`, `parts`, `hud`, `gpu` antes/después con 12 bots.

Fuentes: [OVR_multiview (Khronos)](https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt),
[OVR_multiview2 (WebGL, mismo modelo)](https://registry.khronos.org/webgl/extensions/OVR_multiview2/),
[Meta: swapchains OpenXR](https://developers.meta.com/horizon/documentation/native/android/mobile-openxr-swapchains),
[Meta: multiview](https://developers.meta.com/horizon/documentation/unity/enable-multiview),
[Khronos community: foveado + multiview en Quest/GLES](https://community.khronos.org/t/foveated-rendering-on-quest-with-opengl/108516),
[Magic Leap: swapchain arraySize 2](https://forum.magicleap.cloud/t/multiview-rendering-openxr-opengl-es/2368).
