# Simulador Avanzado de Políticas de Reemplazo de Caché (Proyecto 4)
### Arquitectura del Computador — Semestre 2026

Simulador de memoria caché de alta fidelidad desarrollado en **C++17** para GNU/Linux, enfocado en la evaluación empírica y didáctica de las políticas de reemplazo **LRU**, **FIFO** y **LFU** (junto con la línea base **Random** y la cota teórica óptima de **Belady (OPT)**), con complejidad temporal **$O(1)$** por acceso y desalojo, clasificación formal de fallos según el **Modelo de las 3C de Mark D. Hill**, medición con reloj monotónico `std::chrono::steady_clock`, y una arquitectura modular libre de variables globales que integra las estructuras de datos de `librerias-lpc` (`cola.hpp`, `pila.hpp`, `nodo.hpp`).

---

## 1. Fundamentos Teóricos y Dilema del Hardware

### 1.1 Jerarquía de Memoria (SRAM vs DRAM)
La brecha de velocidad entre la CPU y la memoria principal (DRAM) representa el cuello de botella fundamental de la arquitectura de computadores (el *Memory Wall*). La memoria caché, implementada en tecnología SRAM (Static RAM), proporciona un acceso ultrarrápido (~1 ciclo de reloj) pero con capacidad reducida debido a su alto costo por bit y disipación de potencia. Cuando la CPU solicita una dirección que no está en la caché (**Cache Miss**), el procesador debe detenerse (**CPU Stall**) esperando la transferencia del bloque desde la memoria DRAM lenta (~100 ciclos de penalización).

```
   +-------------+        +--------------------------+        +----------------------+
   |     CPU     | <----> |    CACHE SRAM (Rápida)   | <----> |  DRAM (Memoria Ppal) |
   |             |        |    Latencia: 1 ciclo     |        |  Latencia: 100 ciclos|
   | (Ejecución) |        |    Capacidad: 32 KB      |        |  Capacidad: Gigabytes|
   +-------------+        +--------------------------+        +----------------------+
                                       ^
                            MMU / Cache Controller
                   (Decide qué bloque conservar y cuál desalojar)
```

### 1.2 Lógicas de Decisión de Reemplazo
Al llenarse un conjunto de la caché, la MMU debe seleccionar un **bloque víctima** para ser expulsado:

1. **FIFO (First-In, First-Out)**:
   - Expulsa el bloque que ingresó primero cronológicamente al conjunto.
   - Implementado mediante una cola secuencial (`Cola<int>` de `librerias-lpc/cola.hpp`).
   - Los aciertos **no** alteran el orden de llegada.
   - **Limitación**: Ignora la frecuencia y recencia de uso. Es susceptible a la **Anomalía de Belady** (incrementar el número de vías/marcos puede empeorar la tasa de fallos).

2. **LRU (Least Recently Used)**:
   - Se fundamenta en el **Principio de Localidad Temporal**: los bloques usados recientemente tienen mayor probabilidad de ser usados en el futuro cercano.
   - Expulsa el bloque que lleva más tiempo sin registrar accesos.
   - Cada acierto o inserción mueve el bloque al frente de recencia ($MRU$).

3. **LFU (Least Frequently Used)**:
   - Monitorea la frecuencia acumulada de accesos de cada bloque.
   - Expulsa el bloque con la menor cantidad de referencias acumuladas.
   - Desempata bloques de igual frecuencia utilizando el criterio LRU.
   - **Comportamiento**: Excelente en cargas sesgadas (distribuciones Zipf / Pareto / Hot-Cold), aunque susceptible a la inercia de frecuencia (bloques viejos con frecuencia alta que dejan de usarse).

4. **Random (Aleatoria)**:
   - Selecciona pseudoaleatoriamente una vía del conjunto como víctima. Sirve como línea base de comparación estocástica.

5. **OPT / MIN (Algoritmo Óptimo de Belady - Offline)**:
   - Desaloja el bloque cuyo próximo acceso en la traza se encuentra más alejado en el futuro.
   - Representa la cota superior teórica inalcanzable por algoritmos en línea (*online*).

---

## 2. Clasificación de Fallos: Modelo de las 3C de Mark D. Hill

