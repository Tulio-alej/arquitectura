#include "modo_didactico.hpp"

#include <iostream>
#include <iomanip>
#include <bitset>

namespace cachesim {

ModoDidactico::ModoDidactico(const ConfiguracionCache &cfg, const std::string &politica,
                             bool interactivo, std::size_t limitePasos)
    : configuracion(cfg), nombrePolitica(politica),
      modoInteractivo(interactivo), limitePasos(limitePasos) {}

void ModoDidactico::mostrarEncabezadoConceptual() const {
    std::cout << "\n===============================================================================\n";
    std::cout << "                  MODO DIDACTICO: SIMULACION DE MEMORIA CACHE                  \n";
    std::cout << "===============================================================================\n";
    std::cout << "  Jerarquia de Memoria y el Dilema del Hardware:\n";
    std::cout << "   +---------+        +------------------------+        +-------------------+\n";
    std::cout << "   |   CPU   | <----> |  CACHE SRAM (Rapida)   | <----> | DRAM (Memoria     |\n";
    std::cout << "   |         |        |  Latencia: " << std::fixed << std::setprecision(1) << this->configuracion.getHitLatency() << " ciclo     |        |       Principal)  |\n";
    std::cout << "   | (Core)  |        |  Capacidad: " << this->configuracion.describir() << " |        | Latencia: " << this->configuracion.getMissPenalty() << "c   |\n";
    std::cout << "   +---------+        +------------------------+        +-------------------+\n";
    std::cout << "                                  ^ \n";
    std::cout << "                      MMU (Unidad de Manejo de Memoria)\n";
    std::cout << "              Decide que bloques conservar y cuales desalojar\n";
    std::cout << "                     mediante la Politica: " << this->nombrePolitica << "\n";
    std::cout << "-------------------------------------------------------------------------------\n";
    std::cout << "  Desglose de Bits de la Direccion en Hardware:\n";
    std::cout << "   [ ETIQUETA / TAG (bits 63.." 
              << (this->configuracion.getBitsOffset() + this->configuracion.getBitsIndice()) << ") ]"
              << " [ INDICE CONJUNTO (bits " 
              << (this->configuracion.getBitsOffset() + this->configuracion.getBitsIndice() - 1) << ".." 
              << this->configuracion.getBitsOffset() << ") ]"
              << " [ OFFSET BLOQUE (bits " 
              << (this->configuracion.getBitsOffset() - 1) << "..0) ]\n";
    std::cout << "     Bits Offset : " << this->configuracion.getBitsOffset() << " bits (determina byte dentro de linea de " << this->configuracion.getBlockSize() << "B)\n";
    std::cout << "     Bits Indice : " << this->configuracion.getBitsIndice() << " bits (selecciona 1 de los " << this->configuracion.getNumSets() << " conjuntos)\n";
    std::cout << "     Bits Tag    : " << this->configuracion.getBitsTag() << " bits (identificador unico del bloque en memoria)\n";
    std::cout << "===============================================================================\n\n";
}

void ModoDidactico::mostrarDescomposicionHardware(DireccionMemoria dir) const {
    EtiquetaBloque tag = 0;
    IndiceConjunto indice = 0;
    DireccionMemoria offset = 0;

    const unsigned bitsOffset = this->configuracion.getBitsOffset();
    const unsigned bitsIndice = this->configuracion.getBitsIndice();

    const DireccionMemoria mascaraOffset = (DireccionMemoria(1) << bitsOffset) - 1;
    offset = dir & mascaraOffset;

    if (bitsIndice == 0) {
        indice = 0;
    } else {
        indice = (dir >> bitsOffset) & (this->configuracion.getNumSets() - 1);
    }
    tag = dir >> (bitsOffset + bitsIndice);

    std::cout << "  [Descomposicion de Direccion 0x" << std::hex << dir << std::dec << "]:\n";
    std::cout << "    * Tag (Etiqueta)     : 0x" << std::hex << tag << std::dec << "\n";
    std::cout << "    * Indice de Conjunto : " << indice << " (Set " << indice << ")\n";
    std::cout << "    * Desplazamiento     : " << offset << " bytes (Offset dentro del bloque)\n";
    std::cout << "    * Numero de Bloque   : 0x" << std::hex << (dir >> bitsOffset) << std::dec << "\n";
}

void ModoDidactico::mostrarEstadoConjunto(const Cache &cache, IndiceConjunto idxConjunto) const {
    const ConjuntoCache &conjunto = cache.getConjunto(idxConjunto);
    std::cout << "    * Estado del Set " << idxConjunto << " (Total vias: " << conjunto.lineas.size() << "):\n";

    for (std::size_t v = 0; v < conjunto.lineas.size(); ++v) {
        const LineaCache &linea = conjunto.lineas[v];
        std::cout << "       [Via " << v << "] ";
        if (linea.valida) {
            std::cout << "VALIDA | Bloque: 0x" << std::hex << linea.bloque 
                      << " | Tag: 0x" << linea.tag << std::dec
                      << " | " << (linea.sucia ? "SUCIA (Dirty)" : "LIMPIA (Clean)");
        } else {
            std::cout << "LIBRE (Vacia)";
        }
        std::cout << "\n";
    }

    if (conjunto.politica) {
        std::cout << "       --> " << conjunto.politica->describirEstado() << "\n";
    }
}

void ModoDidactico::ejecutarPasoAPaso(const std::vector<AccesoMemoria> &traza,
                                     const EntornoPolitica &entorno) {
    this->mostrarEncabezadoConceptual();

    Cache cache(this->configuracion, this->nombrePolitica, entorno);
    const std::size_t totalPasos = std::min(this->limitePasos, traza.size());

    std::cout << ">>> Iniciando ejecucion didactica (" << totalPasos 
              << " de " << traza.size() << " accesos mostrados en detalle) <<<\n\n";

    for (std::size_t i = 0; i < totalPasos; ++i) {
        const AccesoMemoria &acc = traza[i];
        const IndiceConjunto setIdx = cache.calcularConjunto(acc.direccion);

        std::cout << "-------------------------------------------------------------------------------\n";
        std::cout << "PASO " << (i + 1) << " / " << totalPasos << " | Operacion: [" 
                  << acc.getSimboloOperacion() << " - " 
                  << (acc.esEscritura() ? "ESCRITURA" : "LECTURA") << "] "
                  << "a la Direccion: " << acc.getDireccionHex() << "\n";
        std::cout << "-------------------------------------------------------------------------------\n";

        this->mostrarDescomposicionHardware(acc.direccion);
        std::cout << "\n  [Estado PREVIO al acceso]:\n";
        this->mostrarEstadoConjunto(cache, setIdx);

        // Ejecutar acceso
        ResultadoAcceso res = cache.acceder(acc, i, true);

        std::cout << "\n  [RESULTADO DEL ACCESO]:\n";
        if (res.acierto) {
            std::cout << "  >>> [!] CACHE HIT (Acierto en cache) <!<<\n";
            std::cout << "      Latencia: " << this->configuracion.getHitLatency() << " ciclo (Acceso directo a SRAM rapida).\n";
            std::cout << "      Explicacion: " << res.motivoDecision << "\n";
        } else {
            std::cout << "  >>> [X] CACHE MISS (Fallo en cache) <!<<\n";
            std::cout << "      Latencia: " << (this->configuracion.getHitLatency() + this->configuracion.getMissPenalty())
                      << " ciclos (Penalizacion DRAM: +" << this->configuracion.getMissPenalty() << " ciclos de Stall).\n";
            std::cout << "      Clasificacion 3C: " << getDescripcionDidactica3C(res.clasificacion3C) << "\n";
            std::cout << "      Decision de Ubicacion: " << res.motivoDecision << "\n";
            if (res.desalojo) {
                std::cout << "      -> Bloque victima expulsado: 0x" << std::hex << res.bloqueDesalojado << std::dec << "\n";
                if (res.bloqueDesalojadoSucio) {
                    std::cout << "      -> AVISO: El bloque estaba modificado en cache. Se emitio Write-Back a DRAM.\n";
                }
            }
        }

        std::cout << "\n  [Estado POSTERIOR al acceso]:\n";
        this->mostrarEstadoConjunto(cache, setIdx);
        std::cout << "\n";

        if (this->modoInteractivo && i + 1 < totalPasos) {
            std::cout << "Presione [ENTER] para avanzar al siguiente acceso (o 'q' + [ENTER] para salir)... ";
            std::string entrada;
            std::getline(std::cin, entrada);
            if (!entrada.empty() && (entrada[0] == 'q' || entrada[0] == 'Q')) {
                std::cout << "Modo interactivo detenido por el usuario.\n\n";
                break;
            }
        }
    }

    this->mostrarConclusionPedagogica(cache.getEstadisticas(), "Traza Didactica");
}

void ModoDidactico::mostrarConclusionPedagogica(const EstadisticasCache &stats,
                                               const std::string &nombreCarga) const {
    std::cout << "===============================================================================\n";
    std::cout << "                      RESUMEN PEDAGOGICO Y METRICAS FINALES                    \n";
    std::cout << "===============================================================================\n";
    std::cout << "  Carga Analizada : " << nombreCarga << "\n";
    std::cout << "  Politica Activa : " << this->nombrePolitica << "\n";
    std::cout << "  Accesos Totales : " << stats.accesosTotales << " (" 
              << stats.lecturas << " lecturas, " << stats.escrituras << " escrituras)\n";
    std::cout << "  Aciertos (Hits) : " << stats.aciertos << "  -->  Hit Ratio: " 
              << std::fixed << std::setprecision(2) << (stats.getHitRatio() * 100.0) << "%\n";
    std::cout << "  Fallos (Misses) : " << stats.fallos << "  -->  Miss Ratio: " 
              << (stats.getMissRatio() * 100.0) << "%\n\n";

    std::cout << "  Desglose Cientifico de Fallos (Modelo 3C de Mark D. Hill):\n";
    std::cout << "    1. Compulsorios (Cold) : " << stats.fallosCold << " ("
              << (stats.fallos ? (double(stats.fallosCold) * 100.0 / stats.fallos) : 0.0) << "% de los fallos)\n";
    std::cout << "       -> Inevitables: primera carga del bloque en el espacio de direcciones.\n";
    std::cout << "    2. Capacidad (Capacity): " << stats.fallosCapacity << " ("
              << (stats.fallos ? (double(stats.fallosCapacity) * 100.0 / stats.fallos) : 0.0) << "% de los fallos)\n";
    std::cout << "       -> El conjunto de trabajo activo supero la capacidad global de la cache ("
              << (this->configuracion.getSizeBytes() / 1024) << " KB).\n";
    std::cout << "    3. Conflicto (Conflict): " << stats.fallosConflict << " ("
              << (stats.fallos ? (double(stats.fallosConflict) * 100.0 / stats.fallos) : 0.0) << "% de los fallos)\n";
    std::cout << "       -> Multiples bloques compitieron por las mismas vias de un conjunto especifico.\n\n";

    std::cout << "  Impacto en el Rendimiento del Procesador:\n";
    std::cout << "    * AMAT (Average Memory Access Time):\n";
    std::cout << "      AMAT = Hit_Time + (Miss_Rate * Miss_Penalty)\n";
    std::cout << "      AMAT = " << this->configuracion.getHitLatency() << " + (" 
              << stats.getMissRatio() << " * " << this->configuracion.getMissPenalty() << ") = "
              << stats.getAmat(this->configuracion) << " ciclos de CPU por acceso.\n";
    std::cout << "    * Ciclos Totales de CPU Stall (espera de DRAM) : " 
              << static_cast<std::uint64_t>(stats.getCiclosStallTotales(this->configuracion)) << " ciclos.\n";
    std::cout << "    * Desalojos totales de lineas : " << stats.desalojosTotales << "\n";
    std::cout << "    * Write-backs a memoria principal : " << stats.escriturasMemoria << "\n";
    std::cout << "===============================================================================\n\n";
}

} // namespace cachesim
