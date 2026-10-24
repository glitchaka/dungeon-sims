# Isla Viva

Abre **Jugar.exe** o **Jugar.bat**. Ambos abren la misma version nativa; no compilan al jugar. **Jugar-Alpha.bat** abre el perfil geografia-v5. Las partidas se conservan en `%LOCALAPPDATA%/IslaVivaNative`.

## Estructura del proyecto

- `native/src/`: codigo C++ por modulo: app, world, simulation, residents, rendering, network, powers, audio y entities.
- `native/assets/runtime/`: unicos recursos que carga y empaqueta el juego, separados por categoria.
- `native/assets/source/`: entradas de generacion necesarias para compilar.
- `native/assets/archive/`: arte anterior y experimentos; no se carga en el juego.
- `native/docs/`: documentacion actual de mecanicas, controles y arquitectura.
- `native/tests/`: pruebas existentes; separadas del codigo de juego.
- `native/build/`: compilacion local y ejecutables generados; ignorado por Git.
- `dist/IslaViva-alpha-family-life/`: paquete jugable activo.
- `releases/`: paquetes ZIP historicos.
- `tools/`: herramientas de compilacion, importacion y empaquetado.
- `legacy/godot/`: proyecto original de Godot, independiente del C++.
- `references/`: material externo de consulta e importacion, incluido C&C.
- `inspection/`: archivos temporales de trabajo y referencias extraidas; ignorado por Git.

## Sprites y animaciones

Los personajes detallados del video estan en [native/assets/runtime/characters](native/assets/runtime/characters). Las hojas v6 son adultos y menores; las v7 contienen trabajo, cortejo y embarazo. Barcos y remeros estan en `runtime/boats`; la nube en `runtime/effects`. [Indice de recursos](native/assets/README.md).

## Documentacion

[Funciones y controles](native/docs/ALPHA.md) · [Familias](native/docs/FAMILIAS.md) · [Multijugador](native/docs/MULTIPLAYER.md) · [Terreno](native/docs/TERRENO.md) · [Organizacion del C++](native/README.md).

## Desarrollo

`powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_native.ps1 -BuildOnly` compila. `tools/package_alpha.ps1` copia el ejecutable y el arbol runtime al paquete activo. El lanzador de la raiz utiliza primero `native/build/Release/isla_viva_native.exe`.
