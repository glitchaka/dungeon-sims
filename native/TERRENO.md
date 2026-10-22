# Terreno cenital v4

Cámara del terreno: cenital ortográfica, mirando verticalmente hacia abajo. Se sustituyen el atlas de ocho materiales y los sprites de relieve en tres cuartos por nueve regiones cenitales. La altura conserva su función en el mundo y la navegación, pero no desplaza los píxeles ni las coordenadas del ratón.

El suelo se reconstruye a partir del estado de la isla, sus accidentes geográficos y su río. No se usa una ilustración de isla como fondo. La costa es una máscara continua suavizada. Las regiones se mezclan con bordes difuminados. Árboles, campos, construcciones y caminos siguen siendo objetos independientes; los bosques pintados del atlas solo se usan en la muestra de composición, no en el suelo del juego.

El formato nativo IVTPACK4 contiene cabecera, metadatos JSON y PNG. La extensión .pck no significa que sea un paquete de Godot. tools/build_topdown_pack.ps1 reconstruye el archivo y comprueba el SHA256 del PNG extraído.

La muestra native/previews/assembled-topdown-v4.png se reproduce con tools/render_topdown_island.ps1. El juego usa el mismo atlas y sus propios datos del mundo, no esa muestra. Los edificios y personajes conservan sus sprites actuales. Esta versión es una alpha; no representa una reproducción exacta de la ilustración original.

## Tamaño de mundo

A partir de la alpha de islas grandes, las islas nuevas ocupan una cuadrícula de 256 x 192 celdas. Sus masas de terreno y accidentes se generan a esa escala; no se trata de ampliar una imagen de fondo. Los objetos mantienen su tamaño por celda. El render del suelo tiene 3072 x 2304 píxeles y la cámara alcanza 800 %.
