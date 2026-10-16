#ifndef CLASIFICADOR_3C_HPP
#define CLASIFICADOR_3C_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <list>

#include "tipos.hpp"
#include "config_cache.hpp"

namespace cachesim {

// ============================================================================
// Clasificador 3C de Mark D. Hill (Cold, Capacity, Conflict)
// ============================================================================
// Emplea una cache sombra totalmente asociativa con reemplazo LRU del mismo
// tamano total para discriminar de forma exacta y cientifica los 3 tipos de fallos.
class Clasificador3C {
private:
    std::size_t capacidadLineas;
    std::unordered_set<DireccionMemoria> bloquesVistos;

    // Cache sombra totalmente asociativa (LRU)
    std::list<DireccionMemoria> lruSombra;
    std::unordered_map<DireccionMemoria, std::list<DireccionMemoria>::iterator> mapaSombra;

public:
    // constructor
    explicit Clasificador3C(std::size_t lineasTotales);

    // metodos principales
    TipoFallo3C clasificarFallo(DireccionMemoria bloque);
    void actualizarAcceso(DireccionMemoria bloque);
    void reiniciar();

    // getters
    std::size_t getBloquesUnicosVistos() const { return this->bloquesVistos.size(); }

    // destructor
    ~Clasificador3C() {}
};

} // namespace cachesim

#endif // CLASIFICADOR_3C_HPP
