# Isla Viva

Abre **Jugar.bat**. Usa el Godot 4.7.2 incluido en `godot`. El proyecto editable es `project.godot`.

## Presentación y controles

La ventana es **sin bordes ni barra de título**. La interfaz está dentro del juego y tiene controles propios; no utiliza temas, controles, fuentes ni iconos predeterminados de Godot. El texto pixelado se compone con un píxel del atlas original. Los terrenos, personajes, animales y construcciones usan sus celdas de 8×8, ampliadas con nearest.

El mapa es una isla compacta diseñada por zonas: aldea, lago y río, bosques con claros, montañas, campos, playas y caminos conectados con curvas, cruces y puentes. La vista inicial amplía los sprites a escala entera 2×. Los habitantes usan las personas de `MAS-Characters-P8.png` y `MAS-Characters-DB.png`; el personaje anterior ya no aparece.

- **Retratos / clic en un habitante / Tab:** cambiar la persona inspeccionada. Su nombre, intención, inventario y seis necesidades aparecen en el panel inferior.
- **Necesidades:** tienen nombre y valor numérico. Pasar el ratón o hacer clic explica qué significan y cómo se satisfacen.
- **Clic en trigo:** abre el estado real de la parcela, crecimiento, cosechas completadas y trabajador actual.
- **Clic en un animal:** identifica la especie y su relación con la caza.
- **PAUSA / Espacio:** detener o continuar el tiempo.
- **1×, 4×, 12×, 32× / teclas 1–4:** cambiar velocidad.
- **SEGUIR / ISLA / F / rueda:** seguir al habitante seleccionado o volver al mapa completo.
- **DIARIO / J:** ver las acciones recientes realizadas por los habitantes.
- **GUARDAR / F5 y CARGAR / F9:** persistir o recuperar la partida.
- **SALIR / Esc:** guardar y cerrar. **F11:** alternar pantalla completa.

Los clics de inspección no dan órdenes de movimiento. Todos los habitantes deciden por sí mismos.

## Agricultura conectada con la vida de la aldea

Cada celda cultivada tiene estado persistente: tierra preparada, sembrada, creciendo o madura. La apariencia cambia con ese estado.

1. Un agricultor reserva una parcela accesible; otro no puede trabajar simultáneamente el mismo lugar.
2. Sembrar consume **una semilla del inventario**.
3. El trigo pasa por brote, crecimiento y maduración. Tarda 120 segundos simulados de crecimiento; de noche crece más despacio.
4. Cosechar retira el trigo visible y produce **3 de grano y 2 semillas**. Un cultivo inmaduro no puede cosecharse.
5. Los habitantes llevan la cosecha al granero y conservan provisiones. Otros habitantes retiran grano de ese almacén para comer. Comer consume una unidad y aumenta la saciedad.
6. Los agricultores vuelven a sembrar la tierra cosechada y preparan **parcelas nuevas**, gradualmente, en suelo cercano apto. No pueden cultivar encima de caminos, edificios, árboles o agua. La ampliación está limitada a 24 celdas adicionales en esta versión.
7. El grano maduro no recogido se pierde con el tiempo. Con suficientes reservas, reducen la siembra; la prioridad de cosecha protege las reservas de semillas.

No se suman comida o semillas simplemente por estar en un bosque o sobre un campo. Inventario, almacén, producción y consumo están conectados y sus cantidades se verifican en las pruebas.

## Supervivencia autónoma

Hay seis habitantes con personalidad, hambre, sed, energía, sociabilidad, diversión, higiene, salud y relaciones. Cazan animales del bosque para obtener raciones; la fauna se desplaza y repuebla gradualmente. Buscan agua dulce en orillas accesibles, se lavan, conversan, exploran y comercian madera. La sed o el hambre urgente pueden interrumpir otras tareas.

Lejos de la aldea, el cansancio puede llevarlos a recolectar madera y montar un campamento: recolectar da cuatro unidades y construir consume tres. Los campamentos permanecen y pueden reutilizarse. El ciclo día/noche cambia gradualmente la iluminación y las prioridades de descanso. Un día dura cinco minutos a velocidad normal.

## Guardado

El guardado conserva necesidades, inventarios, relaciones, fauna, hora, campamentos, parcelas ampliadas, etapas de crecimiento, historial de cosechas y grano comunitario. Se realiza cada 45 segundos reales y al cerrar normalmente.

Esta versión usa `user://isla_viva_v2.json`. El archivo de la primera versión no se sobrescribe: sus posiciones corresponden a otro mapa. En Windows, `user://` está dentro de `%APPDATA%\Godot\app_userdata\DUNGEON SIMS · Isla viva`.

## Verificación

Desde PowerShell, en la raíz:

```powershell
.\godot\Godot_v4.7.2-stable_win64_console.exe --headless --path . --script res://tests/simulation_test.gd
.\godot\Godot_v4.7.2-stable_win64_console.exe --headless --path . --script res://tests/farming_test.gd
.\godot\Godot_v4.7.2-stable_win64_console.exe --path . --script res://tests/presentation_test.gd
```

Las suites verifican supervivencia durante doce días, obstáculos, caza, agua, campamentos, conservación de semillas y grano, expansión agrícola, ciclos repetidos, guardado y controles mediante eventos de ratón. La última abre una ventana breve y comprueba también el modo sin decoración y la selección de los atlas correctos.

La simulación todavía es una base: no incluye interiores habitables, familias ni la complejidad completa de Los Sims. Las tareas de construcción de campamentos, comercio y relaciones son sencillas. El mundo solo avanza mientras está abierto.

Assets: **Aleksandr Makarov / IKnowKingRabbit**, Micro Asset Series. Se conservan todos los originales de `Res` y su `license.txt`. No se descargaron recursos gráficos externos.