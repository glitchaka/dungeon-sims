# Codigo nativo

| Carpeta | Responsabilidad |
| --- | --- |
| src/app | Arranque, lanzador, menu y conexion de la interfaz |
| src/world | Geografia, colocacion de poblados, ecologia y estado del mundo |
| src/simulation | Necesidades, agricultura, familias, pesca, visitas, mazmorras y persistencia |
| src/residents | Registro, decisiones, acciones, movimiento y seleccion de poses |
| src/rendering | Terreno, texturas, sprites, efectos e interpolacion visual |
| src/network | Conexion TCP y mensajes |
| src/powers | Intervenciones divinas y gestos |
| src/audio | Reproduccion de voces |
| src/entities | Tipos de datos compartidos |

El mundo avanza en pasos fijos de 100 ms. El dibujo interpola posiciones y usa su propio reloj; una animacion no aumenta su frecuencia al acelerar la simulacion. Al recorrer una ruta se aprovecha el tiempo restante entre celdas. Si un paso esta bloqueado se detiene la caminata visual.

Los dos fotogramas originales por direccion se mezclan brevemente al pasar de uno a otro, con escala constante y los pies anclados. Esto suaviza las poses disponibles sin inventar una hoja de animacion nueva. La direccion se conserva cuando el habitante se detiene. El terreno modificado se calcula en segundo plano; la carga de la textura SDL se realiza en el hilo del dibujo.

`assets/runtime/` es la fuente unica de recursos del ejecutable y del empaquetado. `assets/archive/` conserva los recursos anteriores sin incluirlos en el juego. [Indice de sprites](assets/README.md).

