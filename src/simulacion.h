#ifndef SIMULACION_H
#define SIMULACION_H

#include "disco.h"

struct Configuracion {
    std::uint64_t disco_bytes;
    std::uint64_t bloque_bytes;
    Mecanismo mecanismo;
    std::uint64_t minimo_kb;
    std::uint64_t maximo_kb;
};

void ejecutar_simulacion(const Configuracion& configuracion);

#endif