A diferencia de los simuladores básicos que solo reportan fallos globales, este simulador implementa formalmente la taxonomía científica de Mark D. Hill (1987) utilizando una **caché sombra totalmente asociativa**:

$$\text{Total de Fallos} = \text{Cold} + \text{Capacity} + \text{Conflict}$$

- **Cold (Compulsorio / Forzoso)**: Ocurre en la primera referencia absoluta que el procesador hace a un bloque de memoria en toda la traza. Inevitable aun con una memoria caché de tamaño infinito.
- **Capacity (Capacidad)**: Ocurre cuando el bloque ya fue referenciado previamente, pero fue expulsado porque el conjunto de trabajo activo (*working set*) del programa excede la capacidad total de la caché física. Ocurriría incluso en una caché totalmente asociativa del mismo tamaño.
- **Conflict (Conflicto / Colisión)**: Ocurre cuando el bloque ya fue referenciado y cabría en una caché totalmente asociativa del mismo tamaño, pero fue expulsado porque múltiples direcciones colisionaron en las vías de un mismo conjunto específico. En una caché totalmente asociativa, este valor es exactamente 0.

### Métricas de Rendimiento
- **Hit Ratio**: $\frac{\text{Aciertos}}{\text{Accesos Totales}}$
- **Miss Ratio**: $1 - \text{Hit Ratio}$
- **AMAT (Average Memory Access Time)**: $\text{Latencia Hit} + (\text{Miss Ratio} \times \text{Penalización DRAM})$
- **Ciclos de CPU Stall**: $\text{Fallos} \times \text{Penalización DRAM}$

---

## 3. Arquitectura del Software y Complejidad $O(1)$

El diseño del simulador sigue el **Patrón de Diseño Strategy**, donde el controlador de la caché desacopla el motor de simulación de las políticas de reemplazo:

```
                  +-----------------------+
                  |   PoliticaReemplazo   |  <<Interface Strategy>>
                  +-----------------------+
                  | +registrarAcierto()   |
                  | +registrarInsercion() |
                  | +seleccionarVictima() |
                  +-----------------------+
                       ^      ^      ^
           +-----------+      |      +-----------+
           |                  |                  |
+--------------------+ +--------------------+ +--------------------+
|    PoliticaLRU     | |    PoliticaFIFO    | |    PoliticaLFU     |
| std::list + vector | |    Cola<int>       | | Cubetas 2D + minFreq|
| Acceso: O(1)       | | Acceso: O(1)       | | Acceso: O(1)       |
| Desalojo: O(1)     | | Desalojo: O(1)     | | Desalojo: O(1)     |
+--------------------+ +--------------------+ +--------------------+
```

### 3.1 Estructuras de Datos y Complejidad Temporal
| Dimensión Técnica | LRU | FIFO | LFU |
|---|---|---|---|
| **Complejidad de Acceso** | **$O(1)$** | **$O(1)$** | **$O(1)$** |
| **Complejidad de Desalojo** | **$O(1)$** | **$O(1)$** | **$O(1)$** |
| **Estructura Interna** | Lista doblemente enlazada + iteradores indexados | `Cola<int>` de orden de llegada | Estructura bidimensional de cubetas de frecuencia + `minFreq` |
| **Overhead de Memoria** | Moderado (punteros anterior/siguiente) | Bajo (cola lineal sobre nodos) | Alto (tabla hash de cubetas enlazadas) |
| **Sensibilidad Secuencial** | Alta degradación por barrido | Vulnerable a anomalía de Belady | Robusta (frecuencia inicial $F=1$) |
| **Respuesta a Bucles** | Óptima si el bucle cabe | Pobre (expulsión prematura) | Inercia de frecuencia |

### 3.2 Integración de las Librerías de Estructuras de Datos (`librerias-lpc`)
El proyecto reutiliza e integra directamente las librerías desarrolladas en `librerias-lpc`:
- **`Cola<int>` (`cola.hpp`)**: Empleada directamente en `PoliticaFIFO` para registrar el orden de inserción de las vías y seleccionar la víctima en el frente de la cola (`getFirst()`, `desencolar()`).
- **`Pila<int>` (`pila.hpp`)**: Empleada en cada conjunto de la clase `Cache` como depósito (*Free List*) de vías libres disponibles (`apilar()`, `desapilar()`). Al inicializar un conjunto, las vías libres se apilan; cuando ocurre un miss y hay vías libres, se extrae una vía sin desalojo en $O(1)$.
- **Estilo de código**: Se preservó la filosofía del autor: uso explícito de `this->`, modularidad estricta, métodos `camelCase`, gestión manual y limpia de punteros, y cero variables globales mutables.

