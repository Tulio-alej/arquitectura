#ifndef MODO_DIDACTICO_HPP
#define MODO_DIDACTICO_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "tipos.hpp"
#include "config_cache.hpp"
#include "cache.hpp"
#include "pila.hpp"

namespace cachesim {

// ============================================================================
// Modulo Explicativo y Didactico de Simulacion de Cache
// ============================================================================
class ModoDidactico {
private:
    ConfiguracionCache configuracion;
    std::string nombrePolitica;
    bool modoInteractivo; // Si es true, espera [Enter] entre pasos
    std::size_t limitePasos;

public:
    // constructores
    ModoDidactico(const ConfiguracionCache &cfg, const std::string &politica,
                  bool interactivo = false, std::size_t limitePasos = 25);

    // getters y setters
    void setInteractivo(bool inter) { this->modoInteractivo = inter; }
    void setLimitePasos(std::size_t lim) { this->limitePasos = lim; }

    // metodos didacticos de visualizacion
    void mostrarEncabezadoConceptual() const;
    void mostrarDescomposicionHardware(DireccionMemoria dir) const;
    void mostrarEstadoConjunto(const Cache &cache, IndiceConjunto idxConjunto) const;

    // ejecucion explicativa paso a paso
    void ejecutarPasoAPaso(const std::vector<AccesoMemoria> &traza,
                           const EntornoPolitica &entorno = EntornoPolitica{});

    // explicacion teorica final
    void mostrarConclusionPedagogica(const EstadisticasCache &stats,
                                     const std::string &nombreCarga) const;

    // destructor
    ~ModoDidactico() {}
};

} // namespace cachesim

#endif // MODO_DIDACTICO_HPP
