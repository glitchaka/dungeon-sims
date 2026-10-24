# Entidades y decisiones de Isla Viva

`World` conserva el estado del mundo y coordina sus sistemas. La definiciÃ³n del
habitante y la polÃ­tica que escoge su conducta tienen puntos de entrada propios:

- `src/entities/Resident.h`: estado fÃ­sico y del juego de un habitante.
- `src/entities/Task.h`: catÃ¡logo de actividades ejecutables del host.
- `src/entities/WorldEntities.h`: estructuras del resto de las entidades.
- `src/ResidentRegistry.h`: propietario de la poblaciÃ³n. `add` es la Ãºnica alta;
  mantiene las filas de relaciones en nacimientos, llegadas y visitas. La colecciÃ³n
  es append-only: una muerte conserva su Ã­ndice para no romper parentescos ni trabajos.
- `src/ResidentDecision.h`: polÃ­tica actual. `next` ordena intenciones, `evaluate`
  calcula preferencias y `shouldReconsider` determina interrupciones. Recibe datos
  de solo lectura; no conoce `World`, rutas, red, JSON ni el motor grÃ¡fico.
- `src/ResidentContext.cpp`: captura los hechos del host para esa polÃ­tica.
- `src/ResidentController.cpp`: resuelve la intenciÃ³n contra recursos y destinos
  reales. Si una alternativa admite fallback y falla, consulta la siguiente con
  hechos actualizados. Conserva la precedencia de urgencias, Ã³rdenes, familia,
  pesca, trabajo y necesidades de la polÃ­tica anterior.
- `src/ResidentActions.cpp`: inicia y ejecuta actividades, mantiene reservas y
  aplica sus resultados fÃ­sicos. `World::begin`, `finish` y `leaveShelter` delegan
  aquÃ­ para los sistemas de familia, construcciÃ³n y viajes existentes.
- `src/ResidentSimulation.cpp`: necesidades fÃ­sicas, movimiento y scheduling.

El target `isla_resident_model` contiene el modelo, registro y polÃ­tica. Compila
en C++17 sin enlazar RA, `ws2_32` ni nlohmann_json. Usa los parÃ¡metros C&C generados
del host. `isla_sim` agrega contexto, resoluciÃ³n y ejecuciÃ³n. La representaciÃ³n
JSON existente sigue en `Persistence.cpp`; los guardados conservan la versiÃ³n 9.

## Frontera pendiente con MSPA

Esta refactorizaciÃ³n no implementa MSPA ni simula su API candidata. `ResidentIntent`
es una intenciÃ³n de la polÃ­tica anterior: **no equivale a una `ActionOption` MSPA
con factibilidad ya resuelta**. La selecciÃ³n de destinos y algunos fallbacks aÃºn
pertenecen al controlador del host.

El adaptador futuro debe ofrecer opciones fÃ­sicamente disponibles antes de
`DECIDE`, ejecutar la seleccionada y emitir `REPORT` tanto al terminar como al
interrumpir. Debe transformar percepciones en `OBSERVE`, sin exponer todo el estado
de `World` al agente. Los hechos usados por la polÃ­tica antigua no constituyen
automÃ¡ticamente percepciones vÃ¡lidas para MSPA.

MSPA poseerÃ¡ el estado psicolÃ³gico; `ResidentRegistry` seguirÃ¡ poseyendo las
entidades fÃ­sicas. El adaptador y el Domain Pack de Isla Viva pertenecerÃ¡n a este
repositorio. Los Ã­ndices locales existentes necesitan un mapeo a identidad
portable; tambiÃ©n quedan pendientes el reloj psicolÃ³gico entero, continuidad de
acciones, RNG psicolÃ³gico por agente y `EXPORT/IMPORT` validado. El RNG y el reloj
fÃ­sicos actuales se conservan.

## VerificaciÃ³n

`resident_ownership_and_decisions` prueba identidad local, crecimiento de
relaciones, restauraciÃ³n independiente, lÃ­mites de poblaciÃ³n, prioridades,
fallback de huida, necesidades, inspiraciÃ³n y reevaluaciÃ³n. Enlaza Ãºnicamente
`isla_resident_model`. Las pruebas de simulaciÃ³n, familias, islas y multijugador
comprueban los flujos fÃ­sicos y la persistencia que utilizan estos componentes.

ResidentSprites posee exclusivamente la presentacion del habitante: atlas por edad y sexo, direccion, paso y escala. No modifica Resident ni decide acciones. Usa SceneTexture para filtrado y transparencia; la simulacion no depende de los PNG.

ResidentAnimation selecciona poses a partir de un estado de solo lectura, sin SDL ni recursos. ResidentSprites posee los atlas, adapta el estado de barcos y compone la imagen; ResidentEffects dibuja particulas y emotes. Family conserva el cortejo y la gestacion; Fishing conserva la fase de botadura. ActionPreview es una escena de verificacion exclusiva de capturas, separada del juego normal.

PresentationClock mantiene el tiempo visual; PresentationMotion conserva dos posiciones para interpolar residentes y barcos sin modificar el estado ni el guardado. ResidentAnimation recibe por separado el reloj de gestacion y el tiempo visual para evitar fotogramas repetidos por aliasing al acelerar la simulacion.

