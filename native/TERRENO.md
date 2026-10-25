# Terreno modular v3 sobre la geografia fisica

Actualizacion 2026-10-25: se retiro TerrainBlockView por deformar las texturas y se desactivo la sobrescritura de IslandGeography por BlockIslandGenerator. La integracion visual de bloques y la variacion geografica completa por semilla siguen pendientes. Las descripciones siguientes del render modular son historicas. Estado vigente: PENDIENTES-2026-10-25.md.

Correccion de superficies: las piezas completas con paredes pintadas ya no se dibujan como tarjetas superpuestas. TerrainBlockView construye una malla continua sobre el campo fisico de alturas y toma solo superficies interiores de los atlas. Las uniones interiores no reciben las faldas de roca o agua del dibujo original. Las zonas de roca se determinan por la pendiente real; TerrainSurface comparte el limite de pendiente con la navegacion. Los personajes guardados en superficies ahora prohibidas se recuperan sobre suelo libre cercano, conservando identidad e inventario.

Las islas nuevas se generan fisicamente con `BlockIslandGenerator`: el manifiesto determina piezas, huellas, costas, relieve y cauces. `terrainModules` conserva la composicion en el guardado y se reutiliza directamente al dibujar. El esquema geografico previo organiza las regiones, pero sus alturas y mascara se reconstruyen con los modulos. Las partidas existentes conservan su mapa; para usar la generacion nueva hay que crear una isla nueva. Detalles en INTEGRACION-MSPA.md.

El ejecutable carga `assets/terrain-blocks-v3/manifest.json` y sus cuatro atlas originales. `TerrainBlocks` selecciona costas por mascara de esquinas y piezas interiores, relieve y cauces por huella y estado del mundo. No gira los PNG ni utiliza `terrain-materials-v5.png` como fuente de arte del terreno. Se mantiene el campo de alturas y la proyeccion compartida con habitantes y seleccion. Las juntas y la correspondencia grafica exacta con la colision quedan pendientes de revision visual en el juego; no se ejecutaron pruebas para esta entrega.

## Geografia fisica v5 conservada

La referencia vigente es `native/reference/island-user-reference.png`, aportada
por el usuario el 2026-10-22. El generador anterior y el render cenital v4 fueron
retirados del ejecutable. Los archivos y muestras v4 son históricos.

## Mundo y cámara

La isla usa 256 x 192 celdas con un campo de elevaciones real. `IslandGeography`
organiza una cordillera quebrada al noreste, mesetas irregulares con barrancos,
una cuenca occidental, cabeceras lacustres, dos drenajes, bahías e islotes. La
semilla cambia el detalle de costa, erosión y vegetación manteniendo esa dirección
geográfica. La organización toma la imagen como referencia; no es una
reconstrucción topográfica exacta de cada accidente.

`TerrainRenderer` aplica materiales a esas alturas y calcula pendientes,
iluminación y sombras. `TerrainProjection` proyecta altura hacia la pantalla,
resuelve la superficie visible y convierte el ratón de vuelta a coordenadas del
mundo. El render y la selección usan la misma proyección oblicua ortográfica.
La navegación y la construcción siguen usando las alturas físicas.

Los árboles, viviendas, campos, cristales, embarcaciones y edificios cívicos son
objetos independientes. El suelo no es una ilustración completa con un poblado
pintado. `SettlementLayout` distribuye habitantes y casas mediante candidatos
mezclados por semilla, separación entre viviendas y validación de acceso al agua.

## Recursos activos

- `terrain-blocks-v3/`: costas direccionales, interiores de distintas huellas, relieve y cauces con sus recortes originales.
- `nature-reference-v5.png`: árboles, cultivos y rocas independientes.
- `buildings-reference-v5.png`: viviendas, almacén, mercado y piezas portuarias.
- `civic-reference-v5.png`: edificios cívicos y afloramiento de cristales.

Se generaron con imagegen integrada, usando la imagen del usuario directamente.
Los prompts exactos están en `assets/REFERENCE-V5-PROMPTS.md`. `SceneTexture`
extrae los recortes por componentes alfa y utiliza alfa premultiplicado, mipmaps
y filtrado lineal para el detalle de paisaje. Los personajes utilizan los atlas
v9 de seis fases por accion. El menu muestra la imagen de referencia.

## Abrir y verificar

`Jugar-Alpha.bat` abre el perfil `geografia-v5`. La primera vez crea una isla con
el generador nuevo; los perfiles anteriores siguen disponibles en Opciones de
isla. Sus mapas no se regeneran ni se recolocan sus habitantes automáticamente.

Captura completa real:

    native/build/Release/isla_viva_native.exe --capture native/previews/geografia-v5.png --seed 1749

Detalle del poblado:

    native/build/Release/isla_viva_native.exe --capture native/previews/geografia-v5-poblado.png --seed 1749 --detail-preview

Vista de geografía sin objetos ni interfaz:

    native/build/Release/isla_geography_preview.exe native/build/geografia-v5.bmp 1749

El render del terreno tiene 3072 x 2304 píxeles. La cámara permite zoom de 100 %
a 3200 %. Las capturas v5 son historicas y no certifican el aspecto de la nueva
composicion modular. No se ejecutaron las pruebas ni las capturas del juego en esta entrega.
