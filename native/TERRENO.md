# Terreno y paquete de recursos

assets/terrain-islands.pck es un contenedor nativo de Isla Viva, formato IVTPACK2; no es un PCK del motor Godot. Incluye metadatos, el atlas de materiales y piezas de relieve con transparencia. tools/build_terrain_pack.ps1 lo reconstruye desde native/assets/terrain-pack. Las fuentes y los prompts se conservan junto al paquete.

Las nuevas islas varian por semilla. El modelo guarda tipos de suelo, alturas, formas geologicas y curso del rio. El renderer mezcla superficies y proyecta las alturas; las pendientes intervienen en navegacion y las obras requieren una base suficientemente plana. La cuadrícula interna sigue siendo de 128x96 para la simulacion; no se dibuja como una malla visible.

Las piezas tienen significados distintos: montana nevada con huella y altura grandes; cerro con menor huella; monte pequeno; rampa transitable que conecta una meseta elevada. La rampa no se usa como un objeto diminuto repetido a lo largo de la costa. Las paredes de costa proceden de diferencias de altura del suelo. El cauce tiene centros continuos y alturas descendentes, con caidas al cambiar de nivel.

AMPLIAR ISLA agrega suelo conectado a la costa dentro del mapa existente; no expande ilimitadamente las dimensiones del mundo. Respeta las posiciones y rutas activas de los barcos. Las nuevas celdas heredan nivel de tierra vecina y actualizan navegacion y dibujo.

El mapa ilustrado anterior permanece disponible para partidas antiguas. Las pruebas de texturas con juntas, montanas en filas y adornos costeros pequenos fueron descartadas como resultado visual. Las nuevas piezas de edificios se conservan. La nueva geografia todavia requiere revision visual del usuario y no se declara equivalente ni aprobada como recreacion exacta de la referencia.
