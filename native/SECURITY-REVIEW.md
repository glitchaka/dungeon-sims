# Revisión de seguridad — 2026-10-22

## Resultado observado

Microsoft Defender estaba activo, con protección en tiempo real activa y firmas del 2026-10-21. Se ejecutó un análisis personalizado de la carpeta del proyecto y otro del paquete dist/IslaViva-alpha-islands. Ambos terminaron correctamente. Get-MpThreatDetection no devolvió detecciones asociadas a DUNGEON-SIMS. Evidencia estructurada: native/previews/defender-review.json.

Esto documenta el resultado de este antivirus en esta máquina; no prueba ausencia absoluta de malware ni garantiza que otros motores no produzcan falsos positivos.

## Código y distribución

Se revisaron los fuentes nativos, lanzadores BAT/VBS y herramientas de desarrollo buscando ejecución de comandos descargados, PowerShell codificado, modificación del antivirus, persistencia de inicio, inyección de procesos y descargas en tiempo de ejecución. No se encontraron esas operaciones en el código propio examinado. CMake obtiene dependencias durante la compilación; el jugador no necesita descargarlas al arrancar.

El lanzador ejecuta exclusivamente el juego ubicado junto a él o en dist/IslaViva-alpha-islands. Se retiró CREATE_NO_WINDOW: ambos programas son aplicaciones Windows y no requieren ocultar una consola hija. Se añadieron recursos de versión, descripción y nombre de producto. No se añadieron exclusiones al antivirus, ofuscación ni empaquetadores de ejecutables.

El paquete contiene dos ejecutables normales de MSVC y un BAT que abre Jugar.exe. No incluye scripts PowerShell ni VBScript. Sus hashes SHA256 están en SHA256SUMS.txt dentro del paquete. Las herramientas de desarrollo usan PowerShell; no son necesarias para jugar.

## Limitaciones relevantes

Jugar.exe e isla_viva_native.exe NO están firmados digitalmente. Los metadatos de versión y hashes no sustituyen una firma de editor ni garantizan reputación en SmartScreen. No hay certificado de firma disponible en esta tarea.

El multijugador directo abre un puerto TCP cuando el usuario elige alojar o conecta a la IPv4 elegida. No hay cifrado ni autenticación de sesión; este análisis no lo certifica como servicio seguro para Internet público. La lógica limita tamaños de mensajes y colas, pero no sustituye una auditoría de red completa.

## Verificación funcional

Compilación Release correcta. Cuatro pruebas existentes aprobadas y una nueva prueba aprobada para el terreno cenital: tamaño, opacidad, ausencia de desplazamiento por altura, cambio del render al modificar el mundo y rechazo de paquete ausente. El ejecutable compilado y el empaquetado arrancaron con semilla 1749, guardaron una captura real y terminaron con código 0.

## Actualización de islas grandes

Recompilado para mapas nuevos de 256 x 192 celdas y guardados v9. Las cinco pruebas pasaron con las nuevas dimensiones. La prueba de islas incluye migración de cuadrículas v8 de 128 x 96, caminos, regalos, rutas de habitantes y guardado posterior. El ejecutable empaquetado generó una isla grande con semilla 1749, guardó native/previews/game-large-island.bmp y terminó con código 0. Defender se volvió a ejecutar sobre el paquete actualizado; el JSON de evidencia incluye los hashes de ambos ejecutables.

## Actualización de autonomía y visitas navales

La compilación de esta revisión incluye obras cívicas por necesidad, viviendas realmente vacantes para inmigración y salida/regreso en barco para nuevas visitas. Cinco pruebas aprobadas; las pruebas de multijugador usan dos conexiones TCP reales y comprueban embarque persistido, salida al mar antes de transferir al visitante, mensajes repetidos y desembarco alternativo al bloquear el muelle. La alpha empaquetada arrancó y terminó con código 0. El análisis Defender del paquete actualizado se registra con hashes en native/previews/defender-review.json. Se mantienen las limitaciones de firma y conexión directa documentadas arriba.

Revision family-life: compilacion Release y siete pruebas aprobadas. Microsoft Defender completo el analisis del paquete dist/IslaViva-alpha-family-life con cero detecciones del proyecto. Hashes en previews/defender-review.json. Los ejecutables siguen sin firma digital; no se garantiza ausencia absoluta de malware.
