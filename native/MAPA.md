# Mapa de la alpha

Abrir Jugar-Alpha.bat desde la raiz. Ejecutable: dist/IslaViva-alpha-map/isla_viva_native.exe. Jugar.bat compila y abre el mismo codigo desde native/build/Release.

El fondo recrea la isla de referencia del usuario: macizo nevado, rio y cascadas, costa con ensenadas, pueblos, campos y dos puertos. El archivo se carga desde assets/island-reference.png junto al ejecutable; no se genera el terreno plano anterior.

La simulacion utiliza una rejilla independiente de 128 por 96: mar y montanas impasables, agua dulce, caminos y puentes. Los puertos y las tres entradas de mazmorra tienen rutas desde el poblado central. La rejilla aproxima las zonas del dibujo; no es colision por pixel ni convierte cada edificio pintado en un objeto individual.

Habitantes y animales usan sprites del proyecto original; los cultivos activos se dibujan encima del fondo y reflejan el estado de la simulacion. Los detalles de pueblos y bosques del fondo son ilustrados. Las obras nuevas de la simulacion se representan por separado.

Controles de esta entrega: Espacio pausa, E velocidad, Q lluvia, clic inspeccion y clic derecho alerta. Los habitantes se desplazan autonomamente.

Procedencia del mapa y prompt: assets/MAP-PROMPT.md. El fondo se creo con imagegen a partir de la referencia, no fue extraido del repositorio de C&C. Los XML importados de C&C siguen proporcionando parametros de locomocion, salud y clima.

Verificacion realizada: compilacion Release, carga y captura de 180 cuadros desde el paquete distribuido, rutas a agua dulce, puertos y entradas, campos transitables y desplazamiento sin atravesar mar o montanas. Esto verifica el mapa de esta entrega; no declara terminadas todas las mecanicas originales.

Camara: rueda del raton de 100% a 200%, anclada al cursor; boton central y WASD desplazan; Enter vuelve al mapa completo. El panel lateral queda fuera del area de dibujo y los clics sobre el panel no seleccionan objetos del mapa. La alpha actual esta en dist/IslaViva-alpha-zoom.
