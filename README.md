# Isla Viva

**Jugar:** abre `Jugar.exe` o `Jugar.bat` en esta carpeta. Ambos abren el mismo juego y no compilan al iniciar. Las partidas permanecen en `%LOCALAPPDATA%/IslaVivaNative`.

## Carpetas

| Carpeta | Contenido |
| --- | --- |
| native | Proyecto C++, codigo por modulos, recursos activos y documentacion |
| tools | Compilacion, importacion y empaquetado; accesos secundarios en launchers |
| external | Material externo de consulta e importacion de C&C |
| legacy/godot | Proyecto original Godot y su acceso de arranque |
| distribution/packages/current | Paquete jugable actual |
| distribution/packages/archive | Paquetes locales anteriores conservados |
| distribution/releases | ZIP publicados e historicos |
| licenses | Licencias y agradecimientos originales |
| .local | Capturas anteriores, logs y archivos temporales de trabajo; no se publica |
| .vscode | Configuracion del editor |

`.git` y `.gitignore` pertenecen al control de versiones.

## Accesos secundarios

- `tools/launchers/Jugar-Alpha.bat`: perfil geografia-v5.
- `tools/launchers/Jugar-Multijugador.bat`: abre el juego para conectar islas desde su interfaz.
- `tools/launchers/Jugar.vbs`: acceso alternativo sin consola.
- `legacy/godot/Jugar-Godot.bat`: version Godot original; requiere el motor Godot.

## Sprites y documentacion

[Personajes y recursos del video](native/assets/README.md) · [Estructura del C++](native/README.md) · [Controles](native/docs/ALPHA.md) · [Familias](native/docs/FAMILIAS.md) · [Multijugador](native/docs/MULTIPLAYER.md).

## Desarrollo

`powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_native.ps1 -BuildOnly` compila. `tools/package_alpha.ps1` actualiza `distribution/packages/current`.

El lanzador principal utiliza primero `native/build/Release/isla_viva_native.exe`; si no existe, busca `distribution/packages/current/isla_viva_native.exe`.
