#include "disco.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

const char* nombre_mecanismo(Mecanismo mecanismo) {
    switch (mecanismo) {
        case Mecanismo::Contigua: return "Contigua";
        case Mecanismo::Enlazada: return "Enlazada";
        case Mecanismo::Indexada: return "Indexada";
    }
    return "Desconocido";
}

Disco::Disco(std::uint64_t disco_bytes, std::uint64_t bloque_bytes,
             Mecanismo mecanismo)
    : disco_bytes_(disco_bytes), bloque_bytes_(bloque_bytes),
      mecanismo_(mecanismo) {
    if (disco_bytes == 0 || bloque_bytes == 0 || bloque_bytes > disco_bytes) {
        throw std::invalid_argument("El disco debe contener al menos un bloque.");
    }
    if (mecanismo != Mecanismo::Contigua && mecanismo != Mecanismo::Enlazada &&
        mecanismo != Mecanismo::Indexada) {
        throw std::invalid_argument("Mecanismo de asignacion invalido.");
    }
    if (mecanismo == Mecanismo::Enlazada && bloque_bytes <= BYTES_PUNTERO) {
        throw std::invalid_argument("El bloque debe tener espacio para datos y puntero.");
    }
    const std::uint64_t cantidad = disco_bytes / bloque_bytes;
    if (cantidad > MAX_BLOQUES) {
        throw std::invalid_argument("Se supera el limite de bloques de la simulacion.");
    }
    bloques_.resize(static_cast<std::size_t>(cantidad));
}

ResultadoCreacion Disco::crear(std::uint64_t tamano_bytes) {
    if (tamano_bytes == 0) {
        throw std::invalid_argument("El archivo debe tener un tamano positivo.");
    }

    ResultadoCreacion resultado;
    const std::uint64_t datos_por_bloque =
        mecanismo_ == Mecanismo::Enlazada ? bloque_bytes_ - BYTES_PUNTERO
                                         : bloque_bytes_;
    
    resultado.bloques_datos_necesarios = tamano_bytes / datos_por_bloque;
    if (tamano_bytes % datos_por_bloque != 0) {
        ++resultado.bloques_datos_necesarios;
    }
    resultado.bloques_totales_necesarios = resultado.bloques_datos_necesarios;

    if (mecanismo_ == Mecanismo::Indexada) {
        const std::uint64_t capacidad_indice = bloque_bytes_ / BYTES_PUNTERO;
        if (resultado.bloques_datos_necesarios > capacidad_indice) {
            resultado.motivo = MotivoFallo::CapacidadIndice;
            return resultado;
        }
        ++resultado.bloques_totales_necesarios; // Tambien se ocupa el indice.
    }

    if (resultado.bloques_totales_necesarios > estadisticas().libres) {
        resultado.motivo = MotivoFallo::EspacioInsuficiente;
        return resultado;
    }

    
    Archivo nuevo;
    nuevo.tamano_bytes = tamano_bytes;
    const auto cantidad = static_cast<std::size_t>(resultado.bloques_datos_necesarios);

    if (mecanismo_ == Mecanismo::Contigua) {
        std::size_t consecutivos = 0;
        std::size_t inicio = 0;
        for (std::size_t i = 0; i < bloques_.size(); ++i) {
            if (bloques_[i].propietario == 0) {
                if (consecutivos == 0) {
                    inicio = i;
                }
                ++consecutivos;
                if (consecutivos == cantidad) {
                    break;
                }
            } else {
                consecutivos = 0;
            }
        }
        if (consecutivos < cantidad) {
            resultado.motivo = MotivoFallo::FragmentacionExterna;
            return resultado;
        }
        nuevo.bloques_datos.reserve(cantidad);
        for (std::size_t i = 0; i < cantidad; ++i) {
            nuevo.bloques_datos.push_back(static_cast<std::int32_t>(inicio + i));
        }
    } else {
        nuevo.bloques_datos.reserve(cantidad);
        for (std::size_t i = 0; i < bloques_.size(); ++i) {
            if (bloques_[i].propietario != 0) {
                continue;
            }
            if (mecanismo_ == Mecanismo::Indexada && nuevo.bloque_indice == -1) {
                nuevo.bloque_indice = static_cast<std::int32_t>(i);
            } else {
                nuevo.bloques_datos.push_back(static_cast<std::int32_t>(i));
            }
            if (nuevo.bloques_datos.size() == cantidad) {
                break;
            }
        }
    }

    if (creados_ == std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error("Se agotaron los identificadores de archivos.");
    }
    nuevo.id = creados_ + 1;
    nuevo.inicio = nuevo.bloques_datos.front();
    
    archivos_.push_back(std::move(nuevo));
    const Archivo& archivo = archivos_.back();
    for (std::size_t i = 0; i < archivo.bloques_datos.size(); ++i) {
        Bloque& bloque = bloques_[static_cast<std::size_t>(archivo.bloques_datos[i])];
        bloque.propietario = archivo.id;
        bloque.es_indice = false;
        bloque.siguiente = -1;
        if (mecanismo_ == Mecanismo::Enlazada && i + 1 < archivo.bloques_datos.size()) {
            bloque.siguiente = archivo.bloques_datos[i + 1];
        }
    }
    if (archivo.bloque_indice != -1) {
        Bloque& indice = bloques_[static_cast<std::size_t>(archivo.bloque_indice)];
        indice.propietario = archivo.id;
        indice.es_indice = true;
        indice.siguiente = -1;
    }
    ++creados_;
    resultado.exito = true;
    resultado.id = archivo.id;
    return resultado;
}

