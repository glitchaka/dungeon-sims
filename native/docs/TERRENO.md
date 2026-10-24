# GeografÃ­a v5: relieve y referencia del usuario

La referencia vigente es `native/reference/island-user-reference.png`, aportada
por el usuario el 2026-10-22. El generador anterior y el render cenital v4 fueron
retirados del ejecutable. Los archivos y muestras v4 son histÃ³ricos.

## Mundo y cÃ¡mara

La isla usa 256 x 192 celdas con un campo de elevaciones real. `IslandGeography`
organiza una cordillera quebrada al noreste, mesetas irregulares con barrancos,
una cuenca occidental, cabeceras lacustres, dos drenajes, bahÃ­as e islotes. La
semilla cambia el detalle de costa, erosiÃ³n y vegetaciÃ³n manteniendo esa direcciÃ³n
geogrÃ¡fica. La organizaciÃ³n toma la imagen como referencia; no es una
reconstrucciÃ³n topogrÃ¡fica exacta de cada accidente.

`TerrainRenderer` aplica materiales a esas alturas y calcula pendientes,
iluminaciÃ³n y sombras. `TerrainProjection` proyecta altura hacia la pantalla,
resuelve la superficie visible y convierte el ratÃ³n de vuelta a coordenadas del
mundo. El render y la selecciÃ³n usan la misma proyecciÃ³n oblicua ortogrÃ¡fica.
La navegaciÃ³n y la construcciÃ³n siguen usando las alturas fÃ­sicas.

Los Ã¡rboles, viviendas, campos, cristales, embarcaciones y edificios cÃ­vicos son
objetos independientes. El suelo no es una ilustraciÃ³n completa con un poblado
pintado. `SettlementLayout` distribuye habitantes y casas mediante candidatos
mezclados por semilla, separaciÃ³n entre viviendas y validaciÃ³n de acceso al agua.

## Recursos activos

- `terrain-materials-v5.png`: seis materiales sin geografÃ­a pintada.
- `nature-reference-v5.png`: Ã¡rboles, cultivos y rocas independientes.
- `buildings-reference-v5.png`: viviendas, almacÃ©n, mercado y piezas portuarias.
- `civic-reference-v5.png`: edificios cÃ­vicos y afloramiento de cristales.

Se generaron con imagegen integrada, usando la imagen del usuario directamente.
Los prompts exactos estÃ¡n en `assets/REFERENCE-V5-PROMPTS.md`. `SceneTexture`
extrae los recortes por componentes alfa y utiliza alfa premultiplicado, mipmaps
y filtrado lineal para el detalle de paisaje. Los personajes mantienen su atlas
animado heredado. El menÃº muestra la imagen de referencia.

## Abrir y verificar

`Jugar-Alpha.bat` abre el perfil `geografia-v5`. La primera vez crea una isla con
el generador nuevo; los perfiles anteriores siguen disponibles en Opciones de
isla. Sus mapas no se regeneran ni se recolocan sus habitantes automÃ¡ticamente.

Captura completa real:

    native/build/Release/isla_viva_native.exe --capture native/previews/geografia-v5.png --seed 1749

Detalle del poblado:

    native/build/Release/isla_viva_native.exe --capture native/previews/geografia-v5-poblado.png --seed 1749 --detail-preview

Vista de geografÃ­a sin objetos ni interfaz:

    native/build/Release/isla_geography_preview.exe native/build/geografia-v5.bmp 1749

El render del terreno tiene 3072 x 2304 pÃ­xeles. La cÃ¡mara permite zoom de 100 %
a 800 %. Las pruebas comprueban generaciÃ³n reproducible, acceso al agua,
dispersiÃ³n de viviendas y habitantes, efecto visual de la altura e inversiÃ³n de
la proyecciÃ³n para el ratÃ³n, ademÃ¡s de los flujos de simulaciÃ³n existentes.

