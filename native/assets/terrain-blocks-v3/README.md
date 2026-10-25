# Bloques modulares de terreno v3

24 piezas en cuatro atlas con fondo transparente. Es un set nuevo de formas intercambiables; no usa el atlas rechazado de ocho texturas planas ni una sola imagen de isla repetida.

- Costa: cuatro orientaciones rectas y ocho esquinas convexas/concavas, con cabos y bahias.
- Interiores: pieza pequena, alargadas en dos ejes, forma L, cerro y meseta con rampa.
- Relieve/agua: macizo grande, cerro mediano, monte pequeno, rio, curva y garganta con cascada.

La camara y la luz mantienen una orientacion comun. No se deben girar los PNG arbitrariamente: las caras visibles y las sombras cambiarian de orientacion. Para las costas se usan variantes direccionales ya dibujadas.

manifest.json define la huella de cada pieza en celdas logicas, altura relativa, puertos y esquinas de tierra/agua. El macizo ocupa 12 celdas, el cerro 4 y el monte 1. Las rampas son suelo transitable que conecta alturas, no adornos de costa.

Los ejemplos de layouts ofrecen cuatro patrones: isla redondeada, alargada, media luna y archipielago. tools/compose_island_blocks.ps1 combina piezas de distintos tamanos, comprueba cobertura y compatibilidad logica de las costas. Los layouts son datos de composicion, no capturas de una isla jugable ni una afirmacion de que las juntas graficas esten ya resueltas.

El renderer carga ahora este manifiesto y los cuatro atlas mediante loadTerrainBlocks. TerrainBlocks compone las piezas segun costas, alturas y agua del mundo: respeta sourceRect, huellas y orientaciones originales, sin rotar ni reflejar los PNG. Las coordenadas del catalogo se escalan a modulos de 12 celdas del mundo; las esquinas diagonales se subdividen en modulos menores. El campo fisico de alturas y las partidas existentes se conservan. Las juntas visuales y la coincidencia exacta entre la superficie pintada y la colision requieren revision dentro del juego; esta integracion no declara esas juntas aprobadas.

Arte generado con la herramienta incorporada imagegen, usando la isla ilustrada solo como referencia de estilo. Prompts exactos en los archivos *-PROMPT.txt. Los originales PNG se conservan sin transformaciones.
