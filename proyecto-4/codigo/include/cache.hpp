#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "tipos.hpp"
#include "config_cache.hpp"
#include "politica_reemplazo.hpp"
#include "clasificador_3c.hpp"
#include "pila.hpp" // Libreria del usuario en librerias-lpc/pila.hpp

namespace cachesim {

// ============================================================================
// Conjunto individual de la cache asociativa
// ============================================================================
struct ConjuntoCache {
    std::vector<LineaCache> lineas;
    std::unordered_map<DireccionMemoria, int> mapaBloqueVia; // Bloque -> Indice de via
    std::unique_ptr<PoliticaReemplazo> politica;
    Pila<int> viasLibres;                                     // Gestiona vias disponibles usando Pila

    // constructor
    ConjuntoCache() {}
};

// ============================================================================
// Controlador de Memoria Cache (Write-Back / Write-Allocate)
// ============================================================================
class Cache {
private:
    ConfiguracionCache configuracion;
    std::string nombrePolitica;
    EntornoPolitica entorno;

    unsigned desplazamientoBloque;
    std::size_t numConjuntos;
    std::size_t mascaraConjuntos;

    std::vector<ConjuntoCache> conjuntos;
    Clasificador3C clasificador;
    EstadisticasCache estadisticas;

    void inicializarEstructuras();

public:
    // constructores
    Cache(const ConfiguracionCache &cfg, const std::string &politica, const EntornoPolitica &env = EntornoPolitica{});

    // metodos principales
    ResultadoAcceso acceder(const AccesoMemoria &acceso, std::size_t indiceTraza = 0, bool registrarStats = true);
    void reiniciar();

    // descomposicion de direcciones a nivel de hardware (MMU)
    void descomponerDireccion(DireccionMemoria dir, EtiquetaBloque &tag, IndiceConjunto &indice, DireccionMemoria &offset) const;
    IndiceConjunto calcularConjunto(DireccionMemoria dir) const;
    DireccionMemoria calcularBloque(DireccionMemoria dir) const;

    // getters
    const ConfiguracionCache& getConfiguracion() const { return this->configuracion; }
    const EstadisticasCache& getEstadisticas() const { return this->estadisticas; }
    EstadisticasCache& getEstadisticasRef() { return this->estadisticas; }
    const std::string& getNombrePolitica() const { return this->nombrePolitica; }
    std::size_t getNumConjuntos() const { return this->numConjuntos; }
    unsigned getDesplazamientoBloque() const { return this->desplazamientoBloque; }
    const ConjuntoCache& getConjunto(std::size_t idx) const { return this->conjuntos[idx]; }

    // destructor
    ~Cache() {}
};

} // namespace cachesim

#endif // CACHE_HPP
