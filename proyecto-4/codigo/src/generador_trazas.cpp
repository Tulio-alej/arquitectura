#include "generador_trazas.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace cachesim {

namespace {

using Rng = std::mt19937_64;
using VectorDirecciones = std::vector<DireccionMemoria>;
using GeneradorDirecciones = std::function<VectorDirecciones(const ParametrosTraza&, Rng&)>;

struct DefinicionCarga {
    std::string nombre;
    std::string tipo;
    std::string descripcion;
    GeneradorDirecciones generador;
};

inline DireccionMemoria alinearA8(DireccionMemoria a) {
    return a & ~DireccionMemoria(7);
}

inline DireccionMemoria uniformeEnRango(Rng &r, DireccionMemoria min, DireccionMemoria max) {
    if (min >= max) return min;
    std::uniform_int_distribution<DireccionMemoria> dist(min, max - 1);
    return alinearA8(dist(r));
}

std::vector<DefinicionCarga> catalogoCargas() {
    std::vector<DefinicionCarga> v;

    // ========================================================================
    // DATOS SINTETICOS
    // ========================================================================
    v.push_back({
        "secuencial",
        "Sintetico",
        "Recorrido secuencial streaming de 8x la cache sin reuso temporal.",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs(p.longitud);
            const DireccionMemoria limite = p.tamanoCacheBytes * 8;
            for (std::size_t i = 0; i < p.longitud; ++i) {
                dirs[i] = (i * 8) % limite;
            }
            return dirs;
        }
    });

