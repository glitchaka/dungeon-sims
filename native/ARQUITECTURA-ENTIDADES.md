# Entidades y decisiones de Isla Viva

`World` conserva el estado del mundo y coordina sus sistemas. La definición del
habitante y la política que escoge su conducta tienen puntos de entrada propios:

- `src/entities/Resident.h`: estado físico y del juego de un habitante.
- `src/entities/Task.h`: catálogo de actividades ejecutables del host.
- `src/entities/WorldEntities.h`: estructuras del resto de las entidades.
- `src/ResidentRegistry.h`: propietario de la población. `add` es la única alta;
  mantiene las filas de relaciones en nacimientos, llegadas y visitas. La colección
  es append-only: una muerte conserva su índice para no romper parentescos ni trabajos.
- `src/ResidentDecision.h`: política actual. `next` ordena intenciones, `evaluate`
  calcula preferencias y `shouldReconsider` determina interrupciones. Recibe datos
  de solo lectura; no conoce `World`, rutas, red, JSON ni el motor gráfico.
- `src/ResidentContext.cpp`: captura los hechos del host para esa política.
- `src/ResidentController.cpp`: resuelve la intención contra recursos y destinos
  reales. Si una alternativa admite fallback y falla, consulta la siguiente con
  hechos actualizados. Conserva la precedencia de urgencias, órdenes, familia,
  pesca, trabajo y necesidades de la política anterior.
- `src/ResidentActions.cpp`: inicia y ejecuta actividades, mantiene reservas y
  aplica sus resultados físicos. `World::begin`, `finish` y `leaveShelter` delegan
  aquí para los sistemas de familia, construcción y viajes existentes.
- `src/ResidentSimulation.cpp`: necesidades físicas, movimiento y scheduling.

El target `isla_resident_model` contiene el modelo, registro y política. Compila
en C++17 sin enlazar RA, `ws2_32` ni nlohmann_json. Usa los parámetros C&C generados
del host. `isla_sim` agrega contexto, resolución y ejecución. La representación
JSON existente sigue en `Persistence.cpp`; los guardados conservan la versión 9.

## Frontera pendiente con MSPA

Esta refactorización no implementa MSPA ni simula su API candidata. `ResidentIntent`
es una intención de la política anterior: **no equivale a una `ActionOption` MSPA
con factibilidad ya resuelta**. La selección de destinos y algunos fallbacks aún
pertenecen al controlador del host.

El adaptador futuro debe ofrecer opciones físicamente disponibles antes de
`DECIDE`, ejecutar la seleccionada y emitir `REPORT` tanto al terminar como al
interrumpir. Debe transformar percepciones en `OBSERVE`, sin exponer todo el estado
de `World` al agente. Los hechos usados por la política antigua no constituyen
automáticamente percepciones válidas para MSPA.

MSPA poseerá el estado psicológico; `ResidentRegistry` seguirá poseyendo las
entidades físicas. El adaptador y el Domain Pack de Isla Viva pertenecerán a este
repositorio. Los índices locales existentes necesitan un mapeo a identidad
portable; también quedan pendientes el reloj psicológico entero, continuidad de
acciones, RNG psicológico por agente y `EXPORT/IMPORT` validado. El RNG y el reloj
físicos actuales se conservan.

## Verificación

`resident_ownership_and_decisions` prueba identidad local, crecimiento de
relaciones, restauración independiente, límites de población, prioridades,
fallback de huida, necesidades, inspiración y reevaluación. Enlaza únicamente
`isla_resident_model`. Las pruebas de simulación, familias, islas y multijugador
comprueban los flujos físicos y la persistencia que utilizan estos componentes.

ResidentSprites posee exclusivamente la presentacion del habitante: atlas por edad y sexo, direccion, paso y escala. No modifica Resident ni decide acciones. Usa SceneTexture para filtrado y transparencia; la simulacion no depende de los PNG.

ResidentAnimation selecciona poses a partir de un estado de solo lectura, sin SDL ni recursos. ResidentSprites posee los atlas, adapta el estado de barcos y compone la imagen; ResidentEffects dibuja particulas y emotes. Family conserva el cortejo y la gestacion; Fishing conserva la fase de botadura. ActionPreview es una escena de verificacion exclusiva de capturas, separada del juego normal.

PresentationClock mantiene el tiempo visual; PresentationMotion conserva dos posiciones para interpolar residentes y barcos sin modificar el estado ni el guardado. ResidentAnimation recibe por separado el reloj de gestacion y el tiempo visual para evitar fotogramas repetidos por aliasing al acelerar la simulacion.
