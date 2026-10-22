# Alpha: habitantes autonomos y poderes divinos

Abrir Jugar-Alpha.bat para jugar sin compilar. Jugar.bat compila y ejecuta. Jugar-Multijugador.bat permite abrir otra isla o conectar por IP. El ZIP portable contiene Multijugador.bat. Esta alpha sigue en desarrollo; el aspecto del mapa no es una reproduccion identica aprobada de la referencia.

## El jugador es una deidad

Clic inspecciona habitantes y objetos. El panel ya no ofrece construir casas, crear campos, mover objetos, ordenar tala o enviar barcos directamente. Tampoco el clic derecho ordena trabajo o movimiento. Las decisiones y trabajos pertenecen a los NPC.

INSP. SUSTENTO favorece agricultura y pesca. INSP. EDIFICAR favorece trabajar en obras incluso de noche. Las inspiraciones duran treinta dias de juego, sin saltarse sed, hambre, descanso ni requisitos materiales. INSP. DIPLOMACIA e INSP. SAQUEO motivan un viaje a la isla conectada cuando el habitante adulto esta preparado; deben quedar al menos otros dos habitantes propios.

ENVIAR LLUVIA / Q cambia el clima y ayuda al crecimiento de los cultivos. DEJAR REGALO permite escoger suelo libre: aparece un objeto solido con doce de grano y ocho de madera que un habitante busca, recoge desde una celda contigua y lleva al suministro comun. MILAGRO: SANAR recupera salud y necesidades basicas del habitante seleccionado.

ANUNCIAR ELEGIDO programa un solo nacimiento pendiente, vinculado al hogar del habitante seleccionado. Nueve meses equivalen a 270 dias; un dia son 300 segundos simulados. A 1x son 22.5 horas reales; a 12x, 1 h 52 min 30 s si no se pausa. El contador se guarda y solo avanza con la simulacion. Nace como menor con cero dias de edad; recibe cuidados y alimentos de la aldea y no trabaja ni viaja antes de los doce anos del calendario de 360 dias. Todavia no hay genealogia, embarazo biologico ni familias completas.

## Autonomia y entorno

Habitantes y animales respetan casas, arboles, plantas con cultivo activo y regalos. Los trabajos se ejecutan desde fuera de su huella. Tala, construccion, demolicion, reubicacion, agricultura, caminos y pesca mantienen sus operaciones internas; ya no son herramientas laborales del jugador.

Los NPC buscan alimento, agua, descanso, higiene, ocio y contacto social. Atienden obras, crean vivienda por necesidad, cultivan, amplian parcelas, pescan y obtienen madera talando arboles reales. Los campos remotos trabajados repetidamente pueden acercarse al almacen si escasea comida; las casas con salud menor de treinta generan encargos de demolicion. Si un propietario esta agotado y vive lejos del almacen, puede encargar reubicar su vivienda mas cerca, respetando costes y accesos. Las nuevas islas nacen sin caminos predefinidos: el transito reciente encarga construirlos y el desuso encarga retirarlos. Los puentes no se eliminan automaticamente. Las partidas existentes conservan caminos y los someten al mismo ciclo de uso.

La pesca requiere caminar al muelle y viajar por mar. Un unico sorteo por viaje tiene un 25% de perdida del barco y su ocupante. La poblacion tambien puede disminuir por necesidades desatendidas y crecer mediante inmigracion cuando existen vivienda y provisiones.

## Controles y partidas

Espacio pausa; E cambia entre 1x/4x/12x; rueda zoom 100-200%; central/WASD desplaza; Enter encuadra. Esc cancela la seleccion de regalo; sin herramienta, cierra. GUARDAR/CARGAR y autoguardado cada 45 segundos reales, ademas del guardado al cerrar. No se carga otra partida durante una conexion activa.

Partidas bajo LOCALAPPDATA/IslaVivaNative. world-v7.json es la partida individual; island-host-v7.json y island-guest-v7.json son islas independientes para red. La partida individual anterior world-v6.json se importa al primer arranque cuando aun no hay v7. Los archivos anteriores se conservan. Clima, regalos, inspiraciones, edad, nacimiento pendiente, identidad de isla, visitas y mensajes pendientes se guardan.

## Pruebas y limites

CTest comprueba entorno y colisiones, construccion, reubicacion, campos, caminos por uso, pesca y poblacion. Comprueba tambien dos conexiones TCP reales, visitas de diplomacia y saqueo, deduplicacion, reconexion, resultados y guardado, regalos, lluvia, necesidades y plazo exacto del elegido. MULTIPLAYER.md detalla el alcance de red. No hay reproduccion visual identica, genealogia completa ni combate tactico multijugador.