### 3.3 Benchmarking Confiable y Monotonicidad
- **`std::chrono::steady_clock`**: Utilizado para medir tiempos de ejecución de forma estrictamente monotónica (inmune a saltos de tiempo del sistema operativo o ajustes NTP).
- **Fase de Calentamiento (Warm-up)**: Permite ejecutar una fracción inicial de la traza para precalentar las líneas de caché y estabilizar el estado antes de registrar métricas.
- **Barrera del Compilador**: Implementación de `SimuladorCache::noOptimizar(...)` mediante ensamblador en línea (`asm volatile`) para impedir que el optimizador del compilador descarte los cálculos críticos.

---

## 4. Estructura de Directorios

```
codigo/
├── Makefile                        # Automatización de compilación y ejecución
├── README.md                       # Documentación técnica completa
├── results_v2.csv                  # Resultados experimentales exportados
├── include/                        # Headers modulares
│   ├── tipos.hpp                   # Direcciones, accesos, lecturas/escrituras
│   ├── config_cache.hpp            # ConfiguracionCache, EstadisticasCache, 3C
│   ├── clasificador_3c.hpp         # Clasificador 3C formal de Hill
│   ├── politica_reemplazo.hpp      # Interfaz Strategy, LRU, FIFO, LFU, OPT, Random
│   ├── cache.hpp                   # Controlador de caché asociativa (Write-Back)
│   ├── generador_trazas.hpp        # Generador de cargas sintéticas, aleatorias y E/S
│   ├── simulador.hpp               # Motor de simulación, warm-up, DoNotOptimize
│   ├── modo_didactico.hpp          # Visualizador interactivo paso a paso y hardware
│   ├── reporteador.hpp             # Tablas ASCII, gráficos de barras y exportador CSV
│   └── cli.hpp                     # Parser de comandos sin variables globales
├── src/                            # Implementación de los módulos
│   ├── cache.cpp
│   ├── clasificador_3c.cpp
│   ├── cli.cpp
│   ├── config_cache.cpp
│   ├── generador_trazas.cpp
│   ├── main.cpp
│   ├── modo_didactico.cpp
│   ├── politica_reemplazo.cpp
│   ├── reporteador.cpp
│   └── simulador.cpp
├── librerias-lpc/                  # Librerías de estructuras de datos reutilizadas
│   ├── cola.hpp
│   ├── lista.hpp
│   ├── nodo.hpp
│   └── pila.hpp
├── tests/                          # Suite de pruebas unitarias
│   └── test_simulador.cpp
└── trazas/                         # Trazas de memoria de prueba
    ├── anomalia_belady.trace
    └── ejemplo_simple.trace
```

---

## 5. Compilación y Ejecución

El proyecto está diseñado para ser compilado con `g++` bajo el estándar **C++17** en cualquier distribución GNU/Linux.

### 5.1 Comandos Básicos de Makefile
```bash
cd codigo

# Compilar el binario principal (build/cachesim)
make

# Ejecutar la suite completa de pruebas unitarias (12 pruebas)
make test

# Ejecutar la comparativa completa de todas las cargas y generar results_v2.csv
make run

# Ejecutar demostración rápida con distribución Zipf
make demo

# Ejecutar demostración en Modo Didáctico paso a paso
make didactico

# Ejecutar barrido de asociatividad (Sweep de Vías)
make sweep

# Mostrar ayuda completa de comandos
make ayuda

# Limpiar archivos compilados
make clean
```

---

## 6. Guía de Uso del Simulador (`./build/cachesim`)

### 6.1 Comandos Principales
```bash
./build/cachesim <comando> [opciones]
```

