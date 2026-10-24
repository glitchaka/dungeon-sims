# Set cenital v4

regions.png es un atlas nuevo de nueve regiones visto desde arriba. PROMPT.txt contiene la instrucción exacta usada para generarlo. El primer candidato no cumplía la perspectiva y no se incorporó.

La muestra ensamblada usa regiones de este archivo en tamaños distintos: sample-layout.json declara cada colocación. tools/TopDownIsland.cs realiza la composición determinista; el mar, la máscara costera y el cauce se calculan por código. La imagen completa no se generó con IA.

Ejecutar desde la raíz:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_topdown_pack.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/render_topdown_island.ps1

El juego carga terrain-topdown-v4.pck y compone el suelo desde World. No incorpora las celdas de bosque al suelo: sus árboles siguen siendo entidades modificables. La muestra visual sí usa las celdas de bosque. El formato IVTPACK4 es propio del programa C++, no es Godot PCK.
