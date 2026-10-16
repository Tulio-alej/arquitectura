#include "politica_reemplazo.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace cachesim {

// ============================================================================
// Implementacion de PoliticaLRU
// ============================================================================

PoliticaLRU::PoliticaLRU(int vias) : totalVias(vias), posicionVia(vias), registrada(vias, false) {}

void PoliticaLRU::moverAlFrente(int via) {
    if (this->registrada[via]) {
        this->ordenRecencia.erase(this->posicionVia[via]);
    }
    // El frente de la lista representa el elemento mas recientemente usado (MRU)
    this->ordenRecencia.push_front(via);
    this->posicionVia[via] = this->ordenRecencia.begin();
    this->registrada[via] = true;
}

void PoliticaLRU::registrarAcierto(int via, const ContextoAcceso &) {
    // En LRU, un acierto renueva la recencia del bloque, moviendolo al frente
    this->moverAlFrente(via);
}

void PoliticaLRU::registrarInsercion(int via, const ContextoAcceso &) {
    // Al insertar un nuevo bloque, entra al frente como el mas reciente
    this->moverAlFrente(via);
}

int PoliticaLRU::seleccionarVictima() {
    // El fondo de la lista contiene el bloque que lleva mas tiempo sin usarse (LRU)
    if (this->ordenRecencia.empty()) {
        return 0;
    }
    return this->ordenRecencia.back();
}

std::string PoliticaLRU::describirEstado() const {
    std::ostringstream ss;
    ss << "Orden LRU [MRU -> LRU]: ";
    bool primero = true;
    for (int via : this->ordenRecencia) {
        if (!primero) ss << " -> ";
        ss << "Via " << via;
        primero = false;
    }
    if (!this->ordenRecencia.empty()) {
        ss << " | Victima sugerida: Via " << this->ordenRecencia.back();
    }
    return ss.str();
}

// ============================================================================
// Implementacion de PoliticaFIFO
// ============================================================================

PoliticaFIFO::PoliticaFIFO(int vias) : totalVias(vias), estaEnCola(vias, false) {}

void PoliticaFIFO::registrarAcierto(int, const ContextoAcceso &) {
    // Caracteristica clave de FIFO: los aciertos NO modifican el orden de llegada.
    // La antiguedad temporal de ingreso se preserva de manera estricta.
}

void PoliticaFIFO::registrarInsercion(int via, const ContextoAcceso &) {
    // Si la via ya estaba registrada en la cola, evitamos duplicidad
    if (!this->estaEnCola[via]) {
        this->colaLlegada.encolar(via);
        this->estaEnCola[via] = true;
    }
}

int PoliticaFIFO::seleccionarVictima() {
    // En FIFO, la victima es exactamente el primer elemento que entro a la cola
    if (this->colaLlegada.esVacia()) {
        return 0;
    }
    int victima = this->colaLlegada.getFirst();
    this->colaLlegada.desencolar();
    this->estaEnCola[victima] = false;
    return victima;
}

std::string PoliticaFIFO::describirEstado() const {
    std::ostringstream ss;
    ss << "Cola FIFO (Llegada): ";
    if (this->colaLlegada.esVacia()) {
        ss << "[Vacia]";
    } else {
        ss << "Frente (Antiguo) = Via " << this->colaLlegada.getFirst() 
           << ", Ultimo = Via " << this->colaLlegada.getLast()
           << " (Tamano cola: " << this->colaLlegada.getSize() << ")";
    }
    return ss.str();
}

// ============================================================================
// Implementacion de PoliticaLFU
// ============================================================================

PoliticaLFU::PoliticaLFU(int vias)
    : totalVias(vias), frecuenciaMinima(1), frecuencias(vias, 0),
      posiciones(vias), registrada(vias, false) {}

void PoliticaLFU::desacoplar(int via) {
    const std::uint64_t freqActual = this->frecuencias[via];
    auto itCubeta = this->cubetasFrecuencia.find(freqActual);
    if (itCubeta != this->cubetasFrecuencia.end()) {
        itCubeta->second.erase(this->posiciones[via]);
        if (itCubeta->second.empty()) {
            this->cubetasFrecuencia.erase(itCubeta);
            // Si la cubeta vaciada era la de frecuencia minima, avanzamos
            if (this->frecuenciaMinima == freqActual) {
                this->frecuenciaMinima = freqActual + 1;
            }
        }
    }
    this->registrada[via] = false;
}

void PoliticaLFU::acoplar(int via, std::uint64_t nuevaFreq) {
    auto &cubeta = this->cubetasFrecuencia[nuevaFreq];
    cubeta.push_back(via); // Agregamos al final de la cubeta (criterio LRU para desempate)
    this->posiciones[via] = std::prev(cubeta.end());
    this->frecuencias[via] = nuevaFreq;
    this->registrada[via] = true;
}

void PoliticaLFU::registrarAcierto(int via, const ContextoAcceso &) {
    // Al haber acierto, incrementamos la frecuencia en O(1)
    const std::uint64_t freqActual = this->frecuencias[via];
    this->desacoplar(via);
    this->acoplar(via, freqActual + 1);
}

void PoliticaLFU::registrarInsercion(int via, const ContextoAcceso &) {
    // Al insertar un nuevo bloque, su frecuencia inicial es 1
    if (this->registrada[via]) {
        this->desacoplar(via);
    }
    this->acoplar(via, 1);
    this->frecuenciaMinima = 1;
}

