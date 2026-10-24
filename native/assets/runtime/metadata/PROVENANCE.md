# Animaciones activas v7

Los atlas residents-work-women-v7.png, residents-work-men-v7.png, residents-pregnancy-v7.png, boats-women-v7.png y boats-men-v7.png y family-cloud-v7.png fueron generados con imagegen integrada el 2026-10-22. Prompts exactos en ANIMATION-V7-PROMPTS.md. Se usan durante acciones reales; las particulas y corazones se dibujan mediante ResidentEffects; la nube usa su atlas detallado. Los menores conservan su atlas separado.

# Recursos activos: geografía v5

La referencia autoritativa es `native/reference/island-user-reference.png`,
aportada por el usuario el 2026-10-22. `menu-island.png` muestra esa referencia
solo en el menú. El mundo jugable usa alturas y objetos de simulación.

Los recursos activos nuevos son `terrain-materials-v5.png`,
`nature-reference-v5.png`, `buildings-reference-v5.png` y
`civic-reference-v5.png`. Se generaron con imagegen integrada utilizando esa
referencia; los prompts exactos y límites están en `REFERENCE-V5-PROMPTS.md`.
Los habitantes usan `residents-women-v6.png`, `residents-men-v6.png` y `residents-children-v6.png`, generados con imagegen integrada. Prompts en `RESIDENT-V6-PROMPTS.md`. `residents.png` se conserva exclusivamente para la fauna heredada, con su licencia original.

Los paquetes v2/v3/v4 y los atlas antiguos descritos debajo son históricos;
el ejecutable y el empaquetador actuales no los cargan ni los distribuyen.

## Historial de recursos anteriores
# Recursos de la alpha

La referencia visual fue proporcionada por el usuario. geology.png, nature-pieces.png y building-pieces.png se generaron con la herramienta imagegen integrada, siguiendo esa referencia. No son graficos extraidos de C&C ni representan una coincidencia identica con el mapa original.

La geologia excluye los objetos modificables; la mascara de colision se obtiene con tools/import_geography.ps1. pieces.json describe las regiones de los objetos independientes en los atlas; no se modifica el atlas para extraerlos.

Los PNG pequenos de personajes y los recursos heredados proceden del proyecto Godot original y conservan la licencia ASSET-LICENSE.txt. Los datos XML adaptados de C&C tienen su propia licencia, conservada en cnc-license junto al ejecutable y en references/cnc-modding-support/LICENSE.md.

Las imagenes completas usadas como referencia estan archivadas en native/reference; no se cargan como mapa jugable.

## Recursos de esta entrega
Los edificios civicos de civic-buildings.png y el material y relieve contenidos en terrain-islands.pck fueron generados con la herramienta incorporada imagegen. Los prompts exactos estan en CIVIC-GENERATION-PROMPT.txt, terrain-pack/GENERATION-PROMPT.txt y terrain-pack/RELIEF-GENERATION-PROMPT.txt. No proceden del repositorio CnC. civic-pieces.json y el manifiesto del paquete contienen recortes de atlas; los PNG originales conservan su transparencia.

## Terreno cenital v4 (2026-10-22)

Nuevo atlas: terrain-topdown-v4/regions.png, generado con imagegen; prompt íntegro en terrain-topdown-v4/PROMPT.txt. Reemplaza el terreno de ocho materiales y el relieve en tres cuartos en el ejecutable. La muestra se ensambla por código con este atlas, no es una generación independiente. Las construcciones y personajes conservan sus recursos anteriores.

menu-island.png reutiliza la muestra compuesta del atlas cenital v4 exclusivamente como ilustracion del menu; no es el suelo del mundo jugable.

## Voces de pareja
ara-ara-girl.mp3, yameteh-kodasai.mp3 y kyaaaaaaaa.mp3 fueron aportados por el usuario desde Downloads el 2026-10-22. Se conservan los MP3 originales sin modificarlos. El repositorio no atribuye una licencia generica a esos audios. FamilyAudio es el adaptador Windows de reproduccion; FamilySoundEvents detecta las transiciones sin depender de audio ni Windows.

## Peces y efecto del poder oculto
sea-fish-v8.png fue generado con imagegen integrada el 2026-10-22. El prompt se conserva en MYSTERY-V8-PROMPTS.md. El titulo Dios Obrando Misteriosamente se renderiza en codigo con tipografia del sistema, arco, degradado y extrusion; no es un texto generado por IA.
