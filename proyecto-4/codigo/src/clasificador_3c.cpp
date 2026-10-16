#include "clasificador_3c.hpp"

namespace cachesim {

Clasificador3C::Clasificador3C(std::size_t lineasTotales) : capacidadLineas(lineasTotales) {}

void Clasificador3C::reiniciar() {
    this->bloquesVistos.clear();
    this->lruSombra.clear();
    this->mapaSombra.clear();
}

TipoFallo3C Clasificador3C::clasificarFallo(DireccionMemoria bloque) {
    // 1. Si el bloque nunca antes ha sido referenciado en toda la ejecucion: COLD (Compulsorio)
    if (this->bloquesVistos.find(bloque) == this->bloquesVistos.end()) {
        return TipoFallo3C::COLD;
    }

    // 2. Si ya fue visto antes, consultamos si estaria presente en una cache totalmente asociativa
    auto itSombra = this->mapaSombra.find(bloque);
    if (itSombra != this->mapaSombra.end()) {
        // Estaba presente en la cache sombra -> Fallo por CONFLICTO (colision de conjunto)
        return TipoFallo3C::CONFLICT;
    } else {
        // Tampoco cabia en la cache sombra -> Fallo por CAPACIDAD (working set > cache)
        return TipoFallo3C::CAPACITY;
    }
}

void Clasificador3C::actualizarAcceso(DireccionMemoria bloque) {
    // Registramos que este bloque ya ha sido visto al menos una vez
    this->bloquesVistos.insert(bloque);

    // Actualizamos la cache sombra totalmente asociativa (politica LRU)
    auto itSombra = this->mapaSombra.find(bloque);
    if (itSombra != this->mapaSombra.end()) {
        // Mover al frente (mas reciente)
        this->lruSombra.erase(itSombra->second);
        this->lruSombra.push_front(bloque);
        this->mapaSombra[bloque] = this->lruSombra.begin();
    } else {
        // Si la cache sombra esta llena, desalojar el LRU
        if (this->lruSombra.size() >= this->capacidadLineas && this->capacidadLineas > 0) {
            DireccionMemoria victima = this->lruSombra.back();
            this->lruSombra.pop_back();
            this->mapaSombra.erase(victima);
        }
        this->lruSombra.push_front(bloque);
        this->mapaSombra[bloque] = this->lruSombra.begin();
    }
}

} // namespace cachesim