int PoliticaLFU::seleccionarVictima() {
    // Seleccionamos el bloque con menor frecuencia acumulada
    auto itMin = this->cubetasFrecuencia.find(this->frecuenciaMinima);
    if (itMin != this->cubetasFrecuencia.end() && !itMin->second.empty()) {
        return itMin->second.front();
    }
    // Salvaguarda: si por alguna razon la cubeta minima no se encuentra, buscar la menor
    if (!this->cubetasFrecuencia.empty()) {
        auto itPrimero = this->cubetasFrecuencia.begin();
        this->frecuenciaMinima = itPrimero->first;
        return itPrimero->second.front();
    }
    return 0;
}

std::string PoliticaLFU::describirEstado() const {
    std::ostringstream ss;
    ss << "LFU [Freq Minima = " << this->frecuenciaMinima << "]: ";
    for (int v = 0; v < this->totalVias; ++v) {
        if (this->registrada[v]) {
            ss << "[V" << v << ": F=" << this->frecuencias[v] << "] ";
        }
    }
    auto itMin = this->cubetasFrecuencia.find(this->frecuenciaMinima);
    if (itMin != this->cubetasFrecuencia.end() && !itMin->second.empty()) {
        ss << "| Victima: Via " << itMin->second.front();
    }
    return ss.str();
}

// ============================================================================
// Implementacion de PoliticaRandom
// ============================================================================

PoliticaRandom::PoliticaRandom(int vias, std::uint64_t semilla)
    : totalVias(vias), generadorRng(semilla) {}

void PoliticaRandom::registrarAcierto(int, const ContextoAcceso &) {}
void PoliticaRandom::registrarInsercion(int, const ContextoAcceso &) {}

int PoliticaRandom::seleccionarVictima() {
    std::uniform_int_distribution<int> dist(0, this->totalVias - 1);
    return dist(this->generadorRng);
}

std::string PoliticaRandom::describirEstado() const {
    return "Random: Seleccion pseudoaleatoria entre vias [0.." + std::to_string(this->totalVias - 1) + "]";
}

// ============================================================================
// Implementacion de PoliticaOPT (Belady)
// ============================================================================

PoliticaOPT::PoliticaOPT(int vias, std::shared_ptr<const std::vector<std::size_t>> siguienteUso)
    : totalVias(vias), proximoUsoVia(vias, 0), tablaSiguienteUso(siguienteUso) {}

void PoliticaOPT::registrarAcierto(int via, const ContextoAcceso &ctx) {
    if (this->tablaSiguienteUso && ctx.indiceTraza < this->tablaSiguienteUso->size()) {
        this->proximoUsoVia[via] = (*this->tablaSiguienteUso)[ctx.indiceTraza];
    }
}

void PoliticaOPT::registrarInsercion(int via, const ContextoAcceso &ctx) {
    if (this->tablaSiguienteUso && ctx.indiceTraza < this->tablaSiguienteUso->size()) {
        this->proximoUsoVia[via] = (*this->tablaSiguienteUso)[ctx.indiceTraza];
    }
}

int PoliticaOPT::seleccionarVictima() {
    // Desaloja la linea cuyo siguiente uso se encuentre mas alejado en el futuro
    auto itMax = std::max_element(this->proximoUsoVia.begin(), this->proximoUsoVia.end());
    return static_cast<int>(std::distance(this->proximoUsoVia.begin(), itMax));
}

std::string PoliticaOPT::describirEstado() const {
    std::ostringstream ss;
    ss << "OPT (Belady): Proximos usos por via -> ";
    for (int v = 0; v < this->totalVias; ++v) {
        ss << "[V" << v << ": paso " << this->proximoUsoVia[v] << "] ";
    }
    return ss.str();
}

// ============================================================================
// Fabrica de Politicas
// ============================================================================

std::unique_ptr<PoliticaReemplazo> crearPolitica(const std::string &nombre, int vias,
                                                 const EntornoPolitica &entorno) {
    std::string n = nombre;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);

    if (n == "lru") {
        return std::make_unique<PoliticaLRU>(vias);
    } else if (n == "fifo") {
        return std::make_unique<PoliticaFIFO>(vias);
    } else if (n == "lfu") {
        return std::make_unique<PoliticaLFU>(vias);
    } else if (n == "random") {
        return std::make_unique<PoliticaRandom>(vias, entorno.semilla);
    } else if (n == "opt") {
        return std::make_unique<PoliticaOPT>(vias, entorno.tablaSiguienteUso);
    }
    throw std::invalid_argument("Politica de reemplazo desconocida: '" + nombre + "'. Use: lru, fifo, lfu, random, opt");
}

std::vector<std::string> obtenerPoliticasDisponibles() {
    return {"lru", "fifo", "lfu", "random", "opt"};
}

bool esPoliticaOffline(const std::string &nombre) {
    std::string n = nombre;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    return n == "opt";
}

std::vector<std::size_t> construirTablaSiguienteUso(const std::vector<AccesoMemoria> &traza,
                                                    unsigned desplazamientoBloque) {
    const std::size_t total = traza.size();
    std::vector<std::size_t> tabla(total, std::numeric_limits<std::size_t>::max());
    std::unordered_map<DireccionMemoria, std::size_t> ultimoPasoPorBloque;

    // Recorrido de atras hacia adelante para conocer el proximo acceso en O(N)
    for (std::size_t i = total; i > 0; --i) {
        const std::size_t idx = i - 1;
        const DireccionMemoria bloque = traza[idx].direccion >> desplazamientoBloque;
        auto it = ultimoPasoPorBloque.find(bloque);
        if (it != ultimoPasoPorBloque.end()) {
            tabla[idx] = it->second;
        }
        ultimoPasoPorBloque[bloque] = idx;
    }
    return tabla;
}

} // namespace cachesim
