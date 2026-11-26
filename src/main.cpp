#include "simulacion.h"

#include <charconv>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>

namespace {


std::uint64_t leer_entero(const std::string& pregunta, std::uint64_t minimo,
                         std::uint64_t maximo) {
    while (true) {
        std::cout << pregunta << std::flush;
        std::string linea;
        if (!std::getline(std::cin, linea)) {
            throw std::runtime_error("Entrada finalizada antes de completar la configuracion.");
        }
        const auto inicio = linea.find_first_not_of(" \t\r");
        const auto fin = linea.find_last_not_of(" \t\r");
        if (inicio != std::string::npos) {
            const std::string texto = linea.substr(inicio, fin - inicio + 1);
            std::uint64_t valor = 0;
            const auto resultado = std::from_chars(
                texto.data(), texto.data() + texto.size(), valor);
            if (resultado.ec == std::errc{} &&
                resultado.ptr == texto.data() + texto.size() &&
                valor >= minimo && valor <= maximo) {
                return valor;
            }
        }
        std::cout << "Entrada invalida. Ingrese un entero entre " << minimo
                  << " y " << maximo << ".\n";
    }
}

Configuracion leer_configuracion() {
    const auto maximo = std::numeric_limits<std::uint64_t>::max();
    std::cout << "SIMULADOR DE ASIGNACION DE ARCHIVOS - TAREA 3\n"
              << "Convencion: 1 MB = 1024 KB; 1 KB = 1024 bytes.\n"
              << "Ingrese tamanos enteros positivos.\n\n";

    const auto disco_mb = leer_entero("Tamano total del disco (MB): ",
                                      1, maximo / BYTES_POR_MB);
    const auto disco_bytes = disco_mb * BYTES_POR_MB;
    std::uint64_t bloque_bytes = 0;
    while (true) {
        const auto bloque_kb = leer_entero("Tamano de cada bloque (KB): ",
                                           1, maximo / BYTES_POR_KB);
        bloque_bytes = bloque_kb * BYTES_POR_KB;
        if (bloque_bytes > disco_bytes) {
            std::cout << "El bloque no puede ser mayor que el disco.\n";
        } else if (disco_bytes / bloque_bytes > MAX_BLOQUES) {
            std::cout << "Se permiten hasta " << MAX_BLOQUES
                      << " bloques para limitar el uso de memoria. "
                      << "Ingrese un bloque mas grande.\n";
        } else {
            break;
        }
    }

    std::cout << "\nMecanismo: 1 = Contigua, 2 = Enlazada, 3 = Indexada.\n";
    const auto opcion = leer_entero("Seleccione el mecanismo (1-3): ", 1, 3);
    const auto minimo = leer_entero("Tamano minimo de los archivos (KB): ",
                                    1, maximo / BYTES_POR_KB);
    const auto max_archivo = leer_entero("Tamano maximo de los archivos (KB): ",
                                        minimo, maximo / BYTES_POR_KB);
    std::cout << '\n';
    return {disco_bytes, bloque_bytes, static_cast<Mecanismo>(opcion),
            minimo, max_archivo};
}

} 

int main() {
    try {
        const auto configuracion = leer_configuracion();
        ejecutar_simulacion(configuracion);
        return 0;
    } catch (const std::bad_alloc&) {
        std::cerr << "Error: no hay memoria suficiente para mantener la simulacion.\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}
