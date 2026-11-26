#ifndef DISCO_H
#define DISCO_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

constexpr std::uint64_t BYTES_POR_KB = 1024;
constexpr std::uint64_t BYTES_POR_MB = 1024 * 1024;
constexpr std::uint64_t BYTES_PUNTERO = 4;

constexpr std::uint64_t MAX_BLOQUES = 1000000;

enum class Mecanismo { Contigua = 1, Enlazada = 2, Indexada = 3 };
enum class MotivoFallo {
    Ninguno, EspacioInsuficiente, FragmentacionExterna, CapacidadIndice
};

struct Bloque {
    std::uint64_t propietario = 0; 
    std::int32_t siguiente = -1;   
    bool es_indice = false;
};

struct Archivo {
    std::uint64_t id = 0;
    std::uint64_t tamano_bytes = 0;
    std::int32_t inicio = -1;
    std::int32_t bloque_indice = -1;
    
    std::vector<std::int32_t> bloques_datos;
};

struct ResultadoCreacion {
    bool exito = false;
    MotivoFallo motivo = MotivoFallo::Ninguno;
    std::uint64_t id = 0;
    std::uint64_t bloques_datos_necesarios = 0;
    std::uint64_t bloques_totales_necesarios = 0;
};

struct ResultadoAcceso {
    std::uint64_t archivo_id = 0;
    std::uint64_t posicion_bytes = 0;
    std::uint64_t bloque_logico = 0;
    std::int32_t bloque_fisico = -1;
    std::uint64_t desplazamiento = 0;
    std::uint64_t accesos = 0;
};

struct Estadisticas {
    std::uint64_t bloques_total = 0;
    std::uint64_t ocupados = 0;
    std::uint64_t libres = 0;
    std::uint64_t mayor_hueco = 0;
    std::uint64_t fragmentacion_bloques = 0;
    double fragmentacion_porcentaje = 0;
    double utilizacion_porcentaje = 0; 
    std::uint64_t creados = 0;
    std::uint64_t eliminados = 0;
    std::uint64_t activos = 0;
    std::uint64_t consultas = 0;
    std::uint64_t total_accesos = 0;
};

const char* nombre_mecanismo(Mecanismo mecanismo);

class Disco {
public:
    Disco(std::uint64_t disco_bytes, std::uint64_t bloque_bytes,
          Mecanismo mecanismo);
    ResultadoCreacion crear(std::uint64_t tamano_bytes);
    std::optional<Archivo> eliminar(std::size_t posicion_archivo);
    std::optional<ResultadoAcceso> acceder(std::size_t posicion_archivo,
                                           std::uint64_t posicion_bytes);
    Estadisticas estadisticas() const;
    const std::vector<Bloque>& bloques() const { return bloques_; }
    const std::vector<Archivo>& archivos() const { return archivos_; }
    std::uint64_t tamano_disco_bytes() const { return disco_bytes_; }
    std::uint64_t tamano_bloque_bytes() const { return bloque_bytes_; }
    Mecanismo mecanismo() const { return mecanismo_; }

private:
    std::uint64_t disco_bytes_;
    std::uint64_t bloque_bytes_;
    Mecanismo mecanismo_;
    std::vector<Bloque> bloques_;
    std::vector<Archivo> archivos_;
    std::uint64_t creados_ = 0;
    std::uint64_t eliminados_ = 0;
    std::uint64_t consultas_ = 0;
    std::uint64_t total_accesos_ = 0;
};

#endif