std::optional<Archivo> Disco::eliminar(std::size_t posicion_archivo) {
    if (posicion_archivo >= archivos_.size()) {
        return std::nullopt;
    }
    Archivo eliminado = std::move(archivos_[posicion_archivo]);
    for (const std::int32_t direccion : eliminado.bloques_datos) {
        bloques_[static_cast<std::size_t>(direccion)] = Bloque{};
    }
    if (eliminado.bloque_indice != -1) {
        bloques_[static_cast<std::size_t>(eliminado.bloque_indice)] = Bloque{};
    }
    archivos_.erase(archivos_.begin() + static_cast<std::ptrdiff_t>(posicion_archivo));
    ++eliminados_;
    return eliminado;
}

std::optional<ResultadoAcceso> Disco::acceder(std::size_t posicion_archivo,
                                            std::uint64_t posicion_bytes) {
    if (posicion_archivo >= archivos_.size()) {
        return std::nullopt;
    }
    const Archivo& archivo = archivos_[posicion_archivo];
    if (posicion_bytes >= archivo.tamano_bytes) {
        return std::nullopt;
    }
    const std::uint64_t datos_por_bloque =
        mecanismo_ == Mecanismo::Enlazada ? bloque_bytes_ - BYTES_PUNTERO
                                         : bloque_bytes_;

    ResultadoAcceso resultado;
    resultado.archivo_id = archivo.id;
    resultado.posicion_bytes = posicion_bytes;
    resultado.bloque_logico = posicion_bytes / datos_por_bloque;
    resultado.desplazamiento = posicion_bytes % datos_por_bloque;

    if (mecanismo_ == Mecanismo::Contigua) {
        resultado.bloque_fisico = archivo.inicio +
            static_cast<std::int32_t>(resultado.bloque_logico);
        resultado.accesos = 1;
    } else if (mecanismo_ == Mecanismo::Enlazada) {
        
        resultado.bloque_fisico = archivo.inicio;
        for (std::uint64_t i = 0; i < resultado.bloque_logico; ++i) {
            if (resultado.bloque_fisico < 0 ||
                static_cast<std::size_t>(resultado.bloque_fisico) >= bloques_.size()) {
                throw std::logic_error("Cadena de bloques invalida.");
            }
            resultado.bloque_fisico =
                bloques_[static_cast<std::size_t>(resultado.bloque_fisico)].siguiente;
        }
        resultado.accesos = resultado.bloque_logico + 1;
    } else {
        resultado.bloque_fisico =
            archivo.bloques_datos[static_cast<std::size_t>(resultado.bloque_logico)];
        resultado.accesos = 2; 
    }

    if (resultado.bloque_fisico < 0 ||
        static_cast<std::size_t>(resultado.bloque_fisico) >= bloques_.size() ||
        bloques_[static_cast<std::size_t>(resultado.bloque_fisico)].propietario != archivo.id) {
        throw std::logic_error("Direccion fisica invalida para el archivo.");
    }
    if (consultas_ == std::numeric_limits<std::uint64_t>::max() ||
        total_accesos_ > std::numeric_limits<std::uint64_t>::max() - resultado.accesos) {
        throw std::overflow_error("Se agotaron los contadores de accesos.");
    }
    ++consultas_;
    total_accesos_ += resultado.accesos;
    return resultado;
}

Estadisticas Disco::estadisticas() const {
    Estadisticas datos;
    datos.bloques_total = bloques_.size();
    std::uint64_t consecutivos = 0;
    for (const Bloque& bloque : bloques_) {
        if (bloque.propietario == 0) {
            ++datos.libres;
            ++consecutivos;
            datos.mayor_hueco = std::max(datos.mayor_hueco, consecutivos);
        } else {
            consecutivos = 0;
        }
    }
    datos.ocupados = datos.bloques_total - datos.libres;
    datos.fragmentacion_bloques = datos.libres - datos.mayor_hueco;
    if (datos.libres != 0) {
        datos.fragmentacion_porcentaje =
            100.0 * static_cast<double>(datos.fragmentacion_bloques) /
            static_cast<double>(datos.libres);
    }
    datos.utilizacion_porcentaje =
        100.0 * static_cast<double>(datos.ocupados) * static_cast<double>(bloque_bytes_) /
        static_cast<double>(disco_bytes_);
    datos.creados = creados_;
    datos.eliminados = eliminados_;
    datos.activos = archivos_.size();
    datos.consultas = consultas_;
    datos.total_accesos = total_accesos_;
    return datos;
}
