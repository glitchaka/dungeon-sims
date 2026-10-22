# Isla ensamblada con el set v3

assembled-island.png es una composición de 42 piezas recortadas de los cuatro atlas existentes, sin generación de imágenes adicional. assembled-island.json contiene cada colocación y tools/render_island_blocks.ps1 reproduce el PNG usando los sourceRect del manifiesto.

Reproducir desde la raíz:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/render_island_blocks.ps1

Limitaciones visibles: el set conserva perspectiva de tres cuartos, aunque la última petición requiere vista cenital. Las conexiones lógicas del catálogo no garantizan uniones gráficas: quedan huecos, costas repetidas y paredes internas. Esta muestra permite evaluar esos problemas; no está integrada al juego ni tiene colisiones. No se ha pintado ni generado una imagen nueva para ocultar las uniones.

island-top-down-sample.png es la imagen conceptual anterior generada desde cero; NO procede del ensamblaje. Su prompt se conserva por trazabilidad.

## Ensamblaje cenital v4

assembled-topdown-v4.png sí está compuesto con el nuevo atlas cenital terrain-topdown-v4/regions.png. Su disposición está en terrain-topdown-v4/sample-layout.json y se reproduce con tools/render_topdown_island.ps1. Costa, mar y cauce se calculan por código. game-topdown-v4.bmp y package-topdown-v4.bmp son capturas reales de la alpha compilada y empaquetada, con semilla 1749. El juego compone desde el mundo actual; no usa esta muestra como fondo.

## Isla grande

game-large-island.bmp es una captura real del ejecutable empaquetado con una isla de 256 x 192 celdas y semilla 1749. No es una ampliación de la muestra estática; la simulación tiene cuatro veces más celdas y la cámara admite zoom hasta 800 %.

menu-integrated.bmp: pantalla integrada sin roles de conexion. night-inside-houses.bmp: captura nocturna real; 6/6 habitantes dentro de sus viviendas.