1. **`compare` (Modo Normal)**:
   Evalúa y compara las políticas sobre una o todas las cargas de trabajo, generando tablas con Hit Ratio, clasificación 3C, AMAT, Stalls, tiempo en milisegundos y barras visuales.
   ```bash
   ./build/cachesim compare --workload zipf
   ./build/cachesim compare --policies lru,fifo,lfu --include-opt --csv out.csv
   ```

2. **`didactico` (Modo Explicativo y Didáctico)**:
   Explica en detalle cada acceso:
   - Descomposición de la dirección en **Tag**, **Índice de Conjunto** y **Offset**.
   - Diagrama conceptual de la jerarquía SRAM / DRAM y rol de la MMU.
   - Estado del conjunto antes y después del acceso.
   - Explicación de por qué ocurrió HIT o MISS.
   - Clasificación 3C pedagógica (Cold, Capacity, Conflict).
   - Justificación de la víctima desalojada según la política activa.
   - Opción interactiva para avanzar paso a paso presionando `[Enter]`.
   ```bash
   ./build/cachesim didactico --workload bucle_thrashing --pasos 15
   ./build/cachesim didactico --interactivo --pasos 10
   ./build/cachesim didactico --file trazas/ejemplo_simple.trace --size 256 --block 64 --ways 2
   ```

3. **`sweep` (Barrido Experimental de Parámetros)**:
   Varia sistemáticamente un parámetro de hardware para estudiar su curva de comportamiento.
   - `--param size`: varía el tamaño de la caché (1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB).
   - `--param ways`: varía el grado de asociatividad (1=Mapeo directo, 2, 4, 8, 16, 0=Totalmente asociativa).
   - `--param block`: varía el tamaño de bloque (16B, 32B, 64B, 128B, 256B).
   ```bash
   ./build/cachesim sweep --param ways --workload caliente_frio
   ./build/cachesim sweep --param size --workload zipf
   ```

4. **`traza` (Simulación de Trazas Reales desde Archivo)**:
   Lee archivos de trazas de memoria con formato estándar:
   ```
   # Formato: [R|W] 0xDIRECCION
   R 0x00401000
   W 0x00401040
   ```
   ```bash
   ./build/cachesim traza --file trazas/ejemplo_simple.trace
   ```

5. **`generar`**:
   Genera una traza sintética o aleatoria y la almacena en disco.
   ```bash
   ./build/cachesim generar --workload matriz --length 50000 --out matriz.trace
   ```

6. **`listar`**:
   Muestra el catálogo completo de políticas y cargas de trabajo soportadas.
   ```bash
   ./build/cachesim listar
   ```

### 6.2 Opciones de Configuración
| Opción | Descripción | Valor por Defecto |
|---|---|---|
| `--size <tam>` | Tamaño total de la caché (ej: 16K, 32K, 64K, 1M) | `32K` |
| `--block <tam>` | Tamaño de línea/bloque en bytes (ej: 32, 64, 128) | `64` |
| `--ways <n>` | Vías de asociatividad (0 = Total, 1 = Directa, N) | `4` |
| `--policies <p>` | Políticas a evaluar separadas por coma | `lru,fifo,lfu` |
| `--include-opt` | Incluye la cota óptima de Belady en la comparación | `false` |
| `--workload <w>` | Nombre de la carga o `all` para todas | `all` |
| `--length <n>` | Cantidad de accesos generados por carga | `100000` |
| `--warmup <p>` | Fracción de accesos para estabilización inicial | `0.0` |
| `--seed <n>` | Semilla para el generador pseudoaleatorio | `42` |
| `--repeats <n>` | Repeticiones para medición de tiempo con steady_clock | `3` |
| `--hit-latency <c>` | Latencia de acierto en ciclos de reloj | `1.0` |
| `--miss-penalty <c>` | Penalización por fallo (DRAM Stall) en ciclos | `100.0` |
| `--pasos <n>` | Límite de accesos mostrados en modo didáctico | `20` |
| `--interactivo` | Modo interactivo con pausa por tecla `[Enter]` | `false` |
| `--csv <ruta>` | Exportar resultados consolidados a archivo CSV | *(desactivado)* |

---

## 7. Catálogo de Cargas de Trabajo (Sintéticas, Aleatorias y Reales)

