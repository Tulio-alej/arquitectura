#include "cli.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "reporteador.hpp"
#include "modo_didactico.hpp"

namespace cachesim {

namespace {

std::vector<std::string> dividirCadena(const std::string &s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) {
            tokens.push_back(item);
        }
    }
    return tokens;
}

bool parsearTamanoBytes(const std::string &s, std::size_t &salida) {
    if (s.empty()) return false;
    std::size_t multiplicador = 1;
    std::string num = s;

    char sufijo = std::toupper(static_cast<unsigned char>(num.back()));
    if (sufijo == 'K') {
        multiplicador = 1024;
        num.pop_back();
    } else if (sufijo == 'M') {
        multiplicador = 1024 * 1024;
        num.pop_back();
    } else if (sufijo == 'G') {
        multiplicador = 1024ULL * 1024ULL * 1024ULL;
        num.pop_back();
    }

    try {
        salida = std::stoull(num) * multiplicador;
        return true;
    } catch (...) {
        return false;
    }
}

// Subcomandos internos
int cmdCompare(const OpcionesCLI &opciones) {
    ParametrosTraza pTraza;
    pTraza.longitud = opciones.longitudTraza;
    pTraza.semilla = opciones.semilla;
    pTraza.tamanoCacheBytes = opciones.configuracion.getSizeBytes();
    pTraza.tamanoBloqueBytes = opciones.configuracion.getBlockSize();
    pTraza.proporcionEscrituras = opciones.proporcionEscrituras;

    auto cargas = generarCargasTrabajo(pTraza, opciones.nombreCarga);

    std::vector<std::string> pols = opciones.politicas;
    if (opciones.incluirOpt && std::find(pols.begin(), pols.end(), "opt") == pols.end()) {
        pols.push_back("opt");
    }

    EntornoPolitica entorno;
    entorno.semilla = opciones.semilla;

    SimuladorCache simulador;
    std::cout << "\n===============================================================================\n";
    std::cout << "          SIMULADOR DE POLITICAS DE REEMPLAZO DE CACHE (PROYECTO 4)            \n";
    std::cout << "===============================================================================\n";
    std::cout << "  Configuracion Cache : " << opciones.configuracion.describir() << "\n";
    std::cout << "  Latencia Hit / Miss : " << opciones.configuracion.getHitLatency() << " ciclos / " 
              << opciones.configuracion.getMissPenalty() << " ciclos penalizacion\n";
    if (opciones.configuracion.getPorcentajeCalentamiento() > 0.0) {
        std::cout << "  Fase Warm-up        : " << (opciones.configuracion.getPorcentajeCalentamiento() * 100.0) << "%\n";
    }
    std::cout << "  Politicas Activas   : ";
    for (const auto &p : pols) std::cout << p << " ";
    std::cout << "\n===============================================================================\n";

    auto filas = simulador.ejecutarComparacion(cargas, opciones.configuracion, pols, entorno, opciones.repeticiones);

    Reporteador::imprimirComparacion(std::cout, filas);
    Reporteador::imprimirMatrizResumen(std::cout, filas);
    Reporteador::imprimirMatrizEstructural(std::cout);

    if (!opciones.archivoCsvSalida.empty()) {
        std::string err;
        if (Reporteador::exportarCsv(opciones.archivoCsvSalida, filas, err)) {
            std::cout << ">>> Resultados exportados exitosamente a: " << opciones.archivoCsvSalida << "\n\n";
        } else {
            std::cerr << "Error al exportar CSV: " << err << "\n";
            return 1;
        }
    }

    return 0;
}

