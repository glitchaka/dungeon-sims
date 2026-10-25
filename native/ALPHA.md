# Probar la alpha

Actualizacion 2026-10-24: clic sobre el personaje lo selecciona; F/SEGUIR centra y sigue, Tab/SIGUIENTE cambia de habitante y Enter muestra la isla. MSPA v1.0 esta integrado en decisiones, percepcion, resultados y guardado. Las islas nuevas usan generacion fisica por bloques. La inmigracion depende de vivienda libre, comida y dinero; los encuentros familiares ya esperan al segundo participante y liberan reservas caducadas. Detalles y limites de verificacion en INTEGRACION-MSPA.md.

Abre **Jugar.exe** con doble clic, en el proyecto o en el ZIP extraido. Es un lanzador grafico y el juego usa el subsistema Windows: no mantiene una consola CMD. Jugar.bat y Jugar-Alpha.bat solo delegan en ese lanzador y terminan. Jugar.vbs es otro acceso sin consola. Para desarrollo se conserva tools/run_native.ps1, que compila; Jugar.bat ya no compila al abrir una partida.

La primera pantalla pide tu nombre y permite continuar o crear una isla. Opciones de isla contiene perfil y semilla. Durante la partida puedes ABRIR MI ISLA / CERRAR MI ISLA o CONECTAR ISLA. No son modos de juego.

`Jugar-Alpha.bat` selecciona el perfil `geografia-v5`. Las partidas anteriores se conservan en sus perfiles y se pueden abrir desde Opciones de isla. TERRENO.md describe el relieve fisico y la composicion visual con los bloques preparados en terrain-blocks-v3.

## Jugador y habitantes

Clic inspecciona. El jugador puede inspirar sustento, edificacion, diplomacia o saqueo; enviar lluvia; sanar; dejar un regalo; anunciar el elegido. El regalo ocupa espacio y se recoge desde fuera. El elegido nace tras 270 dias de juego, no inmediatamente; el contador se guarda. Un dia son 300 segundos simulados.

Ahora tambien hay encargos de MUELLE, TOWNHALL y ALTAR: pulsa el boton y elige suelo adecuado. Se reservan recursos y el habitante seleccionado recibe el encargo, que puede interrumpir por necesidades o descanso. Otros trabajadores pueden retomarlo. No se crea un edificio instantaneo.

Muelle: 16 madera y 12 monedas, junto al mar; aporta un barco funcional. Townhall: 24 madera y 25 monedas; actua como nuevo centro civico y almacen. Altar: 8 madera y 10 monedas; mejora socializacion y animo cerca de el. Conservan tipo propio, colisiones y guardado. Ninguno cuenta como vivienda.

Los NPC deciden agricultura, tala, ampliacion de campos, viviendas, caminos y pesca. Por la noche interrumpen obras y vuelven a descansar; hambre/sed urgentes pueden despertarlos. Los viajeros ya embarcados terminan su travesia. El texto NOCHE y la simulacion usan el mismo reloj. La inspiracion de edificacion ya no obliga a trabajar toda la noche.

## Caminos y migracion

Las partidas nuevas no tienen una red de caminos inicial. El uso reciente crea encargos de caminos y el abandono crea encargos de retirada. Al importar archivos v6/v7 se retiran una sola vez los caminos antiguos y sus encargos obsoletos, conservando estructuras, habitantes y puentes necesarios. Los archivos originales se conservan. Los caminos construidos bajo v8 se guardan y no se borran en cada arranque.

## Controles y guardado

Espacio pausa; E cambia velocidad; Q lluvia; rueda zoom; central/WASD desplaza; Enter encuadra. CHAT abre escritura; Enter envia y Esc cierra. Mientras escribes, las teclas no ordenan mover la camara ni cambian el clima.

LOCALAPPDATA/IslaVivaNative contiene partidas v9 separadas por perfil. Se importa v7 y, para la partida individual antigua, v6. Conserva identidad, visitas pendientes, chat, edificios, alturas, semilla, geografia, ordenes y nacimiento elegido. Guardado manual, automatico y al cerrar. La importacion no mezcla las dos islas ni sus habitantes.

## Estado

Build Release y pruebas de simulacion, TCP real, chat, diplomacia/saqueo, deduplicacion, reconexion, guardado, islas por semilla, crecimiento, pendientes, obras y descanso nocturno. TERRENO.md describe geografia y arte. MULTIPLAYER.md describe conexion y alcance. El acabado visual aun esta en revision; no es una reproduccion aprobada de la isla de referencia.

## Islas grandes

Las partidas nuevas usan 256 x 192 celdas (49.152), cuatro veces la superficie anterior de 128 x 96. El generador amplía costas, cordilleras, valles y bosques; el tamaño físico de habitantes, construcciones y campos se conserva. La cámara encuadra toda la isla con Enter y permite zoom de 100 % a 800 % con la rueda y desplazamiento con WASD o botón central.

Las partidas se guardan en archivos v9. Se pueden importar v6/v7/v8 de 128 x 96: se conserva la isla con sus coordenadas y se remapean los índices de celdas al nuevo ancho, dejando mar en el espacio adicional. El archivo antiguo queda intacto. Para obtener una isla nueva de mayor superficie, marca Nueva isla en la pantalla inicial.

## Decisiones cívicas autónomas

Los habitantes pueden iniciar un muelle si falta comida o una inspiración diplomática/de saqueo necesita transporte; un altar si el bienestar social es bajo y hay provisiones; un townhall si crece la comunidad o existe inspiración de edificación; y una vivienda adicional cuando hay recursos para admitir nuevos pobladores. Se eligen terrenos válidos, se reservan recursos y se ejecutan trabajos reales, con prioridad para necesidades urgentes, viviendas ausentes y caminos necesarios. No se duplican obras cívicas ya existentes o en cola ni se inicia esta planificación de noche.

La vivienda se entrega primero a un adulto sin casa. Si todos tienen vivienda, queda libre para un inmigrante: el constructor conserva su casa anterior. Los encargos manuales siguen disponibles como intervención opcional.

Familias y ciclo de vida implementados: detalles en FAMILIAS.md. Descanso nocturno dentro de viviendas, con salida por necesidades urgentes.

Los habitantes usan los atlas v9: seis fases por accion, cuatro direcciones para caminar y remar, atlas separados para adultos, menores y etapas del embarazo. Hay secuencias de labranza, tala, siembra, cosecha, construccion, caza, comida, bebida, descanso, aseo, conversacion, transporte, abastecimiento, comercio, cuidado de animales, campamento, botadura, remo y pesca. Se aplican tambien a partidas existentes. El catalogo de filas esta en assets/animation-v9.json.

El cortejo adulto conserva ocho segundos (sorpresa inicial y saludo reciproco), la nube opaca con corazones dura seis segundos y conserva ocho frames. El embarazo es visible desde el segundo trimestre y cambia la silueta en el tercero. Tras el parto se recupera el sprite normal. Cortejo y botadura se guardan; partidas v9 anteriores reciben valores por defecto.

Zoom ampliado: rueda hasta 3200% (antes 800%). El zoom conserva el punto bajo el cursor. Los pasos, remos y efectos usan un reloj visual independiente de 1x/4x/12x, y las posiciones se interpolan entre pasos de simulacion de 100 ms. La pausa detiene tambien las animaciones. Los objetos fuera de la vista se descartan antes de comprobar su ocultacion por el relieve.