El simulador implementa los tres tipos de datos solicitados en las especificaciones del curso:

### Datos Sintéticos
1. **`secuencial`**: Recorrido streaming de $8\times$ el tamaño de la caché sin reutilización temporal.
2. **`bucle_ajustado`**: Bucle iterativo sobre $0.5\times$ la capacidad de la caché. Una vez completado el llenado compulsorio (*cold*), todas las políticas logran un 100% de aciertos.
3. **`bucle_thrashing`**: Bucle sobre $1.25\times$ la capacidad de la caché. Escenario patológico donde LRU y FIFO desalojan sistemáticamente los bloques antes de su reutilización, provocando un 0% de aciertos (*thrashing*).
4. **`zancada` (Stride)**: Accesos a intervalos regulares de memoria (saltos de 128 bytes), modelando recorridos por columnas en matrices o acceso a campos específicos de estructuras de datos.
5. **`matriz`**: Recorrido bidimensional combinado por filas y columnas simulando multiplicación de matrices.
6. **`fases`**: Modela cambios de contexto y fases de ejecución alternando entre tres regiones de memoria distintas.

### Datos Aleatorios
7. **`uniforme`**: Accesos distribuidos uniformemente sobre $4\times$ el tamaño de la caché; evalúa el comportamiento sin localidad temporal.
8. **`gaussiana`**: Distribución normal centrada con dispersión controlada ($\sigma = 0.5\times$ caché), generando localidad temporal y espacial suave.
9. **`zipf`**: Distribución de ley de potencias ($\alpha = 1.0$), donde un subconjunto muy reducido de bloques concentra la inmensa mayoría de las referencias. **Escenario predilecto donde LFU supera ampliamente a LRU y FIFO**.
10. **`caliente_frio` (80/20)**: Modela el Principio de Pareto: el 80% de las referencias se concentran en una región caliente de $0.5\times$ caché, y el 20% restante en una región fría extensa.

### Datos Reales
11. **Trazas de Archivo**: Soporte para cargar secuencias reales de lecturas y escrituras extraídas de instrumentación de programas o benchmarks.

---

## 8. Resultados Experimentales y Análisis Comparativo

A continuación se resumen los resultados obtenidos con la configuración estándar (**Caché de 32 KB, Bloque de 64 B, 4 Vías, 100.000 accesos** por carga):

| Carga de Trabajo | Tipo | LRU Hit% | FIFO Hit% | LFU Hit% | Política Ganadora | Justificación Teórica |
|---|---|---|---|---|---|---|
| **`secuencial`** | Sintético | 87.50% | 87.50% | 87.50% | **Empate** | Los aciertos provienen de la localidad espacial dentro del bloque de 64B ($8$ accesos por línea). |
| **`bucle_ajustado`** | Sintético | 99.74% | 99.74% | 99.74% | **Empate** | El conjunto de trabajo cabe íntegramente en la memoria caché SRAM. Solo ocurren fallos Cold. |
| **`bucle_thrashing`** | Sintético | 0.00% | 0.00% | 0.00% | **Empate** | Patología clásica de bucle mayor que la capacidad: el bloque a reutilizar fue desalojado instantes antes. |
| **`matriz`** | Sintético | 44.29% | 44.29% | **46.45%** | **LFU** | Los elementos compartidos por productos internos acumulan frecuencia y son retenidos en la caché. |
| **`fases`** | Sintético | **97.95%** | **97.95%** | 86.86% | **LRU / FIFO** | LRU se adapta velozmente al cambio de fase. LFU sufre de **inercia de frecuencia** reteniendo bloques de la fase previa. |
| **`uniforme`** | Aleatorio | 24.88% | 24.86% | **24.94%** | **LFU** | En ausencia de localidad, todas convergen hacia la probabilidad teórica de ocupación. |
| **`gaussiana`** | Aleatorio | 53.47% | 51.59% | **58.94%** | **LFU** | Los bloques cercanos a la media se acumulan con mayor frecuencia y se protegen contra desalojos. |
| **`zipf`** | Aleatorio | 58.89% | 55.35% | **65.29%** | **LFU** | La asimetría extrema de popularidad beneficia directamente a LFU, reduciendo el AMAT de 42.11c a 35.71c. |
| **`caliente_frio`** | Aleatorio | 77.50% | 69.38% | **80.11%** | **LFU** | FIFO sufre expulsión prematura de bloques calientes debido a llegadas de la región fría. LFU protege la región caliente. |

