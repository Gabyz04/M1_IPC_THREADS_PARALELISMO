// worker.cpp
#include <chrono>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include <atomic>
#include "common.hpp"

using namespace std;
using namespace cv;

// ==== Estruturas da fila (produtor/consumidor) ====
Task queue_buf[QMAX];
int q_head = 0, q_tail = 0, q_count = 0;
mutex q_lock;
condition_variable q_not_empty;
condition_variable q_not_full;

// Dados globais para processamento
PGM g_in, g_out;
int g_mode = MODE_NEG;
int g_t1 = 0, g_t2 = 0;
int g_nthreads = 4;
atomic<int> remaining_tasks{0};
vector<thread> workers;

// Enfileira task
void enqueue_task(const Task &t)
{
    unique_lock<mutex> lk(q_lock);
    q_not_full.wait(lk, []
                    { return q_count < QMAX; });
    queue_buf[q_tail] = t;
    q_tail = (q_tail + 1) % QMAX;
    q_count++;
    q_not_empty.notify_one();
}

// Desenfileira task
Task dequeue_task()
{
    unique_lock<mutex> lk(q_lock);
    q_not_empty.wait(lk, []
                     { return q_count > 0; });
    Task t = queue_buf[q_head];
    q_head = (q_head + 1) % QMAX;
    q_count--;
    q_not_full.notify_one();
    return t;
}

// Filtros por bloco de linhas (implementação simplificada)
void apply_negative_block(int rs, int re)
{
    for (int r = rs; r < re; ++r)
    {
        int base = r * g_in.w;
        for (int c = 0; c < g_in.w; ++c)
        {
            g_out.data[base + c] = (uint8_t)(255 - g_in.data[base + c]);
        }
    }
}

void apply_slice_block(int rs, int re, int t1, int t2)
{
    for (int r = rs; r < re; ++r)
    {
        int base = r * g_in.w;
        for (int c = 0; c < g_in.w; ++c)
        {
            uint8_t z = g_in.data[base + c];
            if (z <= t1 || z >= t2)
                g_out.data[base + c] = 255;
            else
                g_out.data[base + c] = z;
        }
    }
}

// Thread worker
void worker_thread_func(int id)
{
    while (true)
    {
        Task task = dequeue_task();
        // sentinel: row_start == -1 indica fim
        if (task.row_start == -1)
        {
            // re-enfileira sentinel para outros workers
            enqueue_task(task);
            break;
        }
        if (g_mode == MODE_NEG)
        {
            apply_negative_block(task.row_start, task.row_end);
        }
        else
        {
            apply_slice_block(task.row_start, task.row_end, g_t1, g_t2);
        }
        remaining_tasks--;
    }
}

// Lê header + pixels do fifo (blocking)
bool read_from_fifo(const string &fifo)
{
    int fd = open(fifo.c_str(), O_RDONLY);
    if (fd < 0)
    {
        perror("open fifo for read");
        return false;
    }
    Header hdr;
    ssize_t r = read(fd, &hdr, sizeof(hdr));
    if (r != (ssize_t)sizeof(hdr))
    {
        close(fd);
        return false;
    }

    g_in.w = hdr.w;
    g_in.h = hdr.h;
    g_in.maxv = hdr.maxv;
    g_in.data.resize((size_t)g_in.w * g_in.h);

    size_t total = 0;
    size_t payload = g_in.data.size();
    uint8_t *buf = g_in.data.data();
    while (total < payload)
    {
        ssize_t n = read(fd, buf + total, payload - total);
        if (n <= 0)
        {
            perror("read data");
            close(fd);
            return false;
        }
        total += n;
    }
    close(fd);
    return true;
}

int main(int argc, char **argv)
{
    // usage: img_worker <fifo_path> <out_path> <negativo|slice> [t1 t2] [nthreads]
    if (argc < 4)
    {
        cerr << "Usage: " << argv[0] << " <fifo_path> <out_path> <negativo|slice> [t1 t2] [nthreads]\n";
        return 1;
    }
    string fifo = argv[1];
    string outpath = argv[2];
    string mode = argv[3];

    if (mode == "negativo")
    {
        g_mode = MODE_NEG;
        if (argc >= 5)
            g_nthreads = stoi(argv[4]);
    }
    else if (mode == "slice")
    {
        g_mode = MODE_SLICE;
        if (argc < 6)
        {
            cerr << "slice precisa de t1 t2\n";
            return 1;
        }
        g_t1 = stoi(argv[4]);
        g_t2 = stoi(argv[5]);
        if (argc >= 7)
            g_nthreads = stoi(argv[6]);
    }
    else
    {
        cerr << "Modo inválido\n";
        return 1;
    }

    // 1) Lê do FIFO (bloqueante)
    if (!read_from_fifo(fifo))
    {
        cerr << "Falha ao ler FIFO\n";
        return 1;
    }

    // Prepara g_out
    g_out.w = g_in.w;
    g_out.h = g_in.h;
    g_out.maxv = g_in.maxv;
    g_out.data.assign(g_in.data.size(), 0);

    // ================= BENCHMARK START =================
    auto start = chrono::high_resolution_clock::now();

    // 2) Cria pool de threads
    for (int i = 0; i < g_nthreads; ++i)
    {
        workers.emplace_back(worker_thread_func, i);
    }

    // 3) Cria tarefas (por exemplo, uma tarefa por bloco de N linhas)
    const int block_size = max(1, g_in.h / (g_nthreads * 4)); // heurística
    for (int r = 0; r < g_in.h; r += block_size)
    {
        Task t;
        t.row_start = r;
        t.row_end = min(g_in.h, r + block_size);
        remaining_tasks++;
        enqueue_task(t);
    }

    // 4) Envia sentinelas de término (uma por thread)
    Task sentinel;
    sentinel.row_start = -1;
    sentinel.row_end = -1;
    enqueue_task(sentinel);

    // 5) Aguarda término de threads
    for (auto &th : workers)
        if (th.joinable())
            th.join();

    // 6) Grava imagem de saída usando OpenCV
    Mat m(g_out.h, g_out.w, CV_8UC1, g_out.data.data());
    if (!imwrite(outpath, m))
    {
        cerr << "Erro ao salvar imagem\n";
        return 1;
    }
    cout << "Processamento concluído. Salvo em: " << outpath << "\n";

    // ================= BENCHMARK END =================
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Tempo de processamento: " << duration << " ms usando "
         << g_nthreads << " threads.\n";
    return 0;
}
