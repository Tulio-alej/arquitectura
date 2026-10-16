#include "reporteador.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

namespace cachesim {

namespace {

std::vector<std::string> cargasUnicas(const std::vector<FilaExperimento> &filas) {
    std::vector<std::string> lista;
    for (const auto &f : filas) {
        if (std::find(lista.begin(), lista.end(), f.nombreCarga) == lista.end()) {
            lista.push_back(f.nombreCarga);
        }
    }
    return lista;
}

std::vector<std::string> politicasUnicas(const std::vector<FilaExperimento> &filas) {
    std::vector<std::string> lista;
    for (const auto &f : filas) {
        if (std::find(lista.begin(), lista.end(), f.resultado.politica) == lista.end()) {
            lista.push_back(f.resultado.politica);
        }
    }
    return lista;
}

std::string formatoPorcentaje(double ratio) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << (ratio * 100.0) << "%";
    return ss.str();
}

} // namespace

std::string Reporteador::generarBarraProgreso(double proporcion, int ancho) {
    proporcion = std::max(0.0, std::min(1.0, proporcion));
    const int numLlenos = static_cast<int>(std::round(proporcion * ancho));
    const int numVacios = ancho - numLlenos;
    return "[" + std::string(numLlenos, '#') + std::string(numVacios, '.') + "]";
}

void Reporteador::imprimirComparacion(std::ostream &os, const std::vector<FilaExperimento> &filas) {
    const auto cargas = cargasUnicas(filas);

    for (const auto &nomCarga : cargas) {
        const FilaExperimento *primeraFila = nullptr;
        double mejorHitOnline = -1.0;

        for (const auto &f : filas) {
            if (f.nombreCarga == nomCarga) {
                if (!primeraFila) primeraFila = &f;
                if (!esPoliticaOffline(f.resultado.politica)) {
                    mejorHitOnline = std::max(mejorHitOnline, f.resultado.estadisticas.getHitRatio());
                }
            }
        }

        os << "\n=========================================================================================================\n";
        os << " CARGA: " << nomCarga << " [" << primeraFila->tipoCarga << "]\n";
        os << " Config: " << primeraFila->resultado.configuracion.describir() 
           << " | Total Accesos: " << primeraFila->resultado.estadisticas.accesosTotales << "\n";
        os << "---------------------------------------------------------------------------------------------------------\n";
        os << std::left << std::setw(9) << "Politica"
           << std::right << std::setw(9) << "Aciertos"
           << std::setw(9) << "Fallos"
           << std::setw(9) << "Hit%"
           << std::setw(8) << "Cold"
           << std::setw(8) << "Cap."
           << std::setw(8) << "Conf."
           << std::setw(8) << "Desal."
           << std::setw(6) << "WB"
           << std::setw(8) << "AMAT(c)"
           << std::setw(11) << "Stalls(c)"
           << std::setw(9) << "t(ms)"
           << "  Visual Hit Ratio\n";
        os << "---------------------------------------------------------------------------------------------------------\n";

        for (const auto &f : filas) {
            if (f.nombreCarga != nomCarga) continue;
            const auto &s = f.resultado.estadisticas;
            const bool esMejor = !esPoliticaOffline(f.resultado.politica) && 
                                 (std::abs(s.getHitRatio() - mejorHitOnline) < 1e-9);

            os << std::left << std::setw(8) << f.resultado.politica << (esMejor ? "*" : " ")
               << std::right
               << std::setw(9) << s.aciertos
               << std::setw(9) << s.fallos
               << std::setw(9) << formatoPorcentaje(s.getHitRatio())
               << std::setw(8) << s.fallosCold
               << std::setw(8) << s.fallosCapacity
               << std::setw(8) << s.fallosConflict
               << std::setw(8) << s.desalojosTotales
               << std::setw(6) << s.escriturasMemoria
               << std::setw(8) << std::fixed << std::setprecision(2) << f.resultado.amat
               << std::setw(11) << static_cast<std::uint64_t>(f.resultado.ciclosStall)
               << std::setw(9) << std::setprecision(3) << (f.resultado.segundos * 1000.0) << "  "
               << generarBarraProgreso(s.getHitRatio()) << "\n";
        }
    }
    os << "---------------------------------------------------------------------------------------------------------\n";
    os << " (*) Mejor politica online evaluada. AMAT y Stalls en ciclos. 'opt' = cota teorica de Belady (offline).\n\n";
}

void Reporteador::imprimirMatrizResumen(std::ostream &os, const std::vector<FilaExperimento> &filas) {
    const auto cargas = cargasUnicas(filas);
    const auto politicas = politicasUnicas(filas);

    os << "\n===============================================================================\n";
    os << "                  MATRIZ RESUMEN: HIT RATIO POR CARGA Y POLITICA               \n";
    os << "===============================================================================\n";
    os << std::left << std::setw(18) << "Carga Evaluada";
    for (const auto &p : politicas) {
        os << std::right << std::setw(10) << p;
    }
    os << "   Ganadora\n";
    os << "-------------------------------------------------------------------------------\n";

    std::map<std::string, int> victorias;

    for (const auto &c : cargas) {
        os << std::left << std::setw(18) << c;
        std::string ganadora;
        double mejorRatio = -1.0;

        for (const auto &p : politicas) {
            double hitRatio = 0.0;
            for (const auto &f : filas) {
                if (f.nombreCarga == c && f.resultado.politica == p) {
                    hitRatio = f.resultado.estadisticas.getHitRatio();
                    break;
                }
            }
            os << std::right << std::setw(10) << formatoPorcentaje(hitRatio);

            if (!esPoliticaOffline(p) && hitRatio > mejorRatio + 1e-9) {
                mejorRatio = hitRatio;
                ganadora = p;
            }
        }

        // Revisar si hubo empate entre politicas online
        int contEmpate = 0;
        for (const auto &p : politicas) {
            for (const auto &f : filas) {
                if (f.nombreCarga == c && f.resultado.politica == p && !esPoliticaOffline(p)) {
                    if (std::abs(f.resultado.estadisticas.getHitRatio() - mejorRatio) < 1e-9) {
                        contEmpate++;
                    }
                }
            }
        }

        if (contEmpate > 1) {
            ganadora = "Empate";
        } else {
            victorias[ganadora]++;
        }

        os << "   " << ganadora << "\n";
    }

    os << "===============================================================================\n";
    os << " Victorias por Politica Online:\n";
    for (const auto &par : victorias) {
        os << "   * " << par.first << " : " << par.second << " victorias\n";
    }
    os << "===============================================================================\n\n";
}

