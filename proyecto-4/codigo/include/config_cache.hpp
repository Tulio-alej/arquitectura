#ifndef CONFIG_CACHE_HPP
#define CONFIG_CACHE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include "tipos.hpp"

namespace cachesim {

// Clasificacion cientifica de los fallos segun el Modelo de las 3C de Mark D. Hill
enum class TipoFallo3C {
    NINGUNO,      // Fue un acierto (HIT)
    COLD,         // Fallo obligatorio (Compulsory / Cold): primer acceso absoluto al bloque
    CAPACITY,     // Fallo de capacidad: el bloque ya fue visto, pero la capacidad total de la cache es insuficiente
    CONFLICT      // Fallo de conflicto: el bloque ya fue visto y cabria en asociatividad total, pero colisiono en el conjunto
};

// Configuracion de los parametros de hardware de la memoria cache
class ConfiguracionCache {
private:
    std::size_t sizeBytes;              // Tamano total en bytes (ej: 32768 = 32KB)
    std::size_t blockSize;              // Tamano del bloque/linea en bytes (ej: 64B)
    std::size_t ways;                   // Grado de asociatividad (0 = totalmente asociativa, 1 = mapeo directo, N = N vias)
    double hitLatency;                  // Latencia de acceso en ciclos cuando hay acierto (default: 1 ciclo)
    double missPenalty;                 // Penalizacion de acceso a DRAM en ciclos cuando hay fallo (default: 100 ciclos)
    double porcentajeCalentamiento;     // Fraccion de la traza para estabilizacion previa (Warm-up, 0.0 a 1.0)

public:
    // constructores
    ConfiguracionCache();
    ConfiguracionCache(std::size_t sizeBytes, std::size_t blockSize, std::size_t ways,
                       double hitLatency = 1.0, double missPenalty = 100.0, double calentamiento = 0.0);

    // getters y setters
    std::size_t getSizeBytes() const { return this->sizeBytes; }
    void setSizeBytes(std::size_t tam) { this->sizeBytes = tam; }

    std::size_t getBlockSize() const { return this->blockSize; }
    void setBlockSize(std::size_t tamBloque) { this->blockSize = tamBloque; }

    std::size_t getWays() const { return this->ways; }
    void setWays(std::size_t vias) { this->ways = vias; }

    double getHitLatency() const { return this->hitLatency; }
    void setHitLatency(double lat) { this->hitLatency = lat; }

    double getMissPenalty() const { return this->missPenalty; }
    void setMissPenalty(double pen) { this->missPenalty = pen; }

    double getPorcentajeCalentamiento() const { return this->porcentajeCalentamiento; }
    void setPorcentajeCalentamiento(double cal) { this->porcentajeCalentamiento = cal; }

    // calculos de arquitectura de cache
    std::size_t getNumLineas() const { return this->sizeBytes / this->blockSize; }
    std::size_t getViasEfectivas() const { return (this->ways == 0) ? this->getNumLineas() : this->ways; }
    std::size_t getNumSets() const { return this->getNumLineas() / this->getViasEfectivas(); }

    unsigned getBitsOffset() const;
    unsigned getBitsIndice() const;
    unsigned getBitsTag() const;

    // validacion y formato
    bool validar(std::string &mensajeError) const;
    std::string describir() const;
};

// Representacion de una linea individual dentro de un conjunto de cache
struct LineaCache {
    bool valida;
    bool sucia;                         // Para politica Write-Back
    DireccionMemoria bloque;
    EtiquetaBloque tag;

    // constructor
    LineaCache() : valida(false), sucia(false), bloque(0), tag(0) {}
    LineaCache(bool valida, bool sucia, DireccionMemoria bloque, EtiquetaBloque tag)
        : valida(valida), sucia(sucia), bloque(bloque), tag(tag) {}
};

// Resultado detallado de una operacion de acceso a la cache
struct ResultadoAcceso {
    bool acierto;
    IndiceConjunto conjunto;
    int via;
    bool desalojo;
    DireccionMemoria bloqueDesalojado;
    bool bloqueDesalojadoSucio;
    TipoFallo3C clasificacion3C;
    std::string motivoDecision;         // Explicacion didactica del reemplazo o asignacion

    // constructor
    ResultadoAcceso()
        : acierto(false), conjunto(0), via(-1), desalojo(false),
          bloqueDesalojado(0), bloqueDesalojadoSucio(false),
          clasificacion3C(TipoFallo3C::NINGUNO), motivoDecision("") {}
};

// Metricas y estadisticas acumuladas durante la simulacion
class EstadisticasCache {
public:
    std::uint64_t accesosTotales;
    std::uint64_t aciertos;
    std::uint64_t fallos;
    std::uint64_t lecturas;
    std::uint64_t escrituras;

    // Clasificacion del modelo 3C de Mark D. Hill
    std::uint64_t fallosCold;           // Fallos compulsorios
    std::uint64_t fallosCapacity;       // Fallos de capacidad
    std::uint64_t fallosConflict;       // Fallos de conflicto

    std::uint64_t desalojosTotales;
    std::uint64_t escriturasMemoria;    // Writebacks a memoria principal por lineas sucias

    double tiempoSegundos;              // Medido con std::chrono::steady_clock

    // constructor
    EstadisticasCache();

    // metodos de metricas
    double getHitRatio() const;
    double getMissRatio() const;
    double getAmat(const ConfiguracionCache &cfg) const;
    double getCiclosStallTotales(const ConfiguracionCache &cfg) const;

    void reiniciar();
};

// Funciones utilitarias de conversion 3C
std::string getNombreFallo3C(TipoFallo3C tipo);
std::string getDescripcionDidactica3C(TipoFallo3C tipo);

} // namespace cachesim

#endif // CONFIG_CACHE_HPP
