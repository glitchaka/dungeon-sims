# Isla ensamblada con el set v3

assembled-island.png es una composición de 42 piezas recortadas de los cuatro atlas existentes, sin generación de imágenes adicional. assembled-island.json contiene cada colocación y tools/render_island_blocks.ps1 reproduce el PNG usando los sourceRect del manifiesto.

Reproducir desde la raíz:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/render_island_blocks.ps1

Limitaciones visibles: el set conserva perspectiva de tres cuartos, aunque la última petición requiere vista cenital. Las conexiones lógicas del catálogo no garantizan uniones gráficas: quedan huecos, costas repetidas y paredes internas. Esta muestra permite evaluar esos problemas; no está integrada al juego ni tiene colisiones. No se ha pintado ni generado una imagen nueva para ocultar las uniones.

island-top-down-sample.png es la imagen conceptual anterior generada desde cero; NO procede del ensamblaje. Su prompt se conserva por trazabilidad.