---

## 9. Propuesta de Mejora al Recurso de Ayuda (Infografía)

De acuerdo con el enunciado de `Proyectos 2026.pdf`, se evaluaron las infografías del repositorio del curso (`Proyecto_4_teoria.png` y `Proyecto_4_practica.png`).

### Diagnóstico del Recurso Actual:
- **Aspectos Fuertes**: Excelente conceptualización de la dualidad SRAM/DRAM, clara esquematización del patrón Strategy y explicación de las complejidades $O(1)$.
- **Oportunidades de Mejora**:
  1. Falta detallar gráficamente el desempate por LRU dentro de las cubetas de frecuencia de LFU.
  2. Falta ilustrar el fenómeno de la **Anomalía de Belady** en FIFO mediante una curva comparativa de vías vs fallos.
  3. No se visualizaba el mecanismo de **Write-Back** con líneas sucias (*dirty bit*) al interactuar con DRAM.

### Prompt de Mejora Propuesto para Generación de Nuevos Recursos Didácticos:
```text
Crea una infografía técnica y académica de dos paneles para estudiantes de Arquitectura del Computador titulada:
"Simulación y Optimización de Políticas de Reemplazo en Memoria Caché: LRU, FIFO y LFU".

Panel 1: "Mecanismos Internos y Jerarquía de Hardware"
- Esquema de descomposicion de direcciones de memoria en hardware: Tag (etiqueta), Set Index (índice de conjunto) y Block Offset (desplazamiento).
- Diagrama de flujo de la MMU ante un acceso: comprobación de etiqueta -> Hit (acceso en 1 ciclo SRAM) vs Miss (parada CPU Stall y penalización de 100 ciclos DRAM).
- Demostración visual de la política Write-Back: marcado de 'Dirty Bit' en escrituras y transferencia diferida a memoria al momento del desalojo de la víctima.
- Ilustración de las 3 estructuras de datos en O(1):
  * LRU: Lista doblemente enlazada con reordenamiento al frente (MRU).
  * FIFO: Cola lineal de orden de llegada estricto.
  * LFU: Estructura bidimensional de cubetas de frecuencias (Ketan Shah) con desempate LRU horizontal.

Panel 2: "Taxonomía de Fallos y Análisis Comparativo"
- Explicación gráfica de las 3C de Mark D. Hill con diagramas de Venn/conjuntos:
  * Cold (Compulsorio): Primer contacto inevitable.
  * Capacity: Conjunto de trabajo superior al tamaño de la caché física.
  * Conflict: Colisión en las vías de un conjunto particular.
- Curva comparativa de rendimiento: demostración gráfica de la Anomalía de Belady en FIFO (aumento de fallos con 4 vías respecto a 3 vías) frente a la monotonicidad estricta de LRU y OPT.
- Fórmulas de AMAT (Average Memory Access Time) y cálculo de ciclos de CPU Stall con barras comparativas para cargas Zipf, Gaussianas y Secuenciales.
- Paleta de colores sobria y profesional (azul marino, grafito, toques en ámbar y verde esmeralda), con tipografía técnica legible y diagramas vectoriales claros.
```

---

## 10. Conclusiones

1. **Eficiencia Algorítmica**: La implementación de las tres políticas con complejidad temporal $O(1)$ garantiza tiempos de simulación inferiores a 60 milisegundos para 100.000 accesos en entornos modernos.
2. **Reutilización y Modularidad**: El aprovechamiento de `Cola<int>` para FIFO y `Pila<int>` para la gestión de vías libres demuestra la flexibilidad de las estructuras de datos clásicas de `librerias-lpc` en la simulación de componentes de arquitectura de procesadores.
3. **Rigurosidad Didáctica y Científica**: La inclusión del **Modo Didáctico** interactivo y la separación formal de las 3C de Mark D. Hill transforma el simulador en una herramienta pedagógica completa, ideal para laboratorios prácticos y exposiciones académicas.
