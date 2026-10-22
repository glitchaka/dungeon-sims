# Terreno cenital v4

Cámara del terreno: cenital ortográfica, mirando verticalmente hacia abajo. Se sustituyen el atlas de ocho materiales y los sprites de relieve en tres cuartos por nueve regiones cenitales. La altura conserva su función en el mundo y la navegación, pero no desplaza los píxeles ni las coordenadas del ratón.

El suelo se reconstruye a partir del estado de la isla, sus accidentes geográficos y su río. No se usa una ilustración de isla como fondo. La costa es una máscara continua suavizada. Las regiones se mezclan con bordes difuminados. Árboles, campos, construcciones y caminos siguen siendo objetos independientes; los bosques pintados del atlas solo se usan en la muestra de composición, no en el suelo del juego.

El formato nativo IVTPACK4 contiene cabecera, metadatos JSON y PNG. La extensión .pck no significa que sea un paquete de Godot. tools/build_topdown_pack.ps1 reconstruye el archivo y comprueba el SHA256 del PNG extraído.

La muestra native/previews/assembled-topdown-v4.png se reproduce con tools/render_topdown_island.ps1. El juego usa el mismo atlas y sus propios datos del mundo, no esa muestra. Los edificios y personajes conservan sus sprites actuales. Esta versión es una alpha; no representa una reproducción exacta de la ilustración original.
