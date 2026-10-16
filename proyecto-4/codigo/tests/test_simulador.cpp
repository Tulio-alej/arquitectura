#include <iostream>
#include <cstdlib>
#include <cassert>
#include <vector>

#include "cache.hpp"
#include "simulador.hpp"
#include "generador_trazas.hpp"

using namespace cachesim;

namespace {

int pruebasFallidas = 0;

#define AFIRMAR(cond, mensaje) \
    do { \
        if (!(cond)) { \
            std::cerr << " [FALLO] Linea " << __LINE__ << ": " << mensaje << " (" #cond ")\n"; \
            pruebasFallidas++; \
        } else { \
            std::cout << " [OK] " << mensaje << "\n"; \
        } \
    } while (0)

ConfiguracionCache crearCachePequena(std::size_t vias) {
    ConfiguracionCache c;
    c.setBlockSize(64);
    c.setSizeBytes(vias * 64); // 1 solo conjunto con 'vias' vias
    c.setWays(vias);
    return c;
}

std::vector<AccesoMemoria> crearSecuenciaBloques(std::initializer_list<int> bloques) {
    std::vector<AccesoMemoria> traza;
    for (int b : bloques) {
        traza.push_back(AccesoMemoria(static_cast<DireccionMemoria>(b * 64), TipoOperacion::LECTURA));
    }
    return traza;
}

} // namespace

void testPoliticasBasicas() {
    std::cout << "\n========================================\n";
    std::cout << " 1. Pruebas de Politicas: LRU, FIFO, LFU\n";
    std::cout << "========================================\n";

    // Secuencia clasica: 1, 2, 3, 1, 4, 1, 2 con cache de 3 vias (1 conjunto)
    // Pasos esperados:
    // 1 -> Miss (Cold), carga via 0
    // 2 -> Miss (Cold), carga via 1
    // 3 -> Miss (Cold), carga via 2 (cache llena: [1, 2, 3])
    // 1 -> Hit!
    //   LRU: orden pasa a ser [2, 3, 1] (2 es LRU)
    //   FIFO: orden sigue siendo [1, 2, 3] (1 es el mas antiguo)
    //   LFU: bloque 1 tiene freq=2; bloques 2 y 3 tienen freq=1 (2 mas antiguo con f=1)
    // 4 -> Miss!
    //   LRU: desaloja 2 -> cache queda con [3, 1, 4]
    //   FIFO: desaloja 1 -> cache queda con [2, 3, 4]
    //   LFU: desaloja 2 (menor freq=1 mas antiguo) -> cache queda con [1, 3, 4]
    // 1 ->
    //   LRU: Hit! (1 estaba en cache)
    //   FIFO: Miss! (1 habia sido desalojado)
    //   LFU: Hit! (1 estaba en cache con freq=2)
    // 2 ->
    //   LRU: Miss! (2 habia sido desalojado)
    //   FIFO: Hit! (2 aun estaba en cache)
    //   LFU: Miss! (2 habia sido desalojado)
    // Total aciertos: LRU = 2, FIFO = 2, LFU = 2

    const auto traza = crearSecuenciaBloques({1, 2, 3, 1, 4, 1, 2});
    SimuladorCache sim;

    auto rLRU = sim.ejecutarSimulacion(crearCachePequena(3), "lru", traza);
    AFIRMAR(rLRU.estadisticas.aciertos == 2, "LRU obtiene 2 aciertos en secuencia clasica");

    auto rFIFO = sim.ejecutarSimulacion(crearCachePequena(3), "fifo", traza);
    AFIRMAR(rFIFO.estadisticas.aciertos == 1, "FIFO obtiene 1 acierto en secuencia clasica (desaloja el bloque 1)");

    auto rLFU = sim.ejecutarSimulacion(crearCachePequena(3), "lfu", traza);
    AFIRMAR(rLFU.estadisticas.aciertos == 2, "LFU obtiene 2 aciertos en secuencia clasica");
}

void testAnomaliaBeladyFIFO() {
    std::cout << "\n========================================\n";
    std::cout << " 2. Prueba de la Anomalia de Belady (FIFO)\n";
    std::cout << "========================================\n";

    // Secuencia de Belady: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5
    // Con 3 vias: 9 fallos
    // Con 4 vias: 10 fallos (anomalia: mas capacidad pero mas fallos en FIFO)
    const auto traza = crearSecuenciaBloques({1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5});
    SimuladorCache sim;

    auto rFIFO3 = sim.ejecutarSimulacion(crearCachePequena(3), "fifo", traza);
    auto rFIFO4 = sim.ejecutarSimulacion(crearCachePequena(4), "fifo", traza);

    AFIRMAR(rFIFO3.estadisticas.fallos == 9, "FIFO con 3 vias genera exactamente 9 fallos");
    AFIRMAR(rFIFO4.estadisticas.fallos == 10, "FIFO con 4 vias genera exactamente 10 fallos (Anomalia comprobada)");
}

void testPreservacionFrecuenciaLFU() {
    std::cout << "\n========================================\n";
    std::cout << " 3. Prueba de Inercia de Frecuencia (LFU)\n";
    std::cout << "========================================\n";

    // Bloque 1 se accede 4 veces -> Frecuencia alta (f=4)
    // Luego se accede a bloques 2, 3, 4 (desalojando uno)
    // Luego se vuelve a acceder a 1.
    // LFU deberia conservar el bloque 1 porque su frecuencia es superior.
    const auto traza = crearSecuenciaBloques({1, 1, 1, 1, 2, 3, 4, 1});
    SimuladorCache sim;

    auto rLFU = sim.ejecutarSimulacion(crearCachePequena(3), "lfu", traza);
    auto rLRU = sim.ejecutarSimulacion(crearCachePequena(3), "lru", traza);

    AFIRMAR(rLFU.estadisticas.aciertos == 4, "LFU preserva el bloque altamente frecuente y acierta 4 veces");
    AFIRMAR(rLRU.estadisticas.aciertos == 3, "LRU desaloja el bloque 1 tras la rafaga de 2,3,4 y obtiene 3 aciertos");
}

