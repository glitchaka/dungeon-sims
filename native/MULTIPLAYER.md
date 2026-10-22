# Partidas multijugador sin consola

Inicia tu isla con tu nombre. Durante la partida, ABRIR MI ISLA permite recibir visitas y cambia a CERRAR MI ISLA. El otro jugador usa CONECTAR ISLA e introduce direccion y puerto dentro del juego. No hay roles de conexion al iniciar ni comandos obligatorios.

Para dos ventanas en el mismo PC usa perfiles distintos en Opciones de isla; abre una y conecta la otra a 127.0.0.1. No uses el mismo perfil simultaneamente.

Puerto TCP predeterminado 32145. Es una conexion directa entre dos jugadores, utilizable en LAN o cuando los equipos tienen conectividad TCP por esa direccion. Esta entrega no incluye servidor publico, directorio de partidas, relay, apertura automatica de puertos de Internet, cuentas ni medidas contra trampas.

## Chat y visitas

CHAT abre un campo de escritura. Enter envia; Esc cierra la escritura. Los mensajes llevan el nombre elegido, viajan por la misma conexion TCP y se confirman sin duplicarse al retransmitir. El historial reciente se guarda por isla. No hay chat global fuera de la partida conectada.

Inspira diplomacia o saqueo en un habitante adulto. Viaja cuando esta preparado y quedan dos habitantes propios. El receptor crea un visitante con movimiento y necesidades sujetos a sus colisiones. Mientras esta fuera, el original no trabaja ni colisiona en su isla.

Tras 45 segundos simulados en la isla receptora se resuelve la visita. Diplomacia mejora relaciones y puede entregar tres monedas del anfitrion. Saqueo empeora relaciones; defensores cercanos pueden danar o matar al atacante. Si sobrevive, roba hasta doce monedas y ocho de grano del suministro real de esa isla. La resolucion de defensa es inicial, no combate tactico completo. La salida y el regreso se representan en el mar de la isla de origen; no hay un mapa oceanico continuo compartido entre ambas islas.

Cada viaje tiene identificador persistente y resultado confirmado. Una desconexion conserva el viaje y los mensajes pendientes; reconectar las mismas islas no duplica habitantes ni botin. Si el receptor pausa, el tiempo de su mision se pausa. No hay servidor que simule cuando todos cierran. Restaurar manualmente copias antiguas de los archivos no esta protegido por un sistema contra trampas.

## Embarque y regreso

Las nuevas visitas necesitan un barco libre y un muelle accesible. El habitante camina al embarque; el barco recorre celdas de mar hasta alta mar. Solo entonces se envia el mensaje que crea al visitante en una costa libre de la otra isla, conectada al poblado. El inspector muestra caminata al muelle, navegacion, visita y regreso.

Al recibir el resultado, el barco inicia su regreso y el original sigue fuera de la isla hasta desembarcar. Si el desembarco original quedo bloqueado, busca otra costa libre; no atraviesa una construccion. Los estados de embarque, navegacion y espera se guardan. Los mensajes repetidos durante el regreso no duplican recompensas. Los viajes de versiones anteriores conservan su ruta de compatibilidad al recibir resultados.
