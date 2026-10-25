# Estado para continuar desde otro PC

Este commit conserva trabajo en curso; no representa una entrega visual terminada.

- Caminata pendiente: crear e integrar 12 fotogramas distintos que completen ambos pasos, alternando piernas y brazos. Los atlas v9 siguen repitiendo media zancada. No sustituirlo por deformaciones de una imagen.
- Se retiro del codigo la deformacion ResidentWalkRig y la capa TerrainBlockView que estiraba fragmentos de ilustraciones sobre el mapa. Se restauro la carga de terrain-materials-v5.png y se conserva el campo fisico de alturas.
- IslandGeography vuelve a conservar sus alturas: ya no se sobrescriben con colinas independientes por bloque. Sin embargo, su distribucion regional sigue siendo fija: falta generar geografias distintas por semilla, no solo detalles distintos.
- menu-island.png es la referencia de calidad, detalle, relieve y estilo, no una disposicion geografica que deba copiarse. Cada partida nueva debe crear una isla diferente; cargar una partida debe conservarla.
- Los bloques preparados se conservan en assets/terrain-blocks-v3. Su composicion visual correcta sigue pendiente; retirar la capa defectuosa no completa esa integracion.
- Se mantienen los cambios de navegacion por pendiente, calendario biologico y embarazo de 270 segundos de simulacion, poblacion e integracion MSPA. Requieren comprobacion de comportamiento dentro del juego.
- MSPA se fija al commit 4532daf43a4ffa0a2b7abb8cace29ba53a9b8d89. En otro PC debe obtenerse mediante FetchContent; no copiar la cache de CMake ni su ruta local FETCHCONTENT_SOURCE_DIR_MSPA.
- No se ejecutaron pruebas ni se abrio el juego durante las ultimas correcciones, por instruccion del usuario. No afirmar validacion visual ni conformidad completa de MSPA.
- Las partidas existentes conservan su terreno guardado; los cambios de generacion se aplican a islas nuevas.

Las descripciones anteriores en TERRENO.md y PROVENANCE.md sobre TerrainBlockView/ResidentWalkRig quedan superadas por este estado.