void Reporteador::imprimirMatrizEstructural(std::ostream &os) {
    os << "\n===============================================================================\n";
    os << "         MATRIZ DE COMPARACION ESTRUCTURAL DE POLITICAS (PROYECTO 4)           \n";
    os << "===============================================================================\n";
    os << std::left << std::setw(25) << "Dimension Tecnica" 
       << std::setw(16) << "LRU" 
       << std::setw(18) << "FIFO" 
       << std::setw(20) << "LFU" << "\n";
    os << "-------------------------------------------------------------------------------\n";
    os << std::left << std::setw(25) << "Complejidad Acceso" << std::setw(16) << "O(1)" << std::setw(18) << "O(1)" << std::setw(20) << "O(1)" << "\n";
    os << std::left << std::setw(25) << "Complejidad Desalojo" << std::setw(16) << "O(1)" << std::setw(18) << "O(1)" << std::setw(20) << "O(1)" << "\n";
    os << std::left << std::setw(25) << "Overhead Memoria" << std::setw(16) << "Moderado (Punteros)" << std::setw(18) << "Bajo (Cola)" << std::setw(20) << "Alto (Cubetas Freq)" << "\n";
    os << std::left << std::setw(25) << "Sensibilidad Secuencial" << std::setw(16) << "Alta (Degradacion)" << std::setw(18) << "Vulnerable (Belady)" << std::setw(20) << "Robusta (F=1)" << "\n";
    os << std::left << std::setw(25) << "Respuesta a Bucles" << std::setw(16) << "Optima si cabe" << std::setw(18) << "Pobre (Expulsion)" << std::setw(20) << "Inercia Frecuencia" << "\n";
    os << "===============================================================================\n\n";
}

void Reporteador::imprimirBarrido(std::ostream &os, const std::vector<FilaExperimento> &filas) {
    if (filas.empty()) return;

    os << "\n=========================================================================================================\n";
    os << " RESULTADOS DEL BARRIDO PARAMETRICO (SWEEP) | Carga: " << filas.front().nombreCarga << "\n";
    os << "---------------------------------------------------------------------------------------------------------\n";
    os << std::left << std::setw(20) << "Parametro"
       << std::setw(9) << "Politica"
       << std::right << std::setw(10) << "Aciertos"
       << std::setw(10) << "Fallos"
       << std::setw(10) << "Hit%"
       << std::setw(10) << "AMAT(c)"
       << std::setw(10) << "t(ms)"
       << "  Barra de Rendimiento\n";
    os << "---------------------------------------------------------------------------------------------------------\n";

    for (const auto &f : filas) {
        const auto &s = f.resultado.estadisticas;
        os << std::left << std::setw(20) << f.etiquetaParametro
           << std::setw(9) << f.resultado.politica
           << std::right
           << std::setw(10) << s.aciertos
           << std::setw(10) << s.fallos
           << std::setw(10) << formatoPorcentaje(s.getHitRatio())
           << std::setw(10) << std::fixed << std::setprecision(2) << f.resultado.amat
           << std::setw(10) << std::setprecision(3) << (f.resultado.segundos * 1000.0) << "  "
           << generarBarraProgreso(s.getHitRatio()) << "\n";
    }
    os << "=========================================================================================================\n\n";
}

bool Reporteador::exportarCsv(const std::string &ruta, const std::vector<FilaExperimento> &filas, std::string &error) {
    std::ofstream archivo(ruta);
    if (!archivo.is_open()) {
        error = "No se pudo abrir el archivo CSV para escritura: " + ruta;
        return false;
    }

    archivo << "carga,tipo,parametro,politica,tamano_cache_bytes,bloque_bytes,vias,accesos,aciertos,fallos,"
            << "hit_ratio,miss_ratio,fallos_cold,fallos_capacity,fallos_conflict,desalojos,writebacks,"
            << "amat_ciclos,stalls_ciclos,tiempo_ms\n";

    for (const auto &f : filas) {
        const auto &s = f.resultado.estadisticas;
        const auto &c = f.resultado.configuracion;

        archivo << f.nombreCarga << ","
                << f.tipoCarga << ","
                << f.etiquetaParametro << ","
                << f.resultado.politica << ","
                << c.getSizeBytes() << ","
                << c.getBlockSize() << ","
                << c.getWays() << ","
                << s.accesosTotales << ","
                << s.aciertos << ","
                << s.fallos << ","
                << std::fixed << std::setprecision(6) << s.getHitRatio() << ","
                << s.getMissRatio() << ","
                << s.fallosCold << ","
                << s.fallosCapacity << ","
                << s.fallosConflict << ","
                << s.desalojosTotales << ","
                << s.escriturasMemoria << ","
                << std::setprecision(2) << f.resultado.amat << ","
                << static_cast<std::uint64_t>(f.resultado.ciclosStall) << ","
                << std::setprecision(4) << (f.resultado.segundos * 1000.0) << "\n";
    }

    return true;
}

} // namespace cachesim
