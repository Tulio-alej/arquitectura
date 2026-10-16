#ifndef TIPOS_HPP
#define TIPOS_HPP

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

namespace cachesim {

// Tipos basicos para manejo de direcciones y etiquetas en la simulacion
using DireccionMemoria = std::uint64_t;
using EtiquetaBloque   = std::uint64_t;
using IndiceConjunto   = std::uint64_t;

// Tipo de operacion realizada por la CPU
enum class TipoOperacion {
    LECTURA,
    ESCRITURA
};

// Representacion de un acceso a memoria individual de una traza
struct AccesoMemoria {
    DireccionMemoria direccion;
    TipoOperacion operacion;

    // constructores
    AccesoMemoria() : direccion(0), operacion(TipoOperacion::LECTURA) {}
    AccesoMemoria(DireccionMemoria dir, TipoOperacion op) : direccion(dir), operacion(op) {}
    AccesoMemoria(DireccionMemoria dir, bool esEscritura) 
        : direccion(dir), operacion(esEscritura ? TipoOperacion::ESCRITURA : TipoOperacion::LECTURA) {}

    // getters
    bool esEscritura() const { return this->operacion == TipoOperacion::ESCRITURA; }
    bool esLectura() const { return this->operacion == TipoOperacion::LECTURA; }

    std::string getSimboloOperacion() const {
        return this->esEscritura() ? "W" : "R";
    }

    std::string getDireccionHex() const {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << this->direccion;
        return ss.str();
    }
};

} // namespace cachesim

#endif // TIPOS_HPP
