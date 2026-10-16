#include "simulador.hpp"
#include <limits>
#include <iostream>

namespace cachesim {

std::string getNombreParametroBarrido(ParametroBarrido p) {
    switch (p) {
        case ParametroBarrido::TAMANO_CACHE:
            return "Tamano de Cache";
        case ParametroBarrido::VIAS_ASOCIATIVIDAD:
            return "Vias de Asociatividad";
        case ParametroBarrido::TAMANO_BLOQUE:
            return "Tamano de Bloque";
    }
    return "Desconocido";
}

bool parsearParametroBarrido(const std::string &str, ParametroBarrido &salida) {
    if (str == "size" || str == "tamano") {
        salida = ParametroBarrido::TAMANO_CACHE;
        return true;
    } else if (str == "ways" || str == "vias") {
        salida = ParametroBarrido::VIAS_ASOCIATIVIDAD;
        return true;
    } else if (str == "block" || str == "bloque") {
        salida = ParametroBarrido::TAMANO_BLOQUE;
        return true;
    }
    return false;
}

ResultadoCorrida SimuladorCache::ejecutarSimulacion(const ConfiguracionCache &cfg,
                                                    const std::string &politica,
                                                    const std::vector<AccesoMemoria> &traza,
                                                    const EntornoPolitica &entornoIn,
                                                    int repeticiones,
                                                    ObservadorPaso observador) {
    EntornoPolitica entorno = entornoIn;

    // Si es la politica optima (Belady), precalculamos la tabla de accesos futuros
    if (esPoliticaOffline(politica)) {
        entorno.tablaSiguienteUso = std::make_shared<const std::vector<std::size_t>>(
            construirTablaSiguienteUso(traza, cfg.getBitsOffset())
        );
    }

    ResultadoCorrida corrida;
    corrida.politica = politica;
    corrida.configuracion = cfg;

    if (observador) {
        repeticiones = 1;
    }

    double menorTiempo = std::numeric_limits<double>::max();
    const int totalReps = std::max(1, repeticiones);

    for (int rep = 0; rep < totalReps; ++rep) {
        Cache cache(cfg, politica, entorno);

        // ====================================================================
        // FASE DE CALENTAMIENTO (WARM-UP)
        // ====================================================================
        std::size_t pasoInicioMedicion = 0;
        if (cfg.getPorcentajeCalentamiento() > 0.0 && !observador) {
            std::size_t pasosCalentamiento = static_cast<std::size_t>(
                traza.size() * cfg.getPorcentajeCalentamiento()
            );
            for (std::size_t i = 0; i < pasosCalentamiento; ++i) {
                cache.acceder(traza[i], i, false); // No registra estadisticas
            }
            pasoInicioMedicion = pasosCalentamiento;
            // Reiniciar contadores para medir unicamente el estado en regimen permanente
            cache.getEstadisticasRef().reiniciar();
        }

        // ====================================================================
        // MEDICION CON RELOJ MONOTONICO steady_clock Y BARRERA DE COMPILADOR
        // ====================================================================
        const auto inicioTiempo = std::chrono::steady_clock::now();

        for (std::size_t i = pasoInicioMedicion; i < traza.size(); ++i) {
            const ResultadoAcceso res = cache.acceder(traza[i], i, true);

            // Barrera para evitar que el compilador descarte codigo critico
            noOptimizar(res.acierto);
            noOptimizar(res.via);

            if (observador) {
                observador(i, traza[i], res, cache);
            }
        }

        const auto finTiempo = std::chrono::steady_clock::now();
        const double duracionSegundos = std::chrono::duration<double>(finTiempo - inicioTiempo).count();

        if (duracionSegundos < menorTiempo) {
            menorTiempo = duracionSegundos;
        }

        corrida.estadisticas = cache.getEstadisticas();
    }

    corrida.segundos = menorTiempo;
    corrida.estadisticas.tiempoSegundos = menorTiempo;
    corrida.amat = corrida.estadisticas.getAmat(cfg);
    corrida.ciclosStall = corrida.estadisticas.getCiclosStallTotales(cfg);

    return corrida;
}

std::vector<FilaExperimento> SimuladorCache::ejecutarComparacion(const std::vector<TrazaNombrada> &cargas,
                                                                 const ConfiguracionCache &cfg,
                                                                 const std::vector<std::string> &politicas,
                                                                 const EntornoPolitica &entorno,
                                                                 int repeticiones) {
    std::vector<FilaExperimento> filas;

    for (const auto &carga : cargas) {
        for (const auto &pol : politicas) {
            FilaExperimento fila;
            fila.nombreCarga = carga.nombre;
            fila.tipoCarga = carga.tipoDato;
            fila.etiquetaParametro = "";
            fila.resultado = this->ejecutarSimulacion(cfg, pol, carga.accesos, entorno, repeticiones);
            filas.push_back(std::move(fila));
        }
    }

    return filas;
}

std::vector<FilaExperimento> SimuladorCache::ejecutarBarrido(const TrazaNombrada &carga,
                                                            ParametroBarrido parametro,
                                                            const std::vector<std::size_t> &valores,
                                                            const ConfiguracionCache &cfgBase,
                                                            const std::vector<std::string> &politicas,
                                                            const EntornoPolitica &entorno,
                                                            int repeticiones,
                                                            std::vector<std::string> &advertencias) {
    std::vector<FilaExperimento> filas;

    for (std::size_t val : valores) {
        ConfiguracionCache cfgModificada = cfgBase;
        std::string etiqueta;

        switch (parametro) {
            case ParametroBarrido::TAMANO_CACHE:
                cfgModificada.setSizeBytes(val);
                etiqueta = "Tamano=" + std::to_string(val / 1024) + "KB";
                break;
            case ParametroBarrido::VIAS_ASOCIATIVIDAD:
                cfgModificada.setWays(val);
                etiqueta = "Vias=" + (val == 0 ? std::string("Total") : std::to_string(val));
                break;
            case ParametroBarrido::TAMANO_BLOQUE:
                cfgModificada.setBlockSize(val);
                etiqueta = "Bloque=" + std::to_string(val) + "B";
                break;
        }

        std::string error;
        if (!cfgModificada.validar(error)) {
            advertencias.push_back("Valor omitido (" + etiqueta + "): " + error);
            continue;
        }

        for (const auto &pol : politicas) {
            FilaExperimento fila;
            fila.nombreCarga = carga.nombre;
            fila.tipoCarga = carga.tipoDato;
            fila.etiquetaParametro = etiqueta;
            fila.resultado = this->ejecutarSimulacion(cfgModificada, pol, carga.accesos, entorno, repeticiones);
            filas.push_back(std::move(fila));
        }
    }

    return filas;
}

} // namespace cachesim
