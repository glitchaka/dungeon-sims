# Material para el port C++

Origen: https://github.com/glitchaka/CnC_Modding_Support_oups
Revision fija: 3aad8e412961ab543192ae70f8503dc8e178e095

## Seleccion

Se incluyen las categorias disponibles de todos los juegos del repositorio:

| Categoria | Uso para Dungeon Sims | Adaptacion necesaria |
| --- | --- | --- |
| Schemas | Tipos de datos y contratos de configuracion; movimiento, cuerpos, colisiones, camara y comportamiento | Implementar lectores o convertir a estructuras propias; los XSD no implementan los comportamientos |
| Shaders | Referencias de agua, terreno, vegetacion, sombras, iluminacion y particulas | Adaptar al backend grafico y resolver dependencias del pipeline SAGE |
| Libraries | Ejemplos de bibliotecas de mapas, restricciones y listas de objetos | Formatos propios del juego; no son bibliotecas C++ enlazables |
| Xml/BaseObjects, Civilian y Neutral | Civiles, animales, edificios, arboles y objetos del entorno | Las referencias ART/DATA pueden requerir recursos ausentes |
| Xml/GlobalData, Includes, System y Templates | Locomocion y configuracion general | Convertir unidades, referencias y parametros al mundo de Dungeon Sims |
| Xml/Terrain | Configuracion de superficies | Requiere el sistema de terreno propio |
| Xml/SkirmishAI y PathfinderTest | Referencias de decisiones y escenarios de navegacion | La IA militar requiere redisenar objetivos para la simulacion de habitantes |
| Xml/FXParticleSystems | Parametros de efectos ambientales | Requiere implementar un emisor compatible |
| Xml/UI y UIInGame | Configuracion de interfaz | No proporciona una interfaz funcional para RA::Engine |

No se incluyen las grandes colecciones de mapas de campana ni las definiciones especificas de facciones militares. El inventario completo del repositorio puede consultarse en el checkout de trabajo bajo inspection/cnc-modding-support-source. La seleccion conserva los archivos auxiliares de cada directorio elegido, pero no garantiza resolver referencias hacia categorias no incluidas ni hacia assets externos.

## Estado

Los archivos originales se conservan sin modificar. Parte de sus datos ya se usa en el ejecutable C++ mediante tools/import_cnc_data.ps1; consultar native/INTEGRACION.md para las sustituciones activas y los limites. No contiene el codigo C++ del motor SAGE, una implementacion de A* ni una simulacion de necesidades. Los XML pueden nombrar modelos, texturas y sonidos que no estan en este repositorio.

MANIFEST.json registra ruta, tamano y hash de blob Git de cada archivo extraido. La herramienta verifica los bytes originales antes de copiarlos.

## Repetir la extraccion

Desde la raiz del proyecto:

    powershell -NoProfile -ExecutionPolicy Bypass -File tools/extract_cnc_material.ps1

La descarga utiliza un checkout parcial en inspection (ignorado por Git). El resultado versionable esta en references/cnc-modding-support. La herramienta actualiza los archivos seleccionados y no elimina archivos locales adicionales.

## Procedencia y licencia

Se conservan README.md y LICENSE.md originales. La fuente declara GPL v3 con terminos adicionales de Electronic Arts; consultar LICENSE.md para redistribucion y derivados. Los nombres de juegos identifican la procedencia del material.