    v.push_back({
        "bucle_ajustado",
        "Sintetico",
        "Bucle sobre un conjunto de trabajo de 0.5x la cache (cabe en memoria).",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs(p.longitud);
            const DireccionMemoria limite = p.tamanoCacheBytes / 2;
            for (std::size_t i = 0; i < p.longitud; ++i) {
                dirs[i] = (i * 8) % limite;
            }
            return dirs;
        }
    });

    v.push_back({
        "bucle_thrashing",
        "Sintetico",
        "Bucle sobre 1.25x la cache. Patologico para LRU y FIFO (desalojo continuo).",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs(p.longitud);
            const DireccionMemoria limite = p.tamanoCacheBytes + (p.tamanoCacheBytes / 4);
            for (std::size_t i = 0; i < p.longitud; ++i) {
                dirs[i] = (i * p.tamanoBloqueBytes) % limite;
            }
            return dirs;
        }
    });

    v.push_back({
        "zancada",
        "Sintetico",
        "Accesos con salto constante (stride de 128 bytes) simulando acceso a campos estructurados.",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs(p.longitud);
            const DireccionMemoria espacio = p.tamanoCacheBytes * 4;
            DireccionMemoria actual = 0;
            const DireccionMemoria zancada = p.tamanoBloqueBytes * 2;
            for (std::size_t i = 0; i < p.longitud; ++i) {
                dirs[i] = actual;
                actual = (actual + zancada) % espacio;
            }
            return dirs;
        }
    });

    v.push_back({
        "matriz",
        "Sintetico",
        "Recorrido bidimensional simulando acceso a matrices por filas y columnas.",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs;
            dirs.reserve(p.longitud);
            const std::size_t n = 256; // Matriz 256 x 256 enteros de 8 bytes
            while (dirs.size() < p.longitud) {
                // Simula A[i][k] * B[k][j]
                for (std::size_t i = 0; i < n && dirs.size() < p.longitud; ++i) {
                    for (std::size_t j = 0; j < 32 && dirs.size() < p.longitud; ++j) {
                        DireccionMemoria dirA = (i * n + j) * 8;
                        DireccionMemoria dirB = (j * n + i) * 8; // Acceso traspuesto
                        dirs.push_back(dirA);
                        if (dirs.size() < p.longitud) dirs.push_back(dirB);
                    }
                }
            }
            dirs.resize(p.longitud);
            return dirs;
        }
    });

    v.push_back({
        "fases",
        "Sintetico",
        "Simulacion de fases de ejecucion: alterna entre 3 regiones de memoria distintas.",
        [](const ParametrosTraza &p, Rng &) {
            VectorDirecciones dirs(p.longitud);
            const std::size_t longitudFase = p.longitud / 4;
            const DireccionMemoria tamFase = p.tamanoCacheBytes;
            for (std::size_t i = 0; i < p.longitud; ++i) {
                std::size_t fase = (i / longitudFase) % 3;
                DireccionMemoria base = fase * tamFase * 2;
                dirs[i] = base + ((i * 16) % tamFase);
            }
            return dirs;
        }
    });

    // ========================================================================
    // DATOS ALEATORIOS
    // ========================================================================
    v.push_back({
        "uniforme",
        "Aleatorio",
        "Accesos aleatorios uniformes sobre 4x el tamano de la cache.",
        [](const ParametrosTraza &p, Rng &r) {
            VectorDirecciones dirs(p.longitud);
            for (auto &x : dirs) {
                x = uniformeEnRango(r, 0, p.tamanoCacheBytes * 4);
            }
            return dirs;
        }
    });

    v.push_back({
        "gaussiana",
        "Aleatorio",
        "Distribucion normal con media central y sigma = 0.5x cache (alta localidad).",
        [](const ParametrosTraza &p, Rng &r) {
            std::normal_distribution<double> dist(4.0 * p.tamanoCacheBytes, 0.5 * p.tamanoCacheBytes);
            VectorDirecciones dirs(p.longitud);
            for (auto &x : dirs) {
                x = alinearA8(static_cast<DireccionMemoria>(std::max(0.0, dist(r))));
            }
            return dirs;
        }
    });

    v.push_back({
        "zipf",
        "Aleatorio",
        "Distribucion Zipf (ley de potencias, alpha=1). Pocos bloques sumamente populares (ideal para LFU).",
        [](const ParametrosTraza &p, Rng &r) {
            const std::size_t numBloques = std::max<std::size_t>(16, 16 * p.tamanoCacheBytes / p.tamanoBloqueBytes);
            std::vector<double> cdf(numBloques);
            double suma = 0.0;
            for (std::size_t i = 0; i < numBloques; ++i) {
                cdf[i] = (suma += (1.0 / static_cast<double>(i + 1)));
            }

            std::vector<std::size_t> perm(numBloques);
            std::iota(perm.begin(), perm.end(), 0);
            std::shuffle(perm.begin(), perm.end(), r);

            std::uniform_real_distribution<double> distReal(0.0, suma);
            VectorDirecciones dirs(p.longitud);
            for (auto &x : dirs) {
                std::size_t rank = std::lower_bound(cdf.begin(), cdf.end(), distReal(r)) - cdf.begin();
                if (rank >= numBloques) rank = numBloques - 1;
                x = (perm[rank] * p.tamanoBloqueBytes) + uniformeEnRango(r, 0, p.tamanoBloqueBytes);
            }
            return dirs;
        }
    });

    v.push_back({
        "caliente_frio",
        "Aleatorio",
        "Regla 80/20: 80% de accesos van a una region caliente de 0.5x cache, 20% a region fria de 32x.",
        [](const ParametrosTraza &p, Rng &r) {
            std::bernoulli_distribution esCaliente(0.8);
            VectorDirecciones dirs(p.longitud);
            for (auto &x : dirs) {
                if (esCaliente(r)) {
                    x = uniformeEnRango(r, 0, p.tamanoCacheBytes / 2);
                } else {
                    x = uniformeEnRango(r, p.tamanoCacheBytes, p.tamanoCacheBytes * 32);
                }
            }
            return dirs;
        }
    });

    return v;
}

} // namespace

std::vector<std::string> obtenerNombresCargas() {
    std::vector<std::string> nombres;
    for (const auto &c : catalogoCargas()) {
        nombres.push_back(c.nombre);
    }
    return nombres;
}

