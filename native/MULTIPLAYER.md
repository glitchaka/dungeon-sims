# Partidas multijugador sin consola

Abre Jugar.exe. Escribe tu nombre y elige **Abrir mi isla a otro jugador**. Comparte la direccion IPv4 que muestra la pantalla (hay un boton para copiarla). El otro jugador abre Jugar.exe, elige **Conectar a otra isla** y escribe esa direccion. No se necesitan comandos. Dentro del juego, CONECTAR ISLA tambien abre un cuadro de direccion y puerto; ya no intenta conectar ciegamente a localhost.

Para probar dos ventanas en el mismo PC: abre una como anfitrion y conecta la otra a 127.0.0.1. Usa perfiles distintos; si se deja default, el modo anfitrion usa island-host y el modo cliente usa island-guest. Cada perfil tiene nombre, partida e identidad de isla propios.

Puerto TCP predeterminado 32145. Es una conexion directa entre dos jugadores, utilizable en LAN o cuando los equipos tienen conectividad TCP por esa direccion. Esta entrega no incluye servidor publico, directorio de partidas, relay, apertura automatica de puertos de Internet, cuentas ni medidas contra trampas.

## Chat y visitas

CHAT abre un campo de escritura. Enter envia; Esc cierra la escritura. Los mensajes llevan el nombre elegido, viajan por la misma conexion TCP y se confirman sin duplicarse al retransmitir. El historial reciente se guarda por isla. No hay chat global fuera de la partida conectada.

Inspira diplomacia o saqueo en un habitante adulto. Viaja cuando esta preparado y quedan dos habitantes propios. El receptor crea un visitante con movimiento y necesidades sujetos a sus colisiones. Mientras esta fuera, el original no trabaja ni colisiona en su isla.

Tras 45 segundos simulados en la isla receptora se resuelve la visita. Diplomacia mejora relaciones y puede entregar tres monedas del anfitrion. Saqueo empeora relaciones; defensores cercanos pueden danar o matar al atacante. Si sobrevive, roba hasta doce monedas y ocho de grano del suministro real de esa isla. La resolucion de defensa es inicial, no combate tactico completo. No se representa todavia la travesia naval entre ambas islas.

Cada viaje tiene identificador persistente y resultado confirmado. Una desconexion conserva el viaje y los mensajes pendientes; reconectar las mismas islas no duplica habitantes ni botin. Si el receptor pausa, el tiempo de su mision se pausa. No hay servidor que simule cuando todos cierran. Restaurar manualmente copias antiguas de los archivos no esta protegido por un sistema contra trampas.
