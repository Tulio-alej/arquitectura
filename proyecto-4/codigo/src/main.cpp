#include <iostream>
#include <exception>
#include "cli.hpp"

int main(int argc, char *argv[]) {
    try {
        cachesim::OpcionesCLI opciones;
        std::string errorParseo;

        if (!cachesim::InterfazCLI::parsearArgumentos(argc, argv, opciones, errorParseo)) {
            std::cerr << "Error en argumentos: " << errorParseo << "\n\n";
            std::cout << cachesim::InterfazCLI::obtenerGuiaUso() << "\n";
            return 1;
        }

        return cachesim::InterfazCLI::ejecutarComando(opciones);
    } catch (const std::exception &e) {
        std::cerr << "\n[EXCEPCION NO CONTROLADA]: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "\n[ERROR CRITICO]: Error desconocido en la ejecucion del simulador.\n";
        return 1;
    }
}
