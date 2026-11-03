# Sombras dinámicas: incompatibilidad entre programa y framebuffer

Base revisada: 719377ba242ecea91fe3524e95211e89e992491b.

## Defecto confirmado en código

`compileglslshader` añadía `layout(num_views = 2)` a todos los vertex shaders
cuando multiview estaba activo, y convertía `modelmatrix` en un array por ojo.
Sin embargo, `shadowmaptexture` hereda `rendertarget`, que adjunta texturas 2D
y renderbuffers de profundidad de una sola vista. Sus buffers de blur también
son 2D. `animmodel.h` ya carga una sola matriz de luz cuando `shadowmapping`.

Un programa con dos vistas no puede dibujar en ese framebuffer de una vista:
la especificación OVR_multiview establece INVALID_OPERATION cuando no coinciden.
Referencia: https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt

Esto puede dejar vacío/inválido el resultado del pase. Sin captura de Quest no
se afirma que sea la única causa del síntoma de sombras que se rompen al mover
la cabeza. La proyección/cobertura dependiente de yaw/pitch es un segundo asunto
que no se altera en este parche.

## Corrección

Se añade la anotación de fuente `//:singleview` para programas cuyo destino es
un framebuffer normal. El compilador Android no les inyecta multiview ni cambia
sus matrices. La anotación está en ambos stages de:

- shadowmapcaster, incluida la plantilla de variantes de animación esquelética;
- shadowmapreceiver (depth peeling);
- blurx/blury (pasadas separables de rendertarget).

Los shaders que leen la sombra al dibujar el mundo siguen siendo multiview:
ambos ojos consultan el mismo mapa generado desde la luz. No confundir
`shadowmapreceiver` del peeling con la variante `<shadowmap>stdworld` de escena.
Se actualizan data/glsl.cfg y su copia dentro de los assets Android.
No cambia la resolución, intensidad, presets ni filtro de modelos proyectores.

## Validación disponible

`python3 tests/test_shadow_shader_views.py`: dos pruebas aprobadas. La primera
extrae y compila las funciones reales de reescritura/ensamblado del compilador,
interceptando las llamadas GL: comprueba stages de una vista, variante animada,
escena multiview y fallback sin multiview. La segunda comprueba las anotaciones
y que las copias de los shaders coincidan. No compila GLSL en Adreno ni ejecuta
el render. No hay NDK/visor conectado en este entorno; APK pendiente.

## Prueba en Quest para Claude/Killy

1. Compilar APK con un versionCode nuevo e instalar; cerrar/reabrir por completo
   para cargar tanto el código nativo como los nuevos assets de shaders.
2. Usar duel7, un objeto/bot y una zona exterior. Mantener el mismo preset.
   Desactivar capturas automáticas para no introducir sus picos.
3. Con `vrmultiview 1` (requiere reiniciar), `shadowmap 1`, probar primero
   `blurshadowmap 0` y después `blurshadowmap 2` para cubrir generación y blur.
4. Girar yaw, mirar arriba/abajo, inclinar roll y desplazarse. Revisar cada ojo
   por separado. Anotar build, glerr/e502, smcast y shadow de la telemetría.
5. Comparar con `shadowmap 0` y con `vrmultiview 0` + reinicio. Los errores GL
   de otras pasadas pueden existir: el contador global por sí solo no localiza
   un fallo en sombras. Usar una captura RenderDoc si sigue aumentando.
6. Si desaparece el error de vistas pero la sombra sigue cortándose al girar,
   investigar `shadowmaptexture::dorender`, `calcshadowmapbb` e
   `isshadowmapcaster`: rotación de cobertura con yaw/pitch, cuantización de
   texeles y coherencia entre proyección y culling. No aumentar la resolución
   para ocultar un error de matrices. Comprobar también el fundido global
   `updateshadowfade` al cruzar techo/exterior.

Criterio: sombras válidas para ambos ojos y con/sin blur, sin nuevos errores
GL en estos pases y con tiempos sostenidos dentro del presupuesto elegido.
