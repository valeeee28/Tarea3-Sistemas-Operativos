#include "simulacion.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>

namespace {

void mostrar_bloques(const std::vector<std::int32_t>& bloques) {
    std::cout << '[';
    for (std::size_t i = 0; i < bloques.size(); ++i) {
        if (i != 0) std::cout << ", ";
        std::cout << bloques[i];
    }
    std::cout << ']';
}

void mostrar_estado(const Disco& disco) {
    const auto e = disco.estadisticas();
    std::cout << "  Estado: ocupados=" << e.ocupados
              << ", libres=" << e.libres
              << ", mayor hueco=" << e.mayor_hueco
              << ", fragmentacion externa=" << e.fragmentacion_bloques
              << " bloques (" << e.fragmentacion_porcentaje
              << "% de los libres).\n";
}

void mostrar_archivo(const Disco& disco, const Archivo& archivo) {
    std::cout << "archivo_" << archivo.id << " | tamano="
              << archivo.tamano_bytes / BYTES_POR_KB << " KB"
              << " | bloques de datos=";
    mostrar_bloques(archivo.bloques_datos);
    if (disco.mecanismo() == Mecanismo::Contigua) {
        std::cout << " | inicio=" << archivo.inicio
                  << " | cantidad=" << archivo.bloques_datos.size();
    } else if (disco.mecanismo() == Mecanismo::Enlazada) {
        std::cout << " | cadena=";
        auto bloque = archivo.inicio;
        while (bloque != -1) {
            std::cout << bloque << " -> ";
            bloque = disco.bloques().at(static_cast<std::size_t>(bloque)).siguiente;
        }
        std::cout << "FIN";
    } else {
        std::cout << " | bloque indice=" << archivo.bloque_indice;
    }
    std::cout << '\n';
}

void mostrar_fallo(const Disco& disco, const ResultadoCreacion& resultado,
                   std::uint64_t tamano_kb) {
    const auto e = disco.estadisticas();
    std::cout << "No se pudo crear un archivo de " << tamano_kb << " KB.\n"
              << "Motivo de finalizacion: ";
    switch (resultado.motivo) {
    case MotivoFallo::EspacioInsuficiente:
        std::cout << "ESPACIO INSUFICIENTE. Se necesitan "
                  << resultado.bloques_totales_necesarios << " bloques y hay "
                  << e.libres << " libres.\n";
        break;
    case MotivoFallo::FragmentacionExterna:
        std::cout << "FRAGMENTACION EXTERNA. Se necesitan "
                  << resultado.bloques_datos_necesarios << " bloques consecutivos; "
                  << "hay " << e.libres << " libres, pero el mayor hueco es de "
                  << e.mayor_hueco << " bloques.\n";
        break;
    case MotivoFallo::CapacidadIndice:
        std::cout << "CAPACIDAD DEL BLOQUE INDICE EXCEDIDA. El archivo requiere "
                  << resultado.bloques_datos_necesarios
                  << " direcciones, pero el indice solo admite "
                  << disco.tamano_bloque_bytes() / BYTES_PUNTERO << ".\n";
        break;
    case MotivoFallo::Ninguno:
        throw std::logic_error("Fallo de asignacion sin motivo registrado.");
    }
}

void mostrar_estadisticas(const Disco& disco, std::int64_t segundo) {
    const auto e = disco.estadisticas();
    const auto bloque_bytes = disco.tamano_bloque_bytes();
    std::cout << "\n=== ESTADISTICAS FINALES ===\n"
              << "Duracion programada: " << segundo << " segundos\n"
              << "Mecanismo: " << nombre_mecanismo(disco.mecanismo()) << '\n'
              << "Tamano total del disco: "
              << static_cast<double>(disco.tamano_disco_bytes()) / BYTES_POR_MB
              << " MB (" << disco.tamano_disco_bytes() << " bytes)\n"
              << "Tamano del bloque: " << bloque_bytes / BYTES_POR_KB << " KB\n"
              << "Cantidad total de bloques: " << e.bloques_total << '\n'
              << "Bloques ocupados: " << e.ocupados << '\n'
              << "Bloques libres: " << e.libres << '\n'
              << "Bytes residuales no asignables: "
              << disco.tamano_disco_bytes() % bloque_bytes << '\n'
              << "Utilizacion del disco total: " << e.utilizacion_porcentaje << "%\n"
              << "Utilizacion de bloques utilizables: "
              << 100.0 * static_cast<double>(e.ocupados) /
                     static_cast<double>(e.bloques_total) << "%\n"
              << "Archivos creados: " << e.creados << '\n'
              << "Archivos eliminados: " << e.eliminados << '\n'
              << "Archivos almacenados: " << e.activos << '\n'
              << "Mayor secuencia libre: " << e.mayor_hueco << " bloques\n"
              << "Fragmentacion externa: " << e.fragmentacion_bloques
              << " bloques (" << e.fragmentacion_porcentaje << "% de los libres)\n"
              << "Consultas logicas realizadas: " << e.consultas << '\n'
              << "Accesos a bloques acumulados: " << e.total_accesos << '\n';
    if (e.consultas == 0) {
        std::cout << "Promedio de accesos: no se realizaron consultas logicas.\n";
    } else {
        std::cout << "Promedio de accesos: "
                  << static_cast<double>(e.total_accesos) /
                         static_cast<double>(e.consultas) << '\n';
    }
    std::cout << "La ocupacion incluye punteros y bloques indice.\n";
    if (disco.mecanismo() != Mecanismo::Contigua) {
        std::cout << "La dispersion del espacio libre no impide asignar "
                  << "bloques no consecutivos en este mecanismo.\n";
    }
}

} 

