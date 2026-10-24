# Integracion del material de C&C en el port C++

`Jugar.bat` compila y ejecuta ahora `native/build/Release/isla_viva_native.exe`.
Se necesitan Visual Studio 2022 con C++, el SDK de Windows y CMake. La primera compilacion descarga las dependencias fijadas en CMake. `Jugar-Godot.bat` conserva el arranque anterior si se instala su ejecutable de Godot.

## Sustituciones activas

El build ejecuta `tools/import_cnc_data.ps1` sobre los XML originales. Genera `CncData.h` y `cnc-import.json` en el directorio de compilacion. Cambiar los XML y volver a compilar modifica los datos del juego; no se utiliza una copia manual de esos valores.

| Fuente Red Alert 3 | Uso real en el ejecutable |
| --- | --- |
| BaseObjects/BaseCivilian.xml | Velocidad normal y de panico, intervalo y probabilidad de deambulacion, distancias de huida, salud maxima y tamano de sombra |
| GlobalData/Locomotor.xml | Aceleracion de HumanLocomotor y restriccion a suelo navegable |
| BaseObjects/BaseTree.xml y Civilian/*/Shrubbery | Catalogo de 154 variantes de vegetacion, dimensiones y altura; se instancian variantes deterministas con colision |
| Civilian/*/Buildings | Catalogo de 600 edificios con geometria numerica; el mapa intenta colocar 16 variantes y descarta las que solapan agua, otros objetos o zonas reservadas |
| GlobalData/MultiplayerColors.xml | Colores que identifican a cada habitante |
| GlobalData/WaterTemplates.xml | Color difuso del agua por manana, tarde, atardecer y noche |
| GlobalData/Weather.xml | Densidad, velocidad y opacidad de lluvia; Q alterna lluvia |
| Terrain/Terrain.xml | Catalogo de clases y recuento de 406 materiales; sus texturas no estan disponibles y no reemplazan el terreno visible |

Conversion de distancias: 24 unidades de SAGE por celda de isla. Los edificios se limitan a semianchos de 0.4-3 celdas y semiprofundidades de 0.4-2.5 para el mapa compacto. La vegetacion se limita a 0.1-0.8 celdas. Se conservan las dimensiones sin limitar en el informe de importacion. Los dibujos son representaciones 2D propias, no los modelos originales.

El importador recorre los edificios y la vegetacion de Red Alert 3; no mezcla definiciones de juegos con esquemas diferentes. Actualmente registra un XML malformado omitido: `Civilian/Mykonos_MY/Buildings/MY_Boat_01b.xml`. Las definiciones opcionales sin geometria numerica completa no entran al catalogo. Los datos principales ausentes o invalidos detienen el build.

## Simulacion conectada

Se sustituyo el desplazamiento rectilineo provisional por rutas BFS sobre suelo accesible y obstaculos derivados de la geometria importada. Los habitantes aceleran, deambulan y huyen con los parametros de los XML. Las urgencias pueden cambiar la ruta al llegar al centro de la celda, evitando cortar esquinas de obstaculos.

La logica de necesidades es propia de Dungeon Sims: buscan orillas de agua dulce, beben, comen del almacen y descansan. Comer descuenta grano. Una produccion agricola simplificada repone 36 unidades cada 120 segundos simulados; no es la agricultura completa de la version Godot. Un dia dura 300 segundos. La version nativa aun no incorpora guardado ni todas las funciones de Godot.

Controles: Espacio pausa, E alterna 1x/4x/12x, Q alterna lluvia, clic izquierdo inspecciona y clic derecho genera una alerta temporal que activa la huida. Esc cierra.

## Material que requiere otro sistema

Los archivos .fx/.fxh dependen de shaders, constantes y pases de SAGE; el framework RA actual expone dibujo 2D con SDL_Renderer y no ejecuta ese pipeline. Los modelos, texturas y audios referenciados mediante ART no estan incluidos en esta fuente. Los XSD describen comportamientos, no aportan su implementacion C++. Las bibliotecas .map requieren un lector del formato original. Permanecen como referencias; no se presentan como integradas.

## Verificacion

    cmake -S native -B native/build -G "Visual Studio 17 2022" -A x64
    cmake --build native/build --config Release --parallel 8
    ctest --test-dir native/build -C Release --output-on-failure

Las pruebas comprueban rutas continuas a agua, colisiones, recuperacion de sed, consumo real de grano, velocidad de panico y supervivencia durante doce dias. Otra prueba modifica una copia del XML y verifica que cambie la velocidad generada, sin modificar la fuente original, y exige fallo si faltan los XML principales.

`Jugar.bat -SmokeTest` ejecuta 180 cuadros con lluvia activa y cierra. Puede usarse con SDL_VIDEODRIVER=dummy y SDL_RENDER_DRIVER=software para verificar el render sin ventana.

## Procedencia

Fuente fijada en 3aad8e412961ab543192ae70f8503dc8e178e095 de glitchaka/CnC_Modding_Support_oups. El material importado conserva GPL v3 y los terminos adicionales originales. CMake copia LICENSE.md, README.md de origen y el informe cnc-import.json junto al ejecutable. Esta adaptacion es de Isla Viva y no un producto original de Electronic Arts.


