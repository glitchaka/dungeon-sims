# Recursos de la alpha

La referencia visual fue proporcionada por el usuario. geology.png, nature-pieces.png y building-pieces.png se generaron con la herramienta imagegen integrada, siguiendo esa referencia. No son graficos extraidos de C&C ni representan una coincidencia identica con el mapa original.

La geologia excluye los objetos modificables; la mascara de colision se obtiene con tools/import_geography.ps1. pieces.json describe las regiones de los objetos independientes en los atlas; no se modifica el atlas para extraerlos.

Los PNG pequenos de personajes y los recursos heredados proceden del proyecto Godot original y conservan la licencia ASSET-LICENSE.txt. Los datos XML adaptados de C&C tienen su propia licencia, conservada en cnc-license junto al ejecutable y en references/cnc-modding-support/LICENSE.md.

Las imagenes completas usadas como referencia estan archivadas en native/reference; no se cargan como mapa jugable.

## Recursos de esta entrega
Los edificios civicos de civic-buildings.png y el material y relieve contenidos en terrain-islands.pck fueron generados con la herramienta incorporada imagegen. Los prompts exactos estan en CIVIC-GENERATION-PROMPT.txt, terrain-pack/GENERATION-PROMPT.txt y terrain-pack/RELIEF-GENERATION-PROMPT.txt. No proceden del repositorio CnC. civic-pieces.json y el manifiesto del paquete contienen recortes de atlas; los PNG originales conservan su transparencia.

## Terreno cenital v4 (2026-10-22)

Nuevo atlas: terrain-topdown-v4/regions.png, generado con imagegen; prompt íntegro en terrain-topdown-v4/PROMPT.txt. Reemplaza el terreno de ocho materiales y el relieve en tres cuartos en el ejecutable. La muestra se ensambla por código con este atlas, no es una generación independiente. Las construcciones y personajes conservan sus recursos anteriores.