std::string obtenerDescripcionCarga(const std::string &nombre) {
    for (const auto &c : catalogoCargas()) {
        if (c.nombre == nombre) return c.descripcion;
    }
    return "Carga desconocida";
}

std::string obtenerTipoCarga(const std::string &nombre) {
    for (const auto &c : catalogoCargas()) {
        if (c.nombre == nombre) return c.tipo;
    }
    return "Desconocido";
}

std::vector<TrazaNombrada> generarCargasTrabajo(const ParametrosTraza &params,
                                               const std::string &filtro) {
    auto catalogo = catalogoCargas();
    std::vector<TrazaNombrada> resultado;
    Rng rng(params.semilla);
    std::bernoulli_distribution distEscritura(params.proporcionEscrituras);

    for (const auto &def : catalogo) {
        if (filtro != "all" && filtro != def.nombre) {
            continue;
        }

        VectorDirecciones dirs = def.generador(params, rng);
        TrazaNombrada traza;
        traza.nombre = def.nombre;
        traza.descripcion = def.descripcion;
        traza.tipoDato = def.tipo;
        traza.accesos.resize(dirs.size());

        for (std::size_t i = 0; i < dirs.size(); ++i) {
            traza.accesos[i] = AccesoMemoria(dirs[i], distEscritura(rng));
        }

        resultado.push_back(std::move(traza));
    }

    if (resultado.empty() && filtro != "all") {
        throw std::invalid_argument("La carga de trabajo '" + filtro + "' no existe.");
    }

    return resultado;
}

bool cargarTrazaDesdeArchivo(const std::string &ruta, std::vector<AccesoMemoria> &traza, std::string &error) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        error = "No se pudo abrir el archivo de traza: " + ruta;
        return false;
    }

    traza.clear();
    std::string linea;
    std::size_t numLinea = 0;

    while (std::getline(archivo, linea)) {
        numLinea++;
        // Quitar espacios en blanco al inicio
        std::size_t inicio = linea.find_first_not_of(" \t\r\n");
        if (inicio == std::string::npos || linea[inicio] == '#') {
            continue; // Linea vacia o comentario
        }

        std::istringstream ss(linea.substr(inicio));
        std::string token1, token2;
        ss >> token1;

        TipoOperacion op = TipoOperacion::LECTURA;
        std::string strDir = token1;

        if (token1 == "R" || token1 == "r") {
            op = TipoOperacion::LECTURA;
            ss >> strDir;
        } else if (token1 == "W" || token1 == "w") {
            op = TipoOperacion::ESCRITURA;
            ss >> strDir;
        }

        DireccionMemoria dir = 0;
        try {
            if (strDir.rfind("0x", 0) == 0 || strDir.rfind("0X", 0) == 0) {
                dir = std::stoull(strDir, nullptr, 16);
            } else {
                dir = std::stoull(strDir, nullptr, 10);
            }
        } catch (...) {
            error = "Error de formato en linea " + std::to_string(numLinea) + ": '" + linea + "'";
            return false;
        }

        traza.push_back(AccesoMemoria(dir, op));
    }

    return true;
}

bool guardarTrazaEnArchivo(const std::string &ruta, const std::vector<AccesoMemoria> &traza, std::string &error) {
    std::ofstream archivo(ruta);
    if (!archivo.is_open()) {
        error = "No se pudo crear el archivo para guardar la traza: " + ruta;
        return false;
    }

    archivo << "# Traza de memoria generada por SimuladorCache v2\n";
    archivo << "# Formato: [R|W] 0xDIRECCION\n";
    archivo << "# Total accesos: " << traza.size() << "\n";

    for (const auto &acc : traza) {
        archivo << acc.getSimboloOperacion() << " " << acc.getDireccionHex() << "\n";
    }

    return true;
}

} // namespace cachesim
