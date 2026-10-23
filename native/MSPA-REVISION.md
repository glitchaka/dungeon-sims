# Revisión de MSPA para los habitantes de Isla Viva

Revisión del 22 de octubre de 2026. Snapshot examinado: [`decde8c10fa69de0b289521ee83ab416c4b68da4`](https://github.com/glitchaka/MSPA-Lib/tree/decde8c10fa69de0b289521ee83ab416c4b68da4). Se revisaron código, README, especificación 2.4, contrato de interacción 0.2, contrato de Domain Pack 0.1 y arquitectura 0.1.

## Conclusión

La base es compatible con Isla Viva: C++17, biblioteca independiente del host, identificadores explícitos, reloj psicológico, registro semántico y API de observación, decisión, resultados y transferencia. Su implementación actual es provisional, como reconoce el README. Todavía no cumple el comportamiento psicológico descrito en los contratos.

Antes de usarla como política principal hay que conectar la psicología con la decisión y corregir validación, cuotas y conversión temporal. Enchufar ahora `decide()` no haría que hambre, personalidad, relaciones o aprendizaje cambiaran las preferencias por sí mismos.

Esta revisión no activa MSPA en el juego ni modifica su comportamiento.

## Hallazgos prioritarios

### 1. La decisión no utiliza el estado psicológico

[`UtilityComposer::score`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/decision/decision.cpp) recibe únicamente `ActionOption`. Calcula coste negativo, riesgo negativo y progreso acotado; no recibe `AgentPsychState`. [`Runtime::decide`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/runtime/runtime.cpp) añade 0,01 por continuidad y elige el primer máximo. La política de histéresis y el registro de consideraciones existen por separado, pero este recorrido no los utiliza.

Reproducción: dos agentes con audacia y sociabilidad opuestas obtuvieron puntuaciones idénticas para las mismas opciones. El alcance general del problema se confirma además leyendo la firma y el cálculo del compositor.

Consecuencia: incluir personalidad, emociones y recuerdos en estructuras no basta para producir conducta. Hay que incorporar sus consideraciones al recorrido público de decisión, con pruebas que demuestren diferencias explicables en situaciones controladas.

### 2. OBSERVE permite entradas incompletas y crecimiento fuera de cuota

[`PsychologyEngine::observe`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/psychology/engine.cpp) no comprueba los roles requeridos por el predicado antes de añadir una creencia. La comprobación de límites en creación/importación no protege ese crecimiento durante la observación.

Reproducciones independientes dentro del mismo programa de diagnóstico:

- Un predicado con rol obligatorio `actor` aceptó una proposición sin argumentos.
- Con `max_beliefs_per_agent=1`, una segunda observación fue aceptada y quedaron dos creencias.

Hay que validar la proposición y aplicar una política explícita de rechazo, consolidación o descarte antes de modificar el estado. Las observaciones repetidas tampoco deberían convertirse sin límite en entradas duplicadas.

### 3. La migración temporal deja marcas sin convertir

[`import_agent_state`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/persistence/state_transfer.cpp) rebasa el último procesamiento, las creencias y los recuerdos. Omite `mood.updated_at` y `emotions[].started_at`.

Reproducción: exportación en tick 100 con marcas en 90, reloj origen de 1 tick/segundo; importación en tick 300 con destino de 2 ticks/segundo. Las tres marcas deberían quedar en 280. Se obtuvo `last=280, mood=90, emotion=90`.

Debe corregirse antes de transferir viajeros entre islas: las duraciones psicológicas quedarían alteradas.

### 4. Los módulos avanzados todavía no forman un ciclo psicológico completo

Por inspección de [`runtime.cpp`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/runtime/runtime.cpp), `advance_to()` actualiza la marca temporal sin ejecutar el decaimiento de emociones o ánimo. `report()` mantiene estadísticas por tipo de acción sin contexto del resultado; su entrada tampoco identifica la instancia de acción ni distingue una interrupción. El motor de observación añade creencias mediante una valoración mínima y no coordina todos los sistemas psicológicos independientes.

Por inspección de [`domain_pack.cpp`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/semantics/domain_pack.cpp), el pack contiene reglas de inferencia, pero su aplicación registra predicados, conceptos y acciones sin incorporar esas reglas al recorrido de ejecución.

Son diferencias entre el contrato y lo implementado; no afirmaciones de que hayan fallado pruebas oficiales de esas capacidades.

## Compilación y comprobación realizadas

Se descargó el snapshot fijado a `native/build/mspa-review/source`, sin cambiar sus fuentes. Se compiló con MSVC, generador Visual Studio 18 2026, x64, Release y C++17.

```powershell
cmake -S native/build/mspa-review/source -B native/build/mspa-review/build -G "Visual Studio 18 2026" -A x64 -DMSPA_BUILD_TESTS=ON
cmake --build native/build/mspa-review/build --config Release --parallel 6
ctest --test-dir native/build/mspa-review/build -C Release --output-on-failure
```

Resultado: **3/3 pruebas oficiales aprobadas** (`mspa_base_tests`, `mspa_runtime_tests`, `mspa_api_surface_tests`). Esto verifica la suite existente; no certifica conformidad completa con la especificación.

El diagnóstico adicional está en `native/build/mspa-review/probes/review_probes.cpp`, separado de las fuentes originales. Su ejecutable, `native/build/mspa-review/probes-build/Release/mspa_review_probes.exe`, produjo:

```text
Opposite personalities receive identical candidate scores: 1
Observation missing required actor role accepted: 1
Second belief accepted with max_beliefs_per_agent=1: 1 (stored=2)
Time rebase (expected all=280): last=280, mood=90, emotion=90
```

Esas cuatro líneas reproducen carencias; no son cuatro pruebas de conformidad aprobadas. El snapshot y los diagnósticos viven en el directorio de compilación y se perderán si se limpia ese directorio.

## Integración con el código real de Isla Viva

| Responsabilidad | Código actual / destino |
| --- | --- |
| Estado corporal, posición, necesidades físicas, edad y tarea | `entities/Resident.h`, `ResidentRegistry`, `ResidentSimulation`; siguen perteneciendo al host. |
| Preferencias y estado psicológico | Runtime MSPA detrás de un servicio del adaptador, con un agente por habitante. Sustituye gradualmente la política de `ResidentDecision`. |
| Percepción individual | Adaptador que transforma hechos percibidos en `OBSERVE`; no entrega a cada agente todo el conocimiento de `World` o `ResidentContext`. |
| Opciones físicamente realizables | Extraer su descubrimiento de `ResidentController`: ruta, recurso, herramienta, destino y disponibilidad se comprueban antes de `DECIDE`. |
| Inicio y ejecución de acciones | `ResidentActions`, `ResidentSimulation`, `Fishing`, `Family`; ejecutan la opción elegida y conservan las reglas físicas del juego. |
| Resultados e interrupciones | Un punto común de notificación desde finalización, cancelación, muerte, viaje y traslado divino. Envía `REPORT` exactamente una vez por acción. |
| Semántica del juego | `IslaVivaDomainPack` en este repositorio: necesidades, acciones, relaciones, hechos percibidos e influencias. MSPA no debe conocer clases de Isla Viva. |
| Persistencia y viajeros | Codec versionado del adaptador en `Persistence` y `Visits`; estado psicológico separado de los campos físicos. |

El flujo será: hechos percibidos → OBSERVE → opciones viables → DECIDE → ejecución física → REPORT. La ejecución no debe elegir otra acción a escondidas cuando falla la elegida: debe comunicar la interrupción y volver a consultar.

### Restricciones que hay que resolver antes de conectar el runtime

- **Propiedad y copias:** `Environment.cpp` hace `World trial=*this` para validar cambios del terreno. Añadir directamente un `unique_ptr<Runtime>` rompería esas copias; compartir un runtime mutable haría que una simulación provisional pudiera modificar mentes reales. El dueño debe ser una sesión de simulación que coordine mundo y servicio psicológico, con una política explícita de snapshot/restauración. Los cambios provisionales no deben emitir eventos psicológicos hasta confirmarse.
- **Identidad:** `World.worldId` es texto; MSPA usa `WorldId` numérico y `AgentId` compuesto. Hace falta un mapeo persistido y validado. Los índices actuales de habitantes son estables porque el registro añade entradas y conserva fallecidos; el identificador local puede basarse en esa estabilidad con una regla documentada, sin confundir índices con identidades de viajeros.
- **Reloj:** usar ticks enteros del paso fijo de simulación, con dominio declarado; no el reloj de animación ni sumas flotantes convertidas a posteriori. La velocidad visual y el zoom no deben afectar las decisiones.
- **Cancelaciones:** hoy hay cambios directos a `Idle` en varios módulos. Deben pasar por una transición común que libere reservas y registre el resultado, incluidos los traslados de `MysteriousPower`.
- **Guardado:** `ExportedAgent` es una estructura C++ en memoria. Todavía no constituye un formato JSON versionado ni una migración de partidas. El adaptador debe serializar campos explícitos y preservar el RNG al restaurar una misma partida.
- **Viajes:** el protocolo actual de visitas tiene un límite de 16 KB. Debe comprobarse el tamaño real del estado psicológico, definir compatibilidad de packs, remapeo de identidades, reloj y política RNG. Al importar hay que pasar expresamente la semilla de destino cuando corresponda; no confiar en un valor predeterminado.
- **Influencia divina:** `Divinity` puede producir experiencias o influencias interpretables por el agente. Las mutaciones físicas, como moverlo, siguen siendo acciones del host y generan después las observaciones pertinentes.

## CMake y distribución

El [CMake revisado](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/CMakeLists.txt) publica **`MSPA::mspa`** y **`MSPA::base`**, ambos alias de una sola biblioteca estática. No publica `mspa::core`, `mspa_io` ni `mspa_conformance`. El ejemplo anterior con esos nombres no corresponde a esta versión.

La integración podrá usar `FetchContent` con commit fijado y `FETCHCONTENT_SOURCE_DIR_MSPA` durante desarrollo. El enlace pertenece al adaptador del host; MSPA conserva su independencia de `RA::Engine`, Windows y `isla_sim`. No hay dependencia JSON en la biblioteca revisada. Sus opciones incluyen `/fp:strict` para MSVC y `-ffp-contract=off` para otros compiladores.

No se identificó un archivo LICENSE en el árbol revisado; conviene resolver la licencia explícita antes de redistribuir la dependencia. Esto no determina por sí solo qué permisos existen fuera del repositorio.

## Secuencia de implementación propuesta

1. Corregir en MSPA las cuatro reproducciones y coordinar el recorrido de psicología, consideraciones y decisión. Añadir pruebas de conducta y conformance con escenarios, además de cobertura de métodos.
2. Definir el Domain Pack de Isla Viva y un primer escenario controlado: habitante cansado frente a pescar o descansar, con opciones viables y resultados reales. Verificar que sus necesidades, experiencias y diferencias personales cambian la preferencia de forma explicable.
3. Introducir el servicio psicológico y el catálogo de acciones del adaptador, conservando la política actual como respaldo explícito ante errores. Separar su estado del estado físico y de la presentación.
4. Integrar guardado/restauración, nacimientos y fallecimientos. Habilitar después viajeros y multijugador, una vez corregida y probada la transferencia temporal y de identidades.

El criterio de aceptación es conducta observable y persistente, con trazas que expliquen la elección; compilar y enlazar la biblioteca es solo una parte del trabajo.
