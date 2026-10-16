#ifndef GENERADOR_TRAZAS_HPP
#define GENERADOR_TRAZAS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "tipos.hpp"

namespace cachesim {

// Informacion y contenido de una traza con nombre descriptivo
struct TrazaNombrada {
    std::string nombre;
    std::string descripcion;
    std::string tipoDato; // "Sintetico", "Aleatorio", "Real"
    std::vector<AccesoMemoria> accesos;
};

// Parametros para la generacion automatica de trazas
struct ParametrosTraza {
    std::size_t longitud = 200000;
    std::uint64_t semilla = 42;
    std::size_t tamanoCacheBytes = 32 * 1024;
    std::size_t tamanoBloqueBytes = 64;
    double proporcionEscrituras = 0.2; // 20% escrituras, 80% lecturas tipico
};

// Catalogo y generacion de cargas de trabajo
std::vector<std::string> obtenerNombresCargas();
std::string obtenerDescripcionCarga(const std::string &nombre);
std::string obtenerTipoCarga(const std::string &nombre);

// Genera una carga especifica o todas ("all")
std::vector<TrazaNombrada> generarCargasTrabajo(const ParametrosTraza &params,
                                               const std::string &filtro = "all");

// E/S de trazas en disco (Formato: [R|W] 0xDireccion o DireccionDecimal, '#' para comentarios)
bool cargarTrazaDesdeArchivo(const std::string &ruta, std::vector<AccesoMemoria> &traza, std::string &error);
bool guardarTrazaEnArchivo(const std::string &ruta, const std::vector<AccesoMemoria> &traza, std::string &error);

} // namespace cachesim

#endif // GENERADOR_TRAZAS_HPP
