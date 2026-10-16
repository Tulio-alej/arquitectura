#ifndef REPORTEADOR_HPP
#define REPORTEADOR_HPP

#include <string>
#include <vector>
#include <ostream>

#include "simulador.hpp"

namespace cachesim {

// ============================================================================
// Generador de Informes, Tablas y Graficos de Rendimiento
// ============================================================================
class Reporteador {
public:
    // constructor
    Reporteador() {}

    // Visualizacion de barra grafica ASCII de porcentaje
    static std::string generarBarraProgreso(double proporcion, int ancho = 20);

    // Muestra tabla comparativa completa por cada carga evaluada
    static void imprimirComparacion(std::ostream &os, const std::vector<FilaExperimento> &filas);

    // Muestra matriz sintetica de hit ratio y politicas ganadoras
    static void imprimirMatrizResumen(std::ostream &os, const std::vector<FilaExperimento> &filas);

    // Muestra la matriz de comparacion estructural teorica (Infografia Proyecto 4)
    static void imprimirMatrizEstructural(std::ostream &os);

    // Muestra tabla de resultados de un barrido de parametros (Sweep)
    static void imprimirBarrido(std::ostream &os, const std::vector<FilaExperimento> &filas);

    // Exporta los resultados completos a un archivo CSV estructurado
    static bool exportarCsv(const std::string &ruta, const std::vector<FilaExperimento> &filas, std::string &error);

    // destructor
    ~Reporteador() {}
};

} // namespace cachesim

#endif // REPORTEADOR_HPP
