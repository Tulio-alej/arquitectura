#include "cache.hpp"
#include <stdexcept>
#include <sstream>

namespace cachesim {

Cache::Cache(const ConfiguracionCache &cfg, const std::string &politica, const EntornoPolitica &env)
    : configuracion(cfg), nombrePolitica(politica), entorno(env),
      desplazamientoBloque(0), numConjuntos(0), mascaraConjuntos(0),
      clasificador(cfg.getNumLineas()) {

    std::string error;
    if (!this->configuracion.validar(error)) {
        throw std::invalid_argument("Configuracion de cache invalida: " + error);
    }

    this->desplazamientoBloque = this->configuracion.getBitsOffset();
    this->numConjuntos = this->configuracion.getNumSets();
    this->mascaraConjuntos = this->numConjuntos - 1;

    this->inicializarEstructuras();
}

void Cache::inicializarEstructuras() {
    const int vias = static_cast<int>(this->configuracion.getViasEfectivas());
    this->conjuntos.resize(this->numConjuntos);

    for (std::size_t i = 0; i < this->numConjuntos; ++i) {
        this->conjuntos[i].lineas.resize(vias);
        this->conjuntos[i].mapaBloqueVia.clear();
        this->conjuntos[i].politica = crearPolitica(this->nombrePolitica, vias, this->entorno);

        // Inicializar vias libres usando la Pila de librerias-lpc
        this->conjuntos[i].viasLibres.vaciar();
        for (int v = vias - 1; v >= 0; --v) {
            this->conjuntos[i].viasLibres.apilar(v);
        }
    }

    this->clasificador.reiniciar();
    this->estadisticas.reiniciar();
}

void Cache::reiniciar() {
    this->inicializarEstructuras();
}

void Cache::descomponerDireccion(DireccionMemoria dir, EtiquetaBloque &tag,
                                 IndiceConjunto &indice, DireccionMemoria &offset) const {
    const unsigned bitsOffset = this->configuracion.getBitsOffset();
    const unsigned bitsIndice = this->configuracion.getBitsIndice();

    const DireccionMemoria mascaraOffset = (DireccionMemoria(1) << bitsOffset) - 1;
    offset = dir & mascaraOffset;

    if (bitsIndice == 0) {
        // En cache totalmente asociativa (1 solo set)
        indice = 0;
    } else {
        indice = (dir >> bitsOffset) & this->mascaraConjuntos;
    }

    tag = dir >> (bitsOffset + bitsIndice);
}

IndiceConjunto Cache::calcularConjunto(DireccionMemoria dir) const {
    if (this->numConjuntos <= 1) return 0;
    return (dir >> this->desplazamientoBloque) & this->mascaraConjuntos;
}

DireccionMemoria Cache::calcularBloque(DireccionMemoria dir) const {
    return dir >> this->desplazamientoBloque;
}

ResultadoAcceso Cache::acceder(const AccesoMemoria &acceso, std::size_t indiceTraza, bool registrarStats) {
    const DireccionMemoria bloque = this->calcularBloque(acceso.direccion);
    const IndiceConjunto idxConjunto = this->calcularConjunto(acceso.direccion);
    ConjuntoCache &conjunto = this->conjuntos[idxConjunto];

    EtiquetaBloque tag = 0;
    IndiceConjunto idx = 0;
    DireccionMemoria offset = 0;
    this->descomponerDireccion(acceso.direccion, tag, idx, offset);

    ResultadoAcceso res;
    res.conjunto = idxConjunto;

    if (registrarStats) {
        this->estadisticas.accesosTotales++;
        if (acceso.esEscritura()) {
            this->estadisticas.escrituras++;
        } else {
            this->estadisticas.lecturas++;
        }
    }

    // Busqueda en el mapa hash del conjunto para acceso O(1)
    auto it = conjunto.mapaBloqueVia.find(bloque);
    if (it != conjunto.mapaBloqueVia.end()) {
        // ====================================================================
        // ACIERTO EN CACHE (CACHE HIT)
        // ====================================================================
        res.acierto = true;
        res.via = it->second;
        res.desalojo = false;
        res.clasificacion3C = TipoFallo3C::NINGUNO;

        if (acceso.esEscritura()) {
            conjunto.lineas[res.via].sucia = true; // Politica Write-Back
        }

        // Notificar a la politica de reemplazo
        const ContextoAcceso ctx{bloque, indiceTraza};
        conjunto.politica->registrarAcierto(res.via, ctx);

        // Actualizar clasificador 3C
        this->clasificador.actualizarAcceso(bloque);

        if (registrarStats) {
            this->estadisticas.aciertos++;
        }

        std::ostringstream ss;
        ss << "HIT en Via " << res.via << ". Bloque 0x" << std::hex << bloque 
           << " encontrado en cache SRAM (" << (acceso.esEscritura() ? "marcado como sucio" : "lectura exitosa") << ").";
        res.motivoDecision = ss.str();
        return res;
    }

    // ========================================================================
    // FALLO EN CACHE (CACHE MISS)
    // ========================================================================
    res.acierto = false;

    // Clasificacion del fallo segun el Modelo 3C de Mark D. Hill
    res.clasificacion3C = this->clasificador.clasificarFallo(bloque);
    this->clasificador.actualizarAcceso(bloque);

    if (registrarStats) {
        this->estadisticas.fallos++;
        switch (res.clasificacion3C) {
            case TipoFallo3C::COLD:
                this->estadisticas.fallosCold++;
                break;
            case TipoFallo3C::CAPACITY:
                this->estadisticas.fallosCapacity++;
                break;
            case TipoFallo3C::CONFLICT:
                this->estadisticas.fallosConflict++;
                break;
            case TipoFallo3C::NINGUNO:
                break;
        }
    }

    int viaAsignada = -1;
    std::ostringstream ss;

    // Verificar si existen vias libres en el conjunto utilizando la Pila
    if (!conjunto.viasLibres.esVacia()) {
        viaAsignada = conjunto.viasLibres.getDatoTope();
        conjunto.viasLibres.desapilar();
        res.desalojo = false;

        ss << "MISS [" << getNombreFallo3C(res.clasificacion3C) << "]. "
           << "Asignada via libre " << viaAsignada << " (sin desalojo de lineas).";
    } else {
        // El conjunto esta completamente lleno: se invoca la politica de reemplazo
        viaAsignada = conjunto.politica->seleccionarVictima();
        LineaCache &lineaVieja = conjunto.lineas[viaAsignada];

        res.desalojo = true;
        res.bloqueDesalojado = lineaVieja.bloque;
        res.bloqueDesalojadoSucio = lineaVieja.sucia;

        if (registrarStats) {
            this->estadisticas.desalojosTotales++;
            if (lineaVieja.sucia) {
                this->estadisticas.escriturasMemoria++; // Write-Back obligatorio a DRAM
            }
        }

        // Eliminar el bloque victima de la tabla de mapeo
        conjunto.mapaBloqueVia.erase(lineaVieja.bloque);

        ss << "MISS [" << getNombreFallo3C(res.clasificacion3C) << "]. "
           << "Conjunto lleno. Politica " << this->nombrePolitica 
           << " selecciona victima en Via " << viaAsignada << " (Bloque 0x" 
           << std::hex << lineaVieja.bloque << std::dec << ")"
           << (lineaVieja.sucia ? " [Write-Back a DRAM: linea sucia]" : "") << ".";
    }

    // Colocar el nuevo bloque en la via seleccionada
    conjunto.lineas[viaAsignada] = LineaCache(true, acceso.esEscritura(), bloque, tag);
    conjunto.mapaBloqueVia[bloque] = viaAsignada;

    // Notificar insercion a la politica de reemplazo
    const ContextoAcceso ctx{bloque, indiceTraza};
    conjunto.politica->registrarInsercion(viaAsignada, ctx);

    res.via = viaAsignada;
    res.motivoDecision = ss.str();
    return res;
}

} // namespace cachesim
