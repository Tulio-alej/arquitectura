#ifndef CLI_HPP
#define CLI_HPP

#include <string>
#include <vector>
#include "config_cache.hpp"
#include "simulador.hpp"
#include "generador_trazas.hpp"

namespace cachesim {

// Opciones parseadas de la linea de comandos (Sin variables globales)
struct OpcionesCLI {
    std::string comando = "compare";

    // Parametros de hardware de cache
    ConfiguracionCache configuracion;

    // Parametros de politicas y simulacion
    std::vector<std::string> politicas = {"lru", "fifo", "lfu"};
    bool incluirOpt = false;
    int repeticiones = 3;
    std::uint64_t semilla = 42;

    // Carga de trabajo
    std::string nombreCarga = "all";
    std::size_t longitudTraza = 100000;
    double proporcionEscrituras = 0.2;

    // Opciones del modo didactico
    bool modoInteractivo = false;
    std::size_t pasosDidacticos = 20;

    // Opciones de barrido (sweep)
    std::string parametroBarrido = "size";
    std::vector<std::size_t> valoresBarrido;

    // Archivos
    std::string archivoTrazaEntrada;
    std::string archivoTrazaSalida;
    std::string archivoCsvSalida;
};

// Parser y despachador de comandos
class InterfazCLI {
public:
    // constructor
    InterfazCLI() {}

    // Parseo de argumentos de entrada
    static bool parsearArgumentos(int argc, char *argv[], OpcionesCLI &opciones, std::string &error);

    // Despacho del comando solicitado
    static int ejecutarComando(const OpcionesCLI &opciones);

    // Ayuda y uso del simulador
    static std::string obtenerGuiaUso();

    // destructor
    ~InterfazCLI() {}
};

} // namespace cachesim

#endif // CLI_HPP