int cmdDidactico(const OpcionesCLI &opciones) {
    ParametrosTraza pTraza;
    pTraza.longitud = opciones.pasosDidacticos * 2;
    pTraza.semilla = opciones.semilla;
    pTraza.tamanoCacheBytes = opciones.configuracion.getSizeBytes();
    pTraza.tamanoBloqueBytes = opciones.configuracion.getBlockSize();
    pTraza.proporcionEscrituras = opciones.proporcionEscrituras;

    std::vector<AccesoMemoria> traza;
    std::string nombreCarga = opciones.nombreCarga;

    if (!opciones.archivoTrazaEntrada.empty()) {
        std::string err;
        if (!cargarTrazaDesdeArchivo(opciones.archivoTrazaEntrada, traza, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        nombreCarga = opciones.archivoTrazaEntrada;
    } else {
        if (nombreCarga == "all") nombreCarga = "zipf";
        auto cargas = generarCargasTrabajo(pTraza, nombreCarga);
        traza = cargas.front().accesos;
    }

    std::string pol = opciones.politicas.empty() ? "lru" : opciones.politicas.front();
    ModoDidactico didactico(opciones.configuracion, pol, opciones.modoInteractivo, opciones.pasosDidacticos);

    EntornoPolitica entorno;
    entorno.semilla = opciones.semilla;

    didactico.ejecutarPasoAPaso(traza, entorno);
    return 0;
}

int cmdSweep(const OpcionesCLI &opciones) {
    ParametrosTraza pTraza;
    pTraza.longitud = opciones.longitudTraza;
    pTraza.semilla = opciones.semilla;
    pTraza.tamanoCacheBytes = opciones.configuracion.getSizeBytes();
    pTraza.tamanoBloqueBytes = opciones.configuracion.getBlockSize();
    pTraza.proporcionEscrituras = opciones.proporcionEscrituras;

    std::string nomCarga = (opciones.nombreCarga == "all") ? "zipf" : opciones.nombreCarga;
    auto cargas = generarCargasTrabajo(pTraza, nomCarga);

    ParametroBarrido param;
    if (!parsearParametroBarrido(opciones.parametroBarrido, param)) {
        std::cerr << "Error: Parametro de barrido desconocido '" << opciones.parametroBarrido 
                  << "'. Use: size, ways, block\n";
        return 1;
    }

    std::vector<std::size_t> valores = opciones.valoresBarrido;
    if (valores.empty()) {
        switch (param) {
            case ParametroBarrido::TAMANO_CACHE:
                valores = {1024, 2048, 4096, 8192, 16384, 32768, 65536};
                break;
            case ParametroBarrido::VIAS_ASOCIATIVIDAD:
                valores = {1, 2, 4, 8, 16, 0};
                break;
            case ParametroBarrido::TAMANO_BLOQUE:
                valores = {16, 32, 64, 128, 256};
                break;
        }
    }

    std::vector<std::string> pols = opciones.politicas;
    if (opciones.incluirOpt && std::find(pols.begin(), pols.end(), "opt") == pols.end()) {
        pols.push_back("opt");
    }

    EntornoPolitica entorno;
    entorno.semilla = opciones.semilla;

    SimuladorCache simulador;
    std::vector<std::string> advertencias;
    auto filas = simulador.ejecutarBarrido(cargas.front(), param, valores, opciones.configuracion,
                                           pols, entorno, opciones.repeticiones, advertencias);

    for (const auto &adv : advertencias) {
        std::cerr << "Aviso: " << adv << "\n";
    }

    Reporteador::imprimirBarrido(std::cout, filas);

    if (!opciones.archivoCsvSalida.empty()) {
        std::string err;
        if (Reporteador::exportarCsv(opciones.archivoCsvSalida, filas, err)) {
            std::cout << ">>> Barrido exportado a: " << opciones.archivoCsvSalida << "\n\n";
        } else {
            std::cerr << "Error al exportar CSV: " << err << "\n";
            return 1;
        }
    }

    return 0;
}

int cmdTraza(const OpcionesCLI &opciones) {
    if (opciones.archivoTrazaEntrada.empty()) {
        std::cerr << "Error: Debe especificar el archivo de traza con --file <ruta>\n";
        return 1;
    }

    std::vector<AccesoMemoria> traza;
    std::string err;
    if (!cargarTrazaDesdeArchivo(opciones.archivoTrazaEntrada, traza, err)) {
        std::cerr << "Error al leer traza: " << err << "\n";
        return 1;
    }

    TrazaNombrada tn;
    tn.nombre = opciones.archivoTrazaEntrada;
    tn.tipoDato = "Real (Archivo)";
    tn.descripcion = "Traza cargada desde archivo externo";
    tn.accesos = traza;

    std::vector<std::string> pols = opciones.politicas;
    if (opciones.incluirOpt && std::find(pols.begin(), pols.end(), "opt") == pols.end()) {
        pols.push_back("opt");
    }

    EntornoPolitica entorno;
    entorno.semilla = opciones.semilla;

    SimuladorCache simulador;
    auto filas = simulador.ejecutarComparacion({tn}, opciones.configuracion, pols, entorno, opciones.repeticiones);

    Reporteador::imprimirComparacion(std::cout, filas);
    Reporteador::imprimirMatrizResumen(std::cout, filas);

    if (!opciones.archivoCsvSalida.empty()) {
        std::string errCsv;
        if (Reporteador::exportarCsv(opciones.archivoCsvSalida, filas, errCsv)) {
            std::cout << ">>> Resultados de traza exportados a: " << opciones.archivoCsvSalida << "\n\n";
        }
    }

    return 0;
}

int cmdGenerar(const OpcionesCLI &opciones) {
    if (opciones.archivoTrazaSalida.empty()) {
        std::cerr << "Error: Debe especificar el archivo de destino con --out <ruta>\n";
        return 1;
    }

    ParametrosTraza pTraza;
    pTraza.longitud = opciones.longitudTraza;
    pTraza.semilla = opciones.semilla;
    pTraza.tamanoCacheBytes = opciones.configuracion.getSizeBytes();
    pTraza.tamanoBloqueBytes = opciones.configuracion.getBlockSize();
    pTraza.proporcionEscrituras = opciones.proporcionEscrituras;

    std::string nomCarga = (opciones.nombreCarga == "all") ? "zipf" : opciones.nombreCarga;
    auto cargas = generarCargasTrabajo(pTraza, nomCarga);

    std::string err;
    if (!guardarTrazaEnArchivo(opciones.archivoTrazaSalida, cargas.front().accesos, err)) {
        std::cerr << "Error al guardar traza: " << err << "\n";
        return 1;
    }

    std::cout << ">>> Traza '" << nomCarga << "' con " << cargas.front().accesos.size() 
              << " accesos guardada en: " << opciones.archivoTrazaSalida << "\n";
    return 0;
}

int cmdListar() {
    std::cout << "\n===============================================================================\n";
    std::cout << "          POLITICAS DE REEMPLAZO Y CARGAS DE TRABAJO DISPONIBLES               \n";
    std::cout << "===============================================================================\n";
    std::cout << "  Politicas de Reemplazo Evaluadas:\n";
    for (const auto &p : obtenerPoliticasDisponibles()) {
        std::cout << "    * " << std::left << std::setw(10) << p;
        if (p == "lru") std::cout << ": Least Recently Used (Principio de Localidad Temporal)";
        else if (p == "fifo") std::cout << ": First-In, First-Out (Orden estricto de llegada con Cola)";
        else if (p == "lfu") std::cout << ": Least Frequently Used (Estructura 2D O(1) de Ketan Shah)";
        else if (p == "random") std::cout << ": Seleccion pseudoaleatoria (Linea base)";
        else if (p == "opt") std::cout << ": Cota teorica optima de Belady (Offline)";
        std::cout << "\n";
    }

    std::cout << "\n  Cargas de Trabajo Disponibles:\n";
    for (const auto &c : obtenerNombresCargas()) {
        std::cout << "    * " << std::left << std::setw(18) << c 
                  << " [" << std::setw(10) << obtenerTipoCarga(c) << "]: " 
                  << obtenerDescripcionCarga(c) << "\n";
    }
    std::cout << "===============================================================================\n\n";
    return 0;
}

} // namespace

bool InterfazCLI::parsearArgumentos(int argc, char *argv[], OpcionesCLI &opciones, std::string &error) {
    if (argc > 1) {
        std::string primerArg = argv[1];
        if (primerArg[0] != '-') {
            opciones.comando = primerArg;
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "compare" || arg == "didactico" || arg == "sweep" ||
            arg == "traza" || arg == "generar" || arg == "listar" || arg == "ayuda" || arg == "help") {
            opciones.comando = arg;
            continue;
        }

        if (arg == "--size" && i + 1 < argc) {
            std::size_t sz = 0;
            if (!parsearTamanoBytes(argv[++i], sz)) {
                error = "Tamano de cache invalido: " + std::string(argv[i]);
                return false;
            }
            opciones.configuracion.setSizeBytes(sz);
        } else if (arg == "--block" && i + 1 < argc) {
            std::size_t blk = 0;
            if (!parsearTamanoBytes(argv[++i], blk)) {
                error = "Tamano de bloque invalido: " + std::string(argv[i]);
                return false;
            }
            opciones.configuracion.setBlockSize(blk);
        } else if (arg == "--ways" && i + 1 < argc) {
            opciones.configuracion.setWays(std::stoull(argv[++i]));
        } else if (arg == "--policies" && i + 1 < argc) {
            opciones.politicas = dividirCadena(argv[++i], ',');
        } else if (arg == "--include-opt" || arg == "--opt") {
            opciones.incluirOpt = true;
        } else if (arg == "--no-opt") {
            opciones.incluirOpt = false;
        } else if (arg == "--workload" && i + 1 < argc) {
            opciones.nombreCarga = argv[++i];
        } else if (arg == "--length" && i + 1 < argc) {
            opciones.longitudTraza = std::stoull(argv[++i]);
        } else if (arg == "--warmup" && i + 1 < argc) {
            opciones.configuracion.setPorcentajeCalentamiento(std::stod(argv[++i]));
        } else if (arg == "--seed" && i + 1 < argc) {
            opciones.semilla = std::stoull(argv[++i]);
        } else if (arg == "--repeats" && i + 1 < argc) {
            opciones.repeticiones = std::stoi(argv[++i]);
        } else if (arg == "--write-ratio" && i + 1 < argc) {
            opciones.proporcionEscrituras = std::stod(argv[++i]);
        } else if (arg == "--hit-latency" && i + 1 < argc) {
            opciones.configuracion.setHitLatency(std::stod(argv[++i]));
        } else if (arg == "--miss-penalty" && i + 1 < argc) {
            opciones.configuracion.setMissPenalty(std::stod(argv[++i]));
        } else if (arg == "--interactivo") {
            opciones.modoInteractivo = true;
        } else if (arg == "--pasos" && i + 1 < argc) {
            opciones.pasosDidacticos = std::stoull(argv[++i]);
        } else if (arg == "--param" && i + 1 < argc) {
            opciones.parametroBarrido = argv[++i];
        } else if (arg == "--values" && i + 1 < argc) {
            auto vals = dividirCadena(argv[++i], ',');
            opciones.valoresBarrido.clear();
            for (const auto &v : vals) {
                std::size_t n = 0;
                if (parsearTamanoBytes(v, n)) {
                    opciones.valoresBarrido.push_back(n);
                }
            }
        } else if (arg == "--file" && i + 1 < argc) {
            opciones.archivoTrazaEntrada = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            opciones.archivoTrazaSalida = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            opciones.archivoCsvSalida = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            opciones.comando = "ayuda";
        }
    }

    std::string errConfig;
    if (!opciones.configuracion.validar(errConfig)) {
        error = errConfig;
        return false;
    }

    return true;
}

int InterfazCLI::ejecutarComando(const OpcionesCLI &opciones) {
    if (opciones.comando == "compare") {
        return cmdCompare(opciones);
    } else if (opciones.comando == "didactico" || opciones.comando == "step" || opciones.comando == "explicar") {
        return cmdDidactico(opciones);
    } else if (opciones.comando == "sweep") {
        return cmdSweep(opciones);
    } else if (opciones.comando == "traza" || opciones.comando == "trace") {
        return cmdTraza(opciones);
    } else if (opciones.comando == "generar" || opciones.comando == "gen") {
        return cmdGenerar(opciones);
    } else if (opciones.comando == "listar" || opciones.comando == "list") {
        return cmdListar();
    } else if (opciones.comando == "ayuda" || opciones.comando == "help") {
        std::cout << obtenerGuiaUso();
        return 0;
    }

    std::cerr << "Comando desconocido: '" << opciones.comando << "'. Use 'ayuda' para ver las opciones.\n";
    return 1;
}

std::string InterfazCLI::obtenerGuiaUso() {
    return R"(
===============================================================================
  SIMULADOR DE MEMORIA CACHE (PROYECTO 4 - ARQUITECTURA DEL COMPUTADOR)
===============================================================================

Uso:
  ./cachesim <comando> [opciones]

Comandos Principales:
  compare     Modo Normal: Compara cientificamente LRU, FIFO y LFU en las cargas.
  didactico   Modo Explicativo: Muestra paso a paso el funcionamiento interno,
              descomposicion de bits en hardware, estado de sets y justificacion
              de cada decision de reemplazo y fallo 3C.
  sweep       Realiza un barrido experimental de un parametro (size|ways|block).
  traza       Ejecuta la simulacion sobre una traza de archivo (--file <ruta>).
  generar     Genera y exporta una traza de memoria a disco (--out <ruta>).
  listar      Muestra el catalogo completo de politicas y cargas de trabajo.
  ayuda       Imprime esta guia de uso.

Opciones de Configuracion de Hardware:
  --size <tam>       Tamano total de cache (ej: 16K, 32K, 64K, 1M)  [32K]
  --block <tam>      Tamano de linea/bloque en bytes (ej: 32, 64)    [64]
  --ways <n>         Grado de asociatividad (0=Total, 1=Directa, N)  [4]
  --hit-latency <c>  Latencia de acierto en ciclos de reloj          [1.0]
  --miss-penalty <c> Penalizacion por fallo (DRAM Stall) en ciclos   [100.0]
  --warmup <frac>    Fraccion de estabilizacion previa (Warm-up)     [0.0]

Opciones de Simulacion y Politicas:
  --policies <a,b>   Lista separada por comas (lru,fifo,lfu,random)  [lru,fifo,lfu]
  --include-opt      Incluye la cota teorica optima de Belady (opt)
  --workload <nom>   Carga sintetica/aleatoria o 'all'               [all]
  --length <n>       Numero de accesos a simular por carga           [100000]
  --seed <n>         Semilla para reproducibilidad estocastica       [42]
  --repeats <n>      Repeticiones para medicion con steady_clock     [3]
  --write-ratio <r>  Proporcion de escrituras (0.0 a 1.0)            [0.2]

Opciones del Modo Didactico:
  --pasos <n>        Numero de accesos explicados en detalle         [20]
  --interactivo      Avanza paso a paso presionando [Enter]

Opciones de Archivos y Barridos:
  --param <p>        Parametro de barrido: size, ways o block        [size]
  --values <v1,v2>   Valores a evaluar en el barrido
  --file <ruta>      Archivo de traza de entrada
  --out <ruta>       Archivo de salida para guardar traza
  --csv <ruta>       Ruta de exportacion a archivo CSV para graficas

Ejemplos Didacticos y de Evaluacion:
  ./cachesim compare --workload zipf
  ./cachesim didactico --workload bucle_thrashing --pasos 15
  ./cachesim didactico --interactivo --pasos 10
  ./cachesim sweep --param ways --workload caliente_frio --csv barrido_vias.csv
  ./cachesim traza --file trazas/ejemplo_simple.trace
===============================================================================
)";
}

} // namespace cachesim
