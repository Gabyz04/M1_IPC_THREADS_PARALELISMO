#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <semaphore.h>
#include <pthread.h>

using namespace std;

// ===== Tipos e estruturas comuns (adaptados do enunciado) =====
struct PGM {
    int w;
    int h;
    int maxv;               // usualmente 255
    vector<uint8_t> data; // w*h bytes (tons de cinza)
};

struct Header {
    int w;
    int h;
    int maxv;
    // O sender envia apenas metadados básicos; o worker decide o modo via CLI
};

struct Task {
    int row_start; // inclusivo
    int row_end;   // exclusivo
};

// Modos
constexpr int MODE_NEG   = 0;
constexpr int MODE_SLICE = 1;

// FIFO padrão (pode ser passado via CLI)
constexpr const char* DEFAULT_FIFO = "/tmp/imgpipe";

// Fila circular (config)
constexpr int QMAX = 128;
