#include "config_cache.hpp"
#include <sstream>
#include <iomanip>

namespace cachesim {

namespace {
// Funciones auxiliares matematicas para calculo de potencias de 2 y logaritmos
bool esPotenciaDeDos(std::size_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

unsigned calcularLog2(std::size_t x) {
    unsigned bits = 0;
    while (x >>= 1) {
        bits++;
    }
    return bits;
}
} // namespace

// ============================================================================
// Implementacion de ConfiguracionCache
// ============================================================================

// constructores
ConfiguracionCache::ConfiguracionCache()
    : sizeBytes(32 * 1024), blockSize(64), ways(4),
      hitLatency(1.0), missPenalty(100.0), porcentajeCalentamiento(0.0) {}

ConfiguracionCache::ConfiguracionCache(std::size_t sizeBytes, std::size_t blockSize, std::size_t ways,
                                       double hitLatency, double missPenalty, double calentamiento)
    : sizeBytes(sizeBytes), blockSize(blockSize), ways(ways),
      hitLatency(hitLatency), missPenalty(missPenalty), porcentajeCalentamiento(calentamiento) {}

unsigned ConfiguracionCache::getBitsOffset() const {
    return calcularLog2(this->blockSize);
}

unsigned ConfiguracionCache::getBitsIndice() const {
    return calcularLog2(this->getNumSets());
}

unsigned ConfiguracionCache::getBitsTag() const {
    // Para direcciones de 64 bits (o 32 segun la arquitectura del procesador)
    const unsigned totalBits = 64;
    return totalBits - this->getBitsOffset() - this->getBitsIndice();
}

bool ConfiguracionCache::validar(std::string &mensajeError) const {
    if (!esPotenciaDeDos(this->blockSize)) {
        mensajeError = "El tamano de bloque debe ser una potencia de 2 (ej: 16, 32, 64, 128).";
        return false;
    }
    if (this->sizeBytes < this->blockSize || (this->sizeBytes % this->blockSize != 0)) {
        mensajeError = "El tamano total de la cache debe ser multiplo del tamano de bloque.";
        return false;
    }
    const std::size_t lineasTotales = this->getNumLineas();
    if (this->ways > lineasTotales) {
        mensajeError = "El numero de vias no puede ser mayor que la cantidad total de lineas.";
        return false;
    }
    const std::size_t viasEff = this->getViasEfectivas();
    if (lineasTotales % viasEff != 0) {
        mensajeError = "Las lineas totales no son divisibles de forma exacta entre las vias.";
        return false;
    }
    if (!esPotenciaDeDos(this->getNumSets())) {
        mensajeError = "El numero de conjuntos (sets) resultante debe ser una potencia de 2.";
        return false;
    }
    if (this->porcentajeCalentamiento < 0.0 || this->porcentajeCalentamiento >= 1.0) {
        mensajeError = "El porcentaje de calentamiento (warm-up) debe estar en el rango [0.0, 1.0).";
        return false;
    }
    return true;
}

std::string ConfiguracionCache::describir() const {
    std::ostringstream ss;
    // Formatear tamano en KB si es multiplo
    if (this->sizeBytes >= 1024 * 1024 && (this->sizeBytes % (1024 * 1024) == 0)) {
        ss << (this->sizeBytes / (1024 * 1024)) << " MB";
    } else if (this->sizeBytes >= 1024 && (this->sizeBytes % 1024 == 0)) {
        ss << (this->sizeBytes / 1024) << " KB";
    } else {
        ss << this->sizeBytes << " B";
    }

    ss << " (Bloque: " << this->blockSize << " B, ";
    if (this->ways == 0) {
        ss << "Totalmente Asociativa";
    } else if (this->ways == 1) {
        ss << "Mapeo Directo";
    } else {
        ss << this->ways << " Vias";
    }
    ss << ", " << this->getNumSets() << " Sets)";
    return ss.str();
}

// ============================================================================
// Implementacion de EstadisticasCache
// ============================================================================

// constructor
EstadisticasCache::EstadisticasCache() {
    this->reiniciar();
}

void EstadisticasCache::reiniciar() {
    this->accesosTotales = 0;
    this->aciertos = 0;
    this->fallos = 0;
    this->lecturas = 0;
    this->escrituras = 0;
    this->fallosCold = 0;
    this->fallosCapacity = 0;
    this->fallosConflict = 0;
    this->desalojosTotales = 0;
    this->escriturasMemoria = 0;
    this->tiempoSegundos = 0.0;
}

double EstadisticasCache::getHitRatio() const {
    if (this->accesosTotales == 0) return 0.0;
    return static_cast<double>(this->aciertos) / static_cast<double>(this->accesosTotales);
}

double EstadisticasCache::getMissRatio() const {
    if (this->accesosTotales == 0) return 0.0;
    return static_cast<double>(this->fallos) / static_cast<double>(this->accesosTotales);
}

double EstadisticasCache::getAmat(const ConfiguracionCache &cfg) const {
    // Average Memory Access Time = Hit_Time + (Miss_Rate * Miss_Penalty)
    return cfg.getHitLatency() + (this->getMissRatio() * cfg.getMissPenalty());
}

double EstadisticasCache::getCiclosStallTotales(const ConfiguracionCache &cfg) const {
    // Total de ciclos perdidos esperando a la DRAM por fallos de cache
    return static_cast<double>(this->fallos) * cfg.getMissPenalty();
}

// ============================================================================
// Funciones del Modelo 3C de Mark D. Hill
// ============================================================================

std::string getNombreFallo3C(TipoFallo3C tipo) {
    switch (tipo) {
        case TipoFallo3C::COLD:
            return "Compulsory (Cold)";
        case TipoFallo3C::CAPACITY:
            return "Capacity";
        case TipoFallo3C::CONFLICT:
            return "Conflict";
        case TipoFallo3C::NINGUNO:
        default:
            return "Acierto (Hit)";
    }
}

std::string getDescripcionDidactica3C(TipoFallo3C tipo) {
    switch (tipo) {
        case TipoFallo3C::COLD:
            return "Fallo Forzoso (Cold): Primera vez que el procesador referencia este bloque en toda la traza. Inevitable aun con cache infinita.";
        case TipoFallo3C::CAPACITY:
            return "Fallo de Capacidad: El bloque ya fue usado antes, pero fue expulsado porque el conjunto de trabajo activo excede la capacidad total de la cache.";
        case TipoFallo3C::CONFLICT:
            return "Fallo de Conflicto: El bloque ya fue usado y cabria en una cache totalmente asociativa del mismo tamano, pero colisiono con otros bloques en el mismo set.";
        case TipoFallo3C::NINGUNO:
        default:
            return "Acierto (Hit): El bloque se encuentra presente en la memoria cache SRAM rapida.";
    }
}

} // namespace cachesim
