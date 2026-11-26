# Tarea 3 - Sistemas Operativos

## Descripción

Este proyecto consiste en un simulador de administración de almacenamiento secundario. Permite representar cómo se guardan, eliminan y consultan archivos utilizando tres mecanismos de asignación: contigua, enlazada e indexada.

El programa funciona mediante consola y no crea archivos reales en el computador.

## Requisitos

- Ubuntu o Linux.
- Compilador G++.
- Compatibilidad con C++17.

## Compilar el programa

Abrir una terminal en la carpeta principal del proyecto y ejecutar:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc src/main.cpp src/disco.cpp src/simulacion.cpp -pthread -o simulador

```

## Ejecutar el programa

Después de compilar, ejecutar:

```bash
./simulador
```

## Uso del programa

Al iniciar, el programa solicitará:

1. Tamaño total del disco en MB.
2. Tamaño de cada bloque en KB.
3. Mecanismo de asignación:
   - 1: Contigua.
   - 2: Enlazada.
   - 3: Indexada.
4. Tamaño mínimo de los archivos en KB.
5. Tamaño máximo de los archivos en KB.

Los valores deben ingresarse como números enteros, sin escribir las unidades.

## Funcionamiento

- Se generan archivos de tamaños aleatorios cada 2 segundos.
- A partir de los 30 segundos, se elimina un archivo aleatorio cada 5 segundos.
- También se realizan consultas a posiciones lógicas de archivos cada 5 segundos.
- Se registra la ubicación física de los bloques y los accesos necesarios para encontrarlos.
- Se calcula la fragmentación externa del disco.
- La simulación termina cuando no hay espacio suficiente para crear un archivo, cuando la fragmentación externa impide una asignación contigua o cuando se supera la capacidad del bloque índice.
## Mecanismos de asignación

**Asignación contigua:** los bloques de cada archivo se almacenan consecutivamente.

**Asignación enlazada:** los bloques pueden estar separados y se conectan mediante punteros que indican la ubicación del siguiente bloque.

**Asignación indexada:** cada archivo utiliza un bloque índice que contiene las direcciones de sus bloques de datos.

## Consideraciones

- Se utiliza un tamaño de puntero simulado de 4 bytes.
- La asignación indexada utiliza un bloque índice por archivo.
- Los bloques y las posiciones lógicas comienzan desde cero.
- Se considera el espacio ocupado por los punteros y los bloques índice.
- La conversión utilizada es 1 MB = 1024 KB y 1 KB = 1024 bytes.

## Archivos del proyecto

- `src/main.cpp`: solicita los datos iniciales y comienza el programa.
- `src/disco.h` y `src/disco.cpp`: contienen la administración del disco y los mecanismos de asignación.
- `src/simulacion.h` y `src/simulacion.cpp`: controlan los tiempos, eventos y resultados de la simulación.
- `tests/pruebas.cpp`: contiene las pruebas unitarias de los mecanismos de asignación y la administración del disco.

- `tests/verificar_consola.py`: verifica los tiempos, eventos y casos de entrada del simulador.
## Decisiones de diseño

- Se implementó una clase `Disco` para administrar los bloques, archivos y estadísticas.
- Cada bloque almacena información sobre su propietario y, en el caso de la asignación enlazada, un puntero al siguiente bloque.
- Se utiliza un tamaño de puntero simulado de 4 bytes.
- En la asignación contigua, los bloques de cada archivo se almacenan consecutivamente.
- En la asignación enlazada, los bloques pueden ocupar posiciones no consecutivas y se conectan mediante punteros.
- En la asignación indexada, cada archivo utiliza un bloque adicional para almacenar las direcciones de sus bloques de datos.
- La simulación utiliza eventos periódicos para crear, eliminar y consultar archivos.
## Resultados

Al finalizar se muestran estadísticas sobre:

- Utilización del disco.
- Bloques ocupados y libres.
- Archivos creados y eliminados.
- Fragmentación externa.
- Promedio de accesos necesarios para localizar bloques.
