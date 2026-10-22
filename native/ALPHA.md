# Alpha de entorno modificable

Estado: alpha de desarrollo. El aspecto visual de referencia todavia no esta reproducido de forma identica ni aprobado por el usuario.

## Probar

En esta maquina: Jugar-Alpha.bat. Para compilar: Jugar.bat. Paquete portable: releases/IslaViva-alpha-wip.zip; extraer toda la carpeta y ejecutar isla_viva_native.exe manteniendo assets al lado.

Espacio pausa; E alterna velocidad; Q lluvia; rueda zoom 100-200%; central o WASD desplazan; Enter encuadra toda la isla. Clic izquierdo selecciona habitante. Clic derecho da una orden de movimiento o trabajo contextual: cosechar/sembrar, talar un arbol o beber. No envia una alerta.

Los botones CASA, CAMPO y CAMINO/PUENTE encargan una obra en la siguiente celda elegida. DEMOLER retira una casa, un arbol, un camino o un campo mediante trabajo de una unidad. REUBICAR pide origen y destino, conserva la entidad y requiere dos etapas de trabajo. Los costes se reservan al encargar; si la obra resulta inviable se devuelven. CANCELAR ORDEN libera la unidad y sus reservas; la obra queda en la cola para otro trabajador. Esc sale de la herramienta; Esc sin herramienta cierra el juego.

## Entorno

Casas de huella 2x2, arboles y plantas con cultivo activo son solidos tanto para habitantes como para animales. Se trabaja desde el exterior de su huella. Las rutas se calculan con el estado actual, evitan la ocupacion de otras unidades y se revisan si un objeto nuevo corta el paso. La construccion rechaza lugares que bloquearian el acceso de habitantes al agua o al almacen.

Talar retira el arbol y produce cuatro unidades de madera. Los bosques pueden extenderse a nuevas celdas libres; no se plantan sobre unidades, animales, caminos, obras, cultivos o edificios. Los habitantes pueden encargar vivienda si no tienen casa y caminos en zonas muy transitadas. La agricultura reserva parcelas y consume semillas; no genera grano por tiempo sin cosecha.

## Pesca y poblacion

PESCAR OESTE/ESTE asigna al habitante seleccionado a un barco. Primero camina al muelle y embarca. El barco recorre una ruta exclusivamente maritima, pesca treinta segundos y regresa. Un unico sorteo por viaje tiene umbral exacto de 0.25: si se pierde, mueren el ocupante y el barco desaparece; si regresa, el inventario recibe cuatro a ocho pescados consumibles. Reponer un barco perdido requiere ocho de madera y cinco monedas al encargar otro viaje.

La poblacion cuenta vivos, no un total fijo de seis. Puede disminuir por hambre, sed o perdida en el mar. Cada 180 segundos puede llegar un nuevo habitante si hay vivienda vacante, grano por encima de la reserva y ocho monedas; se consumen dos de grano para sus provisiones y esas monedas. No se han implementado familias ni nacimientos.

## Guardado

GUARDAR/CARGAR, autoguardado cada 45 segundos reales y guardado al cerrar. Archivo independiente world-v6.json en LOCALAPPDATA/IslaVivaNative. Conserva edificios, cultivos, trabajos, ordenes, bosque, poblacion, barcos y viajes activos. No importa partidas Godot ni formatos nativos anteriores.

## Arte y limites

geology.png es una textura de geologia y agua fijas, sin casas, bosques, parcelas, puentes ni muelles pintados. La mascara de navegacion se deriva de ella a resolucion 128x96. Los objetos modificables se dibujan por separado desde nature-pieces.png y building-pieces.png, con orden de profundidad por su apoyo en el suelo. Las unidades conservan los sprites originales. No hay colision a resolucion de pixel, simulacion naval meteorologica, editor completo de altura ni reproduccion visual identica de la referencia.

## Comprobacion

Build Release y CTest: colision de habitantes y animales, huella de casas, construccion, demolicion, reubicacion conservando identidad, agricultura desde fuera, tala y regeneracion, caminos, ordenes directas, limite de riesgo del 25%, viaje maritimo real, fluctuacion de poblacion y guardado/carga. Otra prueba cambia una copia del XML para comprobar su propagacion al juego.
