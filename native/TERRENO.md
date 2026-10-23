# Geografía v5: relieve y referencia del usuario

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

- `terrain-materials-v5.png`: seis materiales sin geografía pintada.
- `nature-reference-v5.png`: árboles, cultivos y rocas independientes.
- `buildings-reference-v5.png`: viviendas, almacén, mercado y piezas portuarias.
- `civic-reference-v5.png`: edificios cívicos y afloramiento de cristales.

Se generaron con imagegen integrada, usando la imagen del usuario directamente.
Los prompts exactos están en `assets/REFERENCE-V5-PROMPTS.md`. `SceneTexture`
extrae los recortes por componentes alfa y utiliza alfa premultiplicado, mipmaps
y filtrado lineal para el detalle de paisaje. Los personajes mantienen su atlas
animado heredado. El menú muestra la imagen de referencia.

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
a 800 %. Las pruebas comprueban generación reproducible, acceso al agua,
dispersión de viviendas y habitantes, efecto visual de la altura e inversión de
la proyección para el ratón, además de los flujos de simulación existentes.