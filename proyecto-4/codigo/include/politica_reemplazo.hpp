#ifndef POLITICA_REEMPLAZO_HPP
#define POLITICA_REEMPLAZO_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <random>

#include "tipos.hpp"
#include "cola.hpp" // Libreria del usuario en librerias-lpc/cola.hpp

namespace cachesim {

// Contexto del acceso actual proporcionado a las politicas
struct ContextoAcceso {
    DireccionMemoria bloque;
    std::size_t indiceTraza;
};

// ============================================================================
// Interfaz Abstracta: Patrón Strategy para Políticas de Reemplazo
// ============================================================================
class PoliticaReemplazo {
public:
    virtual ~PoliticaReemplazo() {}

    // Identificador de la politica
    virtual std::string getNombre() const = 0;

    // Notificacion de que la via fue accedida con exito (Hit)
    virtual void registrarAcierto(int via, const ContextoAcceso &ctx) = 0;

    // Notificacion de que un bloque nuevo fue cargado en la via (Miss / Llenado)
    virtual void registrarInsercion(int via, const ContextoAcceso &ctx) = 0;

    // Seleccion del bloque victima para desalojar en tiempo O(1)
    virtual int seleccionarVictima() = 0;

    // Explicacion textual didactica del estado actual para el modo explicativo
    virtual std::string describirEstado() const = 0;
};

// ============================================================================
// Politica LRU (Least Recently Used) - Complejidad O(1)
// ============================================================================
// Mantiene una lista enlazada donde el frente es el mas reciente y la cola
// es el menos recientemente usado, con punteros mapeados para reordenar en O(1).
class PoliticaLRU : public PoliticaReemplazo {
private:
    int totalVias;
    std::list<int> ordenRecencia;
    std::vector<std::list<int>::iterator> posicionVia;
    std::vector<bool> registrada;

    void moverAlFrente(int via);

public:
    // constructor
    explicit PoliticaLRU(int vias);

    // metodos principales
    std::string getNombre() const override { return "LRU"; }
    void registrarAcierto(int via, const ContextoAcceso &ctx) override;
    void registrarInsercion(int via, const ContextoAcceso &ctx) override;
    int seleccionarVictima() override;
    std::string describirEstado() const override;

    // destructor
    virtual ~PoliticaLRU() override {}
};

// ============================================================================
// Politica FIFO (First-In, First-Out) - Complejidad O(1)
// ============================================================================
// Implementada utilizando la clase Cola<T> del usuario (librerias-lpc/cola.hpp).
// El orden de llegada determina el desalojo. Los aciertos no alteran el orden.
class PoliticaFIFO : public PoliticaReemplazo {
private:
    int totalVias;
    Cola<int> colaLlegada;             // Cola de la libreria librerias-lpc
    std::vector<bool> estaEnCola;

public:
    // constructor
    explicit PoliticaFIFO(int vias);

    // metodos principales
    std::string getNombre() const override { return "FIFO"; }
    void registrarAcierto(int via, const ContextoAcceso &ctx) override;
    void registrarInsercion(int via, const ContextoAcceso &ctx) override;
    int seleccionarVictima() override;
    std::string describirEstado() const override;

    // destructor
    virtual ~PoliticaFIFO() override {}
};

// ============================================================================
// Politica LFU (Least Frequently Used) - Complejidad O(1)
// ============================================================================
// Estructura bidimensional entrelazada: cubetas de frecuencias horizontales y
// listas verticales de vias en orden LRU para desempate en O(1).
class PoliticaLFU : public PoliticaReemplazo {
private:
    int totalVias;
    std::uint64_t frecuenciaMinima;
    std::unordered_map<std::uint64_t, std::list<int>> cubetasFrecuencia;
    std::vector<std::uint64_t> frecuencias;
    std::vector<std::list<int>::iterator> posiciones;
    std::vector<bool> registrada;

    void desacoplar(int via);
    void acoplar(int via, std::uint64_t nuevaFreq);

public:
    // constructor
    explicit PoliticaLFU(int vias);

    // metodos principales
    std::string getNombre() const override { return "LFU"; }
    void registrarAcierto(int via, const ContextoAcceso &ctx) override;
    void registrarInsercion(int via, const ContextoAcceso &ctx) override;
    int seleccionarVictima() override;
    std::string describirEstado() const override;

    // destructor
    virtual ~PoliticaLFU() override {}
};

// ============================================================================
// Politica RANDOM (Aleatoria) - Linea Base
// ============================================================================
class PoliticaRandom : public PoliticaReemplazo {
private:
    int totalVias;
    mutable std::mt19937_64 generadorRng;

public:
    // constructor
    PoliticaRandom(int vias, std::uint64_t semilla);

    // metodos principales
    std::string getNombre() const override { return "RANDOM"; }
    void registrarAcierto(int via, const ContextoAcceso &ctx) override;
    void registrarInsercion(int via, const ContextoAcceso &ctx) override;
    int seleccionarVictima() override;
    std::string describirEstado() const override;

    // destructor
    virtual ~PoliticaRandom() override {}
};

// ============================================================================
// Politica OPT (Algoritmo Optimo de Belady / Offline) - Cota Teorica Superior
// ============================================================================
// Desaloja el bloque que sera usado mas lejos en el futuro. Requiere conocer la traza completa.
class PoliticaOPT : public PoliticaReemplazo {
private:
    int totalVias;
    std::vector<std::size_t> proximoUsoVia;
    std::shared_ptr<const std::vector<std::size_t>> tablaSiguienteUso;

public:
    // constructor
    PoliticaOPT(int vias, std::shared_ptr<const std::vector<std::size_t>> siguienteUso);

    // metodos principales
    std::string getNombre() const override { return "OPT"; }
    void registrarAcierto(int via, const ContextoAcceso &ctx) override;
    void registrarInsercion(int via, const ContextoAcceso &ctx) override;
    int seleccionarVictima() override;
    std::string describirEstado() const override;

    // destructor
    virtual ~PoliticaOPT() override {}
};

// ============================================================================
// Entorno y Fabrica de Politicas
// ============================================================================
struct EntornoPolitica {
    std::uint64_t semilla = 42;
    std::shared_ptr<const std::vector<std::size_t>> tablaSiguienteUso;
};

// Fabrica polimorfica para instanciar politicas segun nombre
std::unique_ptr<PoliticaReemplazo> crearPolitica(const std::string &nombre, int vias,
                                                 const EntornoPolitica &entorno);

std::vector<std::string> obtenerPoliticasDisponibles();
bool esPoliticaOffline(const std::string &nombre);

// Precalculo de distancias de proximo uso para OPT
std::vector<std::size_t> construirTablaSiguienteUso(const std::vector<AccesoMemoria> &traza,
                                                    unsigned desplazamientoBloque);

} // namespace cachesim

#endif // POLITICA_REEMPLAZO_HPP
