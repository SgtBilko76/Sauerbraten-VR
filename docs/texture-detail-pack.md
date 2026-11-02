# Paquete experimental de texturas IA v1

Dos materiales usados en los CFG de duel7 y ruebli. No es una restauración de
la biblioteca completa ni una afirmación de resolución óptima medida en Quest.
Base de implementación: a79c1ad; conserva los originales y sus avisos/licencias.
Las imágenes derivadas no se deben tratar como contenido con licencia nueva.

## Contenido e integración

| Original | Original px | Nuevo px | UV lógicos |
|---|---:|---:|---:|
| ik2k/ik_brick_6464c.jpg | 128×128 | 512×512 | 128×128 |
| dg/floor_pavement_stone4_3.jpg | 256×256 | 512×512 | 256×256 |

Generadas con la herramienta integrada de imágenes a partir de las texturas
originales, después reducidas con ImageMagick a 512×512 JPEG calidad 92.
Prompt de piedra: restaurar cuatro bloques 2×2, conservar juntas, musgo,
colores y luminosidad; añadir poros sutiles; albedo ortográfico repetible.
Prompt de pavimento: conservar filas alternadas, proporciones y juntas;
añadir desgaste y poros; albedo ortográfico repetible sin sombras direccionales.
Los prompts solicitaron bordes repetibles; esto NO garantiza ausencia de
costuras. Revisar un plano repetido y paredes a escala real dentro del visor.
Los normal maps anteriores se conservan: revisar alineación del relieve,
porque la generación IA puede modificar juntas aunque se le pida preservarlas.

El prefijo `<logical:w,h>` conserva `Texture::xs/ys` para las UV del mundo,
independientemente de `w/h` de la imagen subida a GPU. Se aplica después de
subirla, para no confundir dimensiones del buffer con dimensiones del material.
Así se conservan escalas, rotaciones y offsets de los vslots del mapa sin
reescribir el OGZ. El prefijo forma parte de la clave de caché de texturas.
Se mantienen el cargador y la generación de mipmaps del motor: 512, 256, 128,
64, 32, 16, 8, 4, 2 y 1. La GPU selecciona el mip por huella en pantalla.
No son diez archivos adicionales ni streaming de mips: la cadena es residente.

## Comparación reversible

```sh
node tools/textures/detail-pack.mjs original
node tools/textures/detail-pack.mjs enhanced
npm run apk:build
```

Elegir el perfil ANTES de compilar. Recargar el mapa en escritorio; en Quest
instalar una APK con versionCode nuevo para reextraer los assets. La herramienta
edita únicamente las referencias difusas de los dos CFG empaquetados; conserva
el orden de slots y parámetros. La build de escritorio usa estos mismos assets.
No ejecutar el perfil enhanced con un binario antiguo: ignoraría el significado
lógico y cambiaría la escala física. No se modifica master automáticamente.

## Presupuesto y validación

Estimación conservadora RGBA8 + mipmaps: cada textura 512 ocupa ~1,33 MiB;
las dos ~2,67 MiB frente a ~0,42 MiB para las originales (~2,25 MiB extra).
Es una estimación, no una medida: el driver y el formato interno pueden variar.
JPEG reduce el paquete, NO garantiza compresión residente en GPU. Esta entrega
no añade un cargador ASTC, ni afirma aprovechar ASTC ya. La siguiente etapa
requiere un contenedor/cargador con todos los mips y selección por capacidades.
No se añade una muestra de textura al shader ni una pasada de render.

Pruebas de host:

```sh
node --test tests/detail-pack.test.mjs
g++ -std=c++11 -Wall -Wextra -Werror tests/texturelogical.cpp -o texturelogical-test
./texturelogical-test
```

Prueban transformaciones reversibles, conservación de argumentos/normales,
idempotencia y parser de dimensiones (incluidas entradas inválidas).
No sustituyen compilación NDK ni validación OpenGL/Quest; pendientes.

Para aprobar: misma build/recorrido/cámara con original y enhanced en cada
mapa; capturas automáticas apagadas (`vrautocapture 0`); medir memoria, GPU,
frames perdidos y percentiles de tiempo durante sesión prolongada. Revisar
pared cercana, suelo oblicuo, repetición 4×4 y normal maps. Si no aporta calidad
visible o aumenta demasiado el coste, revertir o preparar variante 256.

Antes de ampliar el paquete: usar `mapdeps` del motor para distinguir slots
realmente usados de los muchos slots definidos pero no visibles en los CFG.
Priorizar por superficie visible y distancia, no por escalar toda la biblioteca.
