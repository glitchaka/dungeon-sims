# Geografia v2: arte nuevo

Este directorio sustituye las pruebas de texturas planas, bloques de nieve y objetos costeros diminutos rechazadas por el usuario. Contiene regiones geograficas nuevas, con suelo continuo y desniveles visibles; no se reciclan los cuatro sprites del paquete rechazado.

Montana: una cordillera amplia, con crestas, valles y gargantas; tiene una huella mucho mayor que un cerro o un monte. Costa: una meseta extensa sobre paredes continuas de roca, con una cala y una playa a menor altura. Altiplano: un cerro, un monte menor y una rampa natural que da acceso a una meseta.

Son nuevos candidatos de arte y no estan aprobados ni conectados automaticamente al renderer actual. Las alturas y mascaras de navegacion deben ajustarse a las nuevas formas antes de incorporarlas al juego. El manifiesto declara escala y funcion; no pretende que una sola mascara rectangular represente toda una region.

El contenedor terrain-geography-v2.pck usa IVGEO002 y agrupa los tres PNG, el manifiesto y los prompts. No es un PCK de Godot. Se reconstruye con tools/build_geography_pack.ps1. El commit 81304993 conserva el estado solicitado antes de esta nueva propuesta.
