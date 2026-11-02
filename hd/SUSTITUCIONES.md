# Texturas HD: sustituciones para Claude

Estas dos texturas IA están preparadas a 512×512 para evaluar en Quest 3.
Esta carpeta es una entrega de assets: por sí sola NO cambia los mapas ni la APK.
La rama principal de este repositorio se llama `master`.

## Qué archivo sustituye a cuál

Todas las rutas son relativas a la raíz del repositorio.

| Archivo nuevo | Textura original que sustituirá | Resolución original | Nueva | Dimensiones lógicas UV que conservar |
|---|---|---|---|---|
| `hd/ik_brick_6464c.jpg` | `android/app/src/main/assets/packages/ik2k/ik_brick_6464c.jpg` | 128×128 | 512×512 | 128×128 |
| `hd/floor_pavement_stone4_3.jpg` | `android/app/src/main/assets/packages/dg/floor_pavement_stone4_3.jpg` | 256×256 | 512×512 | 256×256 |

Ambas aparecen en los CFG empaquetados `packages/base/duel7.cfg` y
`packages/base/ruebli.cfg`. Preservar el orden de slots, rotaciones, offsets y
variantes del OGZ. No confundir slots declarados con superficies visibles.

## Integración propuesta

La implementación experimental y las pruebas están en el
[PR #1](https://github.com/killyvera/SauVREngine/pull/1),
rama `feat/texture-detail-pack`, commit `ab47654055290a6d290b5a93d10139d7a4f734f1`.

Ese PR guarda copias de estos mismos JPEG en
`android/app/src/main/assets/packages/sauvr/detail-v1/` y añade
`<logical:w,h>` al cargador para conservar escala y offsets físicos.
No duplicar los assets de ambas carpetas dentro de la APK: `hd/` es la entrega
para desarrollo, `packages/sauvr/detail-v1/` es el destino de runtime.

No reemplazar ciegamente un JPEG de 128 por uno de 512: las dimensiones de
la textura intervienen en las UV de Sauer. Integrar la preservación de
dimensiones lógicas junto con los nuevos archivos. El PR aún requiere
compilación Android y validación en visor; no está fusionado por esta entrega.

## Mipmaps, materiales y validación

- Reutilizar la cadena de mipmaps existente: 512→256→128→64→32→16→8→4→2→1.
- Los mipmaps residentes no son streaming de texturas.
- JPEG comprime el archivo, no garantiza compresión de GPU. ASTC está pendiente.
- Se conservaron los normal maps originales en el PR; comprobar alineación
  de juntas y relieve, porque la IA puede modificar la estructura.
- Comprobar costuras con repetición 4×4, paredes cercanas y suelo oblicuo.
- Comparar memoria y tiempo GPU en el mismo recorrido, sin capturas automáticas.
- 512 es un candidato de calidad, no un óptimo ya medido. Estimación RGBA8
  con mipmaps: ~2,67 MiB entre ambas, ~2,25 MiB adicionales sobre originales.
- Conservar originales y avisos de sus autores; son derivados de esos materiales.

Generación: herramienta integrada de imágenes, usando las originales como
referencia y solicitando preservar distribución, paleta, juntas y luminosidad,
con poros/desgaste sutiles y bordes repetibles. Reducción final a 512×512 con
ImageMagick, JPEG calidad 92. Los bordes y el aspecto final requieren validación.
