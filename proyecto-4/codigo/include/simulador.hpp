#ifndef SIMULADOR_HPP
#define SIMULADOR_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

#include "tipos.hpp"
#include "config_cache.hpp"
#include "cache.hpp"
#include "politica_reemplazo.hpp"
#include "generador_trazas.hpp"

namespace cachesim {

// Observador para interactuar paso a paso en cada acceso
using ObservadorPaso = std::function<void(std::size_t paso,
                                          const AccesoMemoria &acceso,
                                          const ResultadoAcceso &resultado,
                                          const Cache &cache)>;

// Resultado consolidado de una corrida de simulacion
struct ResultadoCorrida {
    std::string politica;
    ConfiguracionCache configuracion;
    EstadisticasCache estadisticas;
    double segundos = 0.0;
    double amat = 0.0;
    double ciclosStall = 0.0;
};

// Registro de comparacion en experimentos
struct FilaExperimento {
    std::string nombreCarga;
    std::string tipoCarga;
    std::string etiquetaParametro;
    ResultadoCorrida resultado;
};

// Parametros para barridos experimentales (Sweeps)
enum class ParametroBarrido {
    TAMANO_CACHE,
    VIAS_ASOCIATIVIDAD,
    TAMANO_BLOQUE
};

std::string getNombreParametroBarrido(ParametroBarrido p);
bool parsearParametroBarrido(const std::string &str, ParametroBarrido &salida);

// ============================================================================
// Motor de Simulacion de Cache (Sin variables globales)
// ============================================================================
class SimuladorCache {
public:
    // constructor
    SimuladorCache() {}

    // Barrera de optimizacion del compilador (benchmark::DoNotOptimize)
    template <typename T>
    static inline void noOptimizar(T const &valor) {
        asm volatile("" : : "g"(valor) : "memory");
    }

    // Ejecuta una simulacion individual sobre una traza
    ResultadoCorrida ejecutarSimulacion(const ConfiguracionCache &cfg,
                                        const std::string &politica,
                                        const std::vector<AccesoMemoria> &traza,
                                        const EntornoPolitica &entorno = EntornoPolitica{},
                                        int repeticiones = 1,
                                        ObservadorPaso observador = nullptr);

    // Compara multiples politicas sobre una lista de cargas de trabajo
    std::vector<FilaExperimento> ejecutarComparacion(const std::vector<TrazaNombrada> &cargas,
                                                     const ConfiguracionCache &cfg,
                                                     const std::vector<std::string> &politicas,
                                                     const EntornoPolitica &entorno,
                                                     int repeticiones = 1);

    // Ejecuta un barrido variando un parametro de hardware
    std::vector<FilaExperimento> ejecutarBarrido(const TrazaNombrada &carga,
                                                ParametroBarrido parametro,
                                                const std::vector<std::size_t> &valores,
                                                const ConfiguracionCache &cfgBase,
                                                const std::vector<std::string> &politicas,
                                                const EntornoPolitica &entorno,
                                                int repeticiones,
                                                std::vector<std::string> &advertencias);

    // destructor
    ~SimuladorCache() {}
};

} // namespace cachesim

#endif // SIMULADOR_HPP
