# Integracion de habitantes, poblacion y terreno

Actualizado el 2026-10-24. MSPA-Lib se enlaza estaticamente mediante FetchContent al commit `4532daf43a4ffa0a2b7abb8cace29ba53a9b8d89` (v1.0). Las pruebas, benchmark y fuzzing de la dependencia estan desactivados. Esta entrega se compila; no se ejecutan pruebas ni se abre el juego.

## Habitantes

`ResidentActionCatalog` ofrece acciones con destinos alcanzables, recursos y trabajadores disponibles. `ResidentMindSystem` pasa necesidades corporales y efectos esperados a DECIDE. El runtime conserva personalidad, creencias, afecto, valores, metas, memoria, relaciones y aprendizaje. Las acciones fisicas siguen siendo responsabilidad de Isla Viva.

OBSERVE recibe cambios percibidos de necesidades, peligro cercano e influencia divina. REPORT recibe finalizacion o interrupcion y resultados de trabajo, relaciones, encuentros familiares y pesca. ADVANCE_TO procesa el tiempo psicologico en microsegundos enteros de simulacion, separado del reloj de animacion. Los recien nacidos pueden heredar variaciones de personalidad y valores de sus progenitores.

El Domain Pack vive en `IslaVivaDomainPack`, fuera de la biblioteca. La politica anterior se conserva como respaldo cuando no hay seleccion posible.

El guardado v9 incorpora un sobre psicologico versionado con estados binarios MSPA, reloj, identidad, acciones pendientes y RNG. Las copias de World copian snapshots de las mentes y recrean su runtime de forma independiente: una evaluacion provisional de construccion no comparte mentes mutables con el mundo real.

Las visitas y retornos transportan el estado psicologico. La importacion rebasa tiempos, remapea referencias al propio agente y establece la politica RNG de llegada o retorno. El limite de mensaje es 128 KiB y la cola de transporte esta acotada. Los campos del visitante se validan antes de incorporarlo a la poblacion.

## Poblacion

`Population.cpp` gestiona llegadas independientemente de la pesca. Con vivienda libre, provisiones, dinero y acceso al mercado y al agua, una llegada puede ocurrir cada 180 segundos de simulacion. Una entrada ocupada no descarta las otras viviendas ni consume otro periodo completo. Las viviendas para ampliar la poblacion se solicitan segun reservas y habitantes vivos. Se retira el limite anterior de 24, manteniendo el limite del registro de 1000 identidades.

Las reservas de pareja se liberan al interrumpirse, fallecer, dejar de estar disponibles o caducar el desplazamiento al encuentro. Los encargos de fallecidos vuelven a estar disponibles. El calendario biologico usa un dia por segundo de simulacion: nueve meses son 270 segundos (4,5 minutos a 1x). El envejecimiento y el posparto usan esa misma escala, independiente del ciclo solar de 300 segundos. Las partidas sin version biologica se convierten conservando el progreso de sus embarazos y nacimientos elegidos; edades y esperanza de vida ya estan expresadas en dias y se conservan.

## Seleccion y camara

Clic en el cuerpo visible selecciona al habitante; tambien se puede seleccionar al pasajero de una embarcacion. F o SEGUIR centra y sigue al seleccionado, ajustando el zoom si hace falta. Tab o SIGUIENTE recorre habitantes vivos. Doble clic centra. WASD o arrastre central deja de seguir; Enter vuelve a la isla completa. Dentro de una casa se centra la vivienda.

## Terreno fisico por bloques

`TerrainBlockCatalog.cmake` obtiene huellas, esquinas, altura y relieve del manifiesto preparado. `BlockIslandGenerator` usa el esquema geografico como plano de organizacion y ensambla las piezas: construye de nuevo la mascara de tierra/agua, elevaciones, rampas, pendientes y cauces. Las posiciones se guardan en `terrainModules` y el renderer consume esas mismas posiciones e identificadores. Los atlas originales se dibujan directamente, sin reducir previamente su detalle a una textura de doce pixeles por celda.

Las islas nuevas usan este generador. Las partidas existentes conservan geografia y habitantes. Para ver una isla generada por bloques hay que crear una isla nueva desde Opciones de isla. Las juntas y el comportamiento jugable quedan sin comprobacion dentro del juego, conforme a la indicacion de no ejecutar pruebas.
