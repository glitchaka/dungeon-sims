# Visitas multijugador entre dos islas

## Conectar

Ejecuta Jugar-Multijugador.bat en el repositorio, o Multijugador.bat dentro del ZIP extraido. Un jugador elige abrir su isla; el otro elige conectar e introduce la IPv4 del primero. Puerto TCP predeterminado: 32145. Las ventanas conservan mundos distintos, no una misma isla compartida.

Para probar dos jugadores en este PC, abre el lanzador dos veces: abre una isla en la primera y conecta a 127.0.0.1 en la segunda. Los perfiles automaticos island-host e island-guest tienen partidas e identidades separadas. El panel tambien permite ABRIR MI ISLA o CONECTAR ISLA; este ultimo usa la IP proporcionada al arrancar, o localhost.

Opciones directas del ejecutable:

```
isla_viva_native.exe --host --profile manuel --port 32145
isla_viva_native.exe --join 192.168.1.20 --profile visitante --port 32145
```

Primera entrega: pareja de jugadores mediante conexion TCP directa, utilizable en localhost o una red con conectividad entre ambos equipos. No incluye cuentas, descubrimiento de partidas, servidor publico, relay ni apertura automatica de puertos de Internet. El protocolo es para jugadores de confianza; no incorpora autenticacion, cifrado ni medidas contra trampas.

## Misiones

Selecciona un habitante adulto propio e inspira diplomacia o saqueo. Viaja cuando tiene salud y necesidades suficientes, dejando dos habitantes propios. Mientras esta fuera no trabaja, colisiona ni consume recursos en su isla original. El receptor crea un visitante independiente con movimiento y necesidades sujetos a sus colisiones. No duplica el inventario original.

Tras 45 segundos simulados en la isla receptora se resuelve la visita. Diplomacia mejora diez puntos las relaciones entre esas dos islas y entrega hasta tres monedas desde el tesoro anfitrion. Saqueo empeora veinte puntos las relaciones; la resistencia de habitantes adultos cercanos causa dano al saqueador. Si sobrevive, roba como maximo doce monedas y ocho de grano del suministro real del receptor. Hay alerta local. El dano puede causar una muerte real. Los resultados se resuelven en la isla visitada, no en la de quien envia.

El visitante desaparece de la isla receptora al regresar. El original vuelve con su resultado y botin, o queda muerto si no sobrevivio. Si su lugar anterior se construyo o bloqueo mientras viajaba, busca un desembarco libre. Esta entrega usa una resolucion inicial de defensa por presencia y salud: todavia no hay combate tactico ni trayecto de barcos entre las dos islas.

## Desconexion y guardado

Cada isla tiene identidad persistente. Los viajes usan identificadores unicos, guardado del estado antes de transmitir, reenvio de mensajes pendientes y confirmacion de resultados. Repetir un paquete o reconectar no vuelve a crear el visitante ni duplica el botin. Si se corta la conexion, el residente queda fuera y su viaje pendiente se conserva; conecta otra vez las mismas islas para completar la entrega. No se genera una copia local gratuita al desconectar.

Los tiempos de ambas islas son independientes: la mision espera si el receptor pausa o cierra. No hay simulacion de servidor cuando los dos juegos estan cerrados. Una conexion sirve a otra isla por vez. El guardado local no es una base de datos transaccional distribuida ni evita que un usuario restaure manualmente copias antiguas de sus archivos.