void ejecutar_simulacion(const Configuracion& configuracion) {
    Disco disco(configuracion.disco_bytes, configuracion.bloque_bytes,
                configuracion.mecanismo);
    std::mt19937_64 generador(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> tamano_aleatorio(
        configuracion.minimo_kb, configuracion.maximo_kb);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Mecanismo: " << nombre_mecanismo(configuracion.mecanismo) << '\n'
              << "Bloques fisicos y posiciones logicas numerados desde cero.\n"
              << "Puntero simulado: 4 bytes. Un bloque indice por archivo indexado.\n"
              << "Creaciones: t=2,4,6,...; eliminaciones y accesos: t=30,35,40,...\n"
              << "Orden si coinciden: eliminar, acceder, crear.\n";
    mostrar_estado(disco);

    
    using Reloj = std::chrono::steady_clock;
    const auto inicio = Reloj::now();
    std::int64_t proxima_creacion = 2;
    std::int64_t proximo_periodico = 30;
    std::cout << "[t=0 s] INICIO de la simulacion.\n" << std::flush;

    while (true) {
        const auto segundo = std::min(proxima_creacion, proximo_periodico);
        std::this_thread::sleep_until(inicio + std::chrono::seconds(segundo));

        if (segundo == proximo_periodico) {
            std::cout << "[t=" << segundo << " s] ELIMINAR ";
            if (disco.archivos().empty()) {
                std::cout << "No hay archivos disponibles.\n";
            } else {
                std::uniform_int_distribution<std::size_t> elegir(
                    0, disco.archivos().size() - 1);
                const auto eliminado = disco.eliminar(elegir(generador));
                if (!eliminado) throw std::logic_error("Archivo a eliminar inexistente.");
                std::cout << "archivo_" << eliminado->id << " | bloques liberados=";
                mostrar_bloques(eliminado->bloques_datos);
                if (eliminado->bloque_indice != -1) {
                    std::cout << " | indice liberado=" << eliminado->bloque_indice;
                }
                std::cout << '\n';
                mostrar_estado(disco);
            }

            std::cout << "[t=" << segundo << " s] ACCEDER ";
            if (disco.archivos().empty()) {
                std::cout << "No hay archivos disponibles para consultar.\n";
            } else {
                std::uniform_int_distribution<std::size_t> elegir(
                    0, disco.archivos().size() - 1);
                const auto indice = elegir(generador);
                std::uniform_int_distribution<std::uint64_t> elegir_posicion(
                    0, disco.archivos()[indice].tamano_bytes - 1);
                const auto acceso = disco.acceder(indice, elegir_posicion(generador));
                if (!acceso) throw std::logic_error("Posicion logica inexistente.");
                std::cout << "archivo_" << acceso->archivo_id
                          << " | posicion logica=" << acceso->posicion_bytes << " bytes"
                          << " | bloque logico=" << acceso->bloque_logico
                          << " | bloque fisico=" << acceso->bloque_fisico
                          << " | desplazamiento=" << acceso->desplazamiento << " bytes"
                          << " | accesos=" << acceso->accesos << '\n';
            }
            proximo_periodico += 5;
        }

        if (segundo == proxima_creacion) {
            const auto tamano_kb = tamano_aleatorio(generador);
            const auto resultado = disco.crear(tamano_kb * BYTES_POR_KB);
            std::cout << "[t=" << segundo << " s] CREAR ";
            if (!resultado.exito) {
                mostrar_fallo(disco, resultado, tamano_kb);
                mostrar_estadisticas(disco, segundo);
                std::cout << std::flush;
                return;
            }
            mostrar_archivo(disco, disco.archivos().back());
            mostrar_estado(disco);
            proxima_creacion += 2;
        }
        std::cout << std::flush;
    }
}
