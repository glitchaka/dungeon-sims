# RevisiÃ³n de MSPA para los habitantes de Isla Viva

RevisiÃ³n del 22 de octubre de 2026. Snapshot examinado: [`decde8c10fa69de0b289521ee83ab416c4b68da4`](https://github.com/glitchaka/MSPA-Lib/tree/decde8c10fa69de0b289521ee83ab416c4b68da4). Se revisaron cÃ³digo, README, especificaciÃ³n 2.4, contrato de interacciÃ³n 0.2, contrato de Domain Pack 0.1 y arquitectura 0.1.

## ConclusiÃ³n

La base es compatible con Isla Viva: C++17, biblioteca independiente del host, identificadores explÃ­citos, reloj psicolÃ³gico, registro semÃ¡ntico y API de observaciÃ³n, decisiÃ³n, resultados y transferencia. Su implementaciÃ³n actual es provisional, como reconoce el README. TodavÃ­a no cumple el comportamiento psicolÃ³gico descrito en los contratos.

Antes de usarla como polÃ­tica principal hay que conectar la psicologÃ­a con la decisiÃ³n y corregir validaciÃ³n, cuotas y conversiÃ³n temporal. Enchufar ahora `decide()` no harÃ­a que hambre, personalidad, relaciones o aprendizaje cambiaran las preferencias por sÃ­ mismos.

Esta revisiÃ³n no activa MSPA en el juego ni modifica su comportamiento.

## Hallazgos prioritarios

### 1. La decisiÃ³n no utiliza el estado psicolÃ³gico

[`UtilityComposer::score`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/decision/decision.cpp) recibe Ãºnicamente `ActionOption`. Calcula coste negativo, riesgo negativo y progreso acotado; no recibe `AgentPsychState`. [`Runtime::decide`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/runtime/runtime.cpp) aÃ±ade 0,01 por continuidad y elige el primer mÃ¡ximo. La polÃ­tica de histÃ©resis y el registro de consideraciones existen por separado, pero este recorrido no los utiliza.

ReproducciÃ³n: dos agentes con audacia y sociabilidad opuestas obtuvieron puntuaciones idÃ©nticas para las mismas opciones. El alcance general del problema se confirma ademÃ¡s leyendo la firma y el cÃ¡lculo del compositor.

Consecuencia: incluir personalidad, emociones y recuerdos en estructuras no basta para producir conducta. Hay que incorporar sus consideraciones al recorrido pÃºblico de decisiÃ³n, con pruebas que demuestren diferencias explicables en situaciones controladas.

### 2. OBSERVE permite entradas incompletas y crecimiento fuera de cuota

[`PsychologyEngine::observe`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/psychology/engine.cpp) no comprueba los roles requeridos por el predicado antes de aÃ±adir una creencia. La comprobaciÃ³n de lÃ­mites en creaciÃ³n/importaciÃ³n no protege ese crecimiento durante la observaciÃ³n.

Reproducciones independientes dentro del mismo programa de diagnÃ³stico:

- Un predicado con rol obligatorio `actor` aceptÃ³ una proposiciÃ³n sin argumentos.
- Con `max_beliefs_per_agent=1`, una segunda observaciÃ³n fue aceptada y quedaron dos creencias.

Hay que validar la proposiciÃ³n y aplicar una polÃ­tica explÃ­cita de rechazo, consolidaciÃ³n o descarte antes de modificar el estado. Las observaciones repetidas tampoco deberÃ­an convertirse sin lÃ­mite en entradas duplicadas.

### 3. La migraciÃ³n temporal deja marcas sin convertir

[`import_agent_state`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/persistence/state_transfer.cpp) rebasa el Ãºltimo procesamiento, las creencias y los recuerdos. Omite `mood.updated_at` y `emotions[].started_at`.

ReproducciÃ³n: exportaciÃ³n en tick 100 con marcas en 90, reloj origen de 1 tick/segundo; importaciÃ³n en tick 300 con destino de 2 ticks/segundo. Las tres marcas deberÃ­an quedar en 280. Se obtuvo `last=280, mood=90, emotion=90`.

Debe corregirse antes de transferir viajeros entre islas: las duraciones psicolÃ³gicas quedarÃ­an alteradas.

### 4. Los mÃ³dulos avanzados todavÃ­a no forman un ciclo psicolÃ³gico completo

Por inspecciÃ³n de [`runtime.cpp`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/runtime/runtime.cpp), `advance_to()` actualiza la marca temporal sin ejecutar el decaimiento de emociones o Ã¡nimo. `report()` mantiene estadÃ­sticas por tipo de acciÃ³n sin contexto del resultado; su entrada tampoco identifica la instancia de acciÃ³n ni distingue una interrupciÃ³n. El motor de observaciÃ³n aÃ±ade creencias mediante una valoraciÃ³n mÃ­nima y no coordina todos los sistemas psicolÃ³gicos independientes.

Por inspecciÃ³n de [`domain_pack.cpp`](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/src/semantics/domain_pack.cpp), el pack contiene reglas de inferencia, pero su aplicaciÃ³n registra predicados, conceptos y acciones sin incorporar esas reglas al recorrido de ejecuciÃ³n.

Son diferencias entre el contrato y lo implementado; no afirmaciones de que hayan fallado pruebas oficiales de esas capacidades.

## CompilaciÃ³n y comprobaciÃ³n realizadas

Se descargÃ³ el snapshot fijado a `native/build/mspa-review/source`, sin cambiar sus fuentes. Se compilÃ³ con MSVC, generador Visual Studio 18 2026, x64, Release y C++17.

```powershell
cmake -S native/build/mspa-review/source -B native/build/mspa-review/build -G "Visual Studio 18 2026" -A x64 -DMSPA_BUILD_TESTS=ON
cmake --build native/build/mspa-review/build --config Release --parallel 6
ctest --test-dir native/build/mspa-review/build -C Release --output-on-failure
```

Resultado: **3/3 pruebas oficiales aprobadas** (`mspa_base_tests`, `mspa_runtime_tests`, `mspa_api_surface_tests`). Esto verifica la suite existente; no certifica conformidad completa con la especificaciÃ³n.

El diagnÃ³stico adicional estÃ¡ en `native/build/mspa-review/probes/review_probes.cpp`, separado de las fuentes originales. Su ejecutable, `native/build/mspa-review/probes-build/Release/mspa_review_probes.exe`, produjo:

```text
Opposite personalities receive identical candidate scores: 1
Observation missing required actor role accepted: 1
Second belief accepted with max_beliefs_per_agent=1: 1 (stored=2)
Time rebase (expected all=280): last=280, mood=90, emotion=90
```

Esas cuatro lÃ­neas reproducen carencias; no son cuatro pruebas de conformidad aprobadas. El snapshot y los diagnÃ³sticos viven en el directorio de compilaciÃ³n y se perderÃ¡n si se limpia ese directorio.

## IntegraciÃ³n con el cÃ³digo real de Isla Viva

| Responsabilidad | CÃ³digo actual / destino |
| --- | --- |
| Estado corporal, posiciÃ³n, necesidades fÃ­sicas, edad y tarea | `entities/Resident.h`, `ResidentRegistry`, `ResidentSimulation`; siguen perteneciendo al host. |
| Preferencias y estado psicolÃ³gico | Runtime MSPA detrÃ¡s de un servicio del adaptador, con un agente por habitante. Sustituye gradualmente la polÃ­tica de `ResidentDecision`. |
| PercepciÃ³n individual | Adaptador que transforma hechos percibidos en `OBSERVE`; no entrega a cada agente todo el conocimiento de `World` o `ResidentContext`. |
| Opciones fÃ­sicamente realizables | Extraer su descubrimiento de `ResidentController`: ruta, recurso, herramienta, destino y disponibilidad se comprueban antes de `DECIDE`. |
| Inicio y ejecuciÃ³n de acciones | `ResidentActions`, `ResidentSimulation`, `Fishing`, `Family`; ejecutan la opciÃ³n elegida y conservan las reglas fÃ­sicas del juego. |
| Resultados e interrupciones | Un punto comÃºn de notificaciÃ³n desde finalizaciÃ³n, cancelaciÃ³n, muerte, viaje y traslado divino. EnvÃ­a `REPORT` exactamente una vez por acciÃ³n. |
| SemÃ¡ntica del juego | `IslaVivaDomainPack` en este repositorio: necesidades, acciones, relaciones, hechos percibidos e influencias. MSPA no debe conocer clases de Isla Viva. |
| Persistencia y viajeros | Codec versionado del adaptador en `Persistence` y `Visits`; estado psicolÃ³gico separado de los campos fÃ­sicos. |

El flujo serÃ¡: hechos percibidos â†’ OBSERVE â†’ opciones viables â†’ DECIDE â†’ ejecuciÃ³n fÃ­sica â†’ REPORT. La ejecuciÃ³n no debe elegir otra acciÃ³n a escondidas cuando falla la elegida: debe comunicar la interrupciÃ³n y volver a consultar.

### Restricciones que hay que resolver antes de conectar el runtime

- **Propiedad y copias:** `Environment.cpp` hace `World trial=*this` para validar cambios del terreno. AÃ±adir directamente un `unique_ptr<Runtime>` romperÃ­a esas copias; compartir un runtime mutable harÃ­a que una simulaciÃ³n provisional pudiera modificar mentes reales. El dueÃ±o debe ser una sesiÃ³n de simulaciÃ³n que coordine mundo y servicio psicolÃ³gico, con una polÃ­tica explÃ­cita de snapshot/restauraciÃ³n. Los cambios provisionales no deben emitir eventos psicolÃ³gicos hasta confirmarse.
- **Identidad:** `World.worldId` es texto; MSPA usa `WorldId` numÃ©rico y `AgentId` compuesto. Hace falta un mapeo persistido y validado. Los Ã­ndices actuales de habitantes son estables porque el registro aÃ±ade entradas y conserva fallecidos; el identificador local puede basarse en esa estabilidad con una regla documentada, sin confundir Ã­ndices con identidades de viajeros.
- **Reloj:** usar ticks enteros del paso fijo de simulaciÃ³n, con dominio declarado; no el reloj de animaciÃ³n ni sumas flotantes convertidas a posteriori. La velocidad visual y el zoom no deben afectar las decisiones.
- **Cancelaciones:** hoy hay cambios directos a `Idle` en varios mÃ³dulos. Deben pasar por una transiciÃ³n comÃºn que libere reservas y registre el resultado, incluidos los traslados de `MysteriousPower`.
- **Guardado:** `ExportedAgent` es una estructura C++ en memoria. TodavÃ­a no constituye un formato JSON versionado ni una migraciÃ³n de partidas. El adaptador debe serializar campos explÃ­citos y preservar el RNG al restaurar una misma partida.
- **Viajes:** el protocolo actual de visitas tiene un lÃ­mite de 16 KB. Debe comprobarse el tamaÃ±o real del estado psicolÃ³gico, definir compatibilidad de packs, remapeo de identidades, reloj y polÃ­tica RNG. Al importar hay que pasar expresamente la semilla de destino cuando corresponda; no confiar en un valor predeterminado.
- **Influencia divina:** `Divinity` puede producir experiencias o influencias interpretables por el agente. Las mutaciones fÃ­sicas, como moverlo, siguen siendo acciones del host y generan despuÃ©s las observaciones pertinentes.

## CMake y distribuciÃ³n

El [CMake revisado](https://github.com/glitchaka/MSPA-Lib/blob/decde8c10fa69de0b289521ee83ab416c4b68da4/CMakeLists.txt) publica **`MSPA::mspa`** y **`MSPA::base`**, ambos alias de una sola biblioteca estÃ¡tica. No publica `mspa::core`, `mspa_io` ni `mspa_conformance`. El ejemplo anterior con esos nombres no corresponde a esta versiÃ³n.

La integraciÃ³n podrÃ¡ usar `FetchContent` con commit fijado y `FETCHCONTENT_SOURCE_DIR_MSPA` durante desarrollo. El enlace pertenece al adaptador del host; MSPA conserva su independencia de `RA::Engine`, Windows y `isla_sim`. No hay dependencia JSON en la biblioteca revisada. Sus opciones incluyen `/fp:strict` para MSVC y `-ffp-contract=off` para otros compiladores.

No se identificÃ³ un archivo LICENSE en el Ã¡rbol revisado; conviene resolver la licencia explÃ­cita antes de redistribuir la dependencia. Esto no determina por sÃ­ solo quÃ© permisos existen fuera del repositorio.

## Secuencia de implementaciÃ³n propuesta

1. Corregir en MSPA las cuatro reproducciones y coordinar el recorrido de psicologÃ­a, consideraciones y decisiÃ³n. AÃ±adir pruebas de conducta y conformance con escenarios, ademÃ¡s de cobertura de mÃ©todos.
2. Definir el Domain Pack de Isla Viva y un primer escenario controlado: habitante cansado frente a pescar o descansar, con opciones viables y resultados reales. Verificar que sus necesidades, experiencias y diferencias personales cambian la preferencia de forma explicable.
3. Introducir el servicio psicolÃ³gico y el catÃ¡logo de acciones del adaptador, conservando la polÃ­tica actual como respaldo explÃ­cito ante errores. Separar su estado del estado fÃ­sico y de la presentaciÃ³n.
4. Integrar guardado/restauraciÃ³n, nacimientos y fallecimientos. Habilitar despuÃ©s viajeros y multijugador, una vez corregida y probada la transferencia temporal y de identidades.

El criterio de aceptaciÃ³n es conducta observable y persistente, con trazas que expliquen la elecciÃ³n; compilar y enlazar la biblioteca es solo una parte del trabajo.