void testClasificacion3CHill() {
    std::cout << "\n========================================\n";
    std::cout << " 4. Prueba del Modelo 3C de Mark D. Hill\n";
    std::cout << "========================================\n";

    // Primera vez que se accede a 3 bloques distintos -> Todos deben ser COLD (Compulsorios)
    ConfiguracionCache cfg;
    cfg.setSizeBytes(128); // 2 lineas de 64B
    cfg.setBlockSize(64);
    cfg.setWays(1);        // 2 sets de 1 via (mapeo directo)

    Cache cache(cfg, "lru");

    // Acceso 1 (bloque 0 -> set 0)
    ResultadoAcceso res1 = cache.acceder(AccesoMemoria(0, TipoOperacion::LECTURA), 0, true);
    AFIRMAR(res1.clasificacion3C == TipoFallo3C::COLD, "Primer acceso es fallo Compulsorio (Cold)");

    // Acceso 2 (bloque 2 -> set 0 colisiona con bloque 0!)
    ResultadoAcceso res2 = cache.acceder(AccesoMemoria(128, TipoOperacion::LECTURA), 1, true);
    AFIRMAR(res2.clasificacion3C == TipoFallo3C::COLD, "Segundo acceso a bloque nuevo es Compulsorio (Cold)");

    // Acceso 3 (bloque 0 de nuevo):
    // En mapeo directo de 2 sets, bloque 0 y bloque 2 colisionan en Set 0.
    // Pero en una cache totalmente asociativa de 2 lineas, ambos cabrian juntos!
    // Por lo tanto, debe clasificarse cientificamente como fallo de CONFLICTO.
    ResultadoAcceso res3 = cache.acceder(AccesoMemoria(0, TipoOperacion::LECTURA), 2, true);
    AFIRMAR(res3.clasificacion3C == TipoFallo3C::CONFLICT, "Fallo por colision en mapeo directo clasificado como CONFLICTO");
}

void testWriteBackLineasSucias() {
    std::cout << "\n========================================\n";
    std::cout << " 5. Prueba de Politica Write-Back\n";
    std::cout << "========================================\n";

    // Cache de 1 sola linea de 64 bytes
    ConfiguracionCache cfg = crearCachePequena(1);
    Cache cache(cfg, "lru");

    // Escritura en bloque 0 -> se marca como sucia
    cache.acceder(AccesoMemoria(0, TipoOperacion::ESCRITURA), 0, true);

    // Lectura de bloque 1 -> desaloja bloque 0 sucio -> debe provocar 1 Write-back a DRAM
    ResultadoAcceso res = cache.acceder(AccesoMemoria(64, TipoOperacion::LECTURA), 1, true);

    AFIRMAR(res.desalojo == true, "Ocurrio desalojo de linea");
    AFIRMAR(res.bloqueDesalojadoSucio == true, "La linea desalojada estaba sucia");
    AFIRMAR(cache.getEstadisticas().escriturasMemoria == 1, "Se registro 1 Write-Back a memoria principal");
}

void testValidacionesConfiguracion() {
    std::cout << "\n========================================\n";
    std::cout << " 6. Validacion de Parametros de Hardware\n";
    std::cout << "========================================\n";

    std::string err;
    ConfiguracionCache cfgInvalida1;
    cfgInvalida1.setBlockSize(47); // No es potencia de 2
    AFIRMAR(!cfgInvalida1.validar(err), "Detecta tamano de bloque que no es potencia de 2");

    ConfiguracionCache cfgInvalida2;
    cfgInvalida2.setSizeBytes(100);
    cfgInvalida2.setBlockSize(64); // Tamano no es multiplo de bloque
    AFIRMAR(!cfgInvalida2.validar(err), "Detecta tamano de cache no multiplo del bloque");

    ConfiguracionCache cfgValida;
    cfgValida.setSizeBytes(32 * 1024);
    cfgValida.setBlockSize(64);
    cfgValida.setWays(4);
    AFIRMAR(cfgValida.validar(err), "Configuracion tipica (32KB, bloque 64B, 4 vias) es valida");
}

int main() {
    std::cout << "===============================================================================\n";
    std::cout << "              EJECUTANDO SUITE DE PRUEBAS UNITARIAS (v2)                       \n";
    std::cout << "===============================================================================\n";

    testPoliticasBasicas();
    testAnomaliaBeladyFIFO();
    testPreservacionFrecuenciaLFU();
    testClasificacion3CHill();
    testWriteBackLineasSucias();
    testValidacionesConfiguracion();

    std::cout << "\n===============================================================================\n";
    if (pruebasFallidas == 0) {
        std::cout << " >>> EXITO: TODAS LAS PRUEBAS PASARON EXITOSAMENTE (0 fallos) <<<\n";
    } else {
        std::cerr << " >>> ATENCION: " << pruebasFallidas << " PRUEBAS FALLARON <<<\n";
    }
    std::cout << "===============================================================================\n\n";

    return pruebasFallidas ? EXIT_FAILURE : EXIT_SUCCESS;
}
