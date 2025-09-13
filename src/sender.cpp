// sender.cpp
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "common.hpp"

using namespace std;
using namespace cv;

// Helper: read image with OpenCV and convert to PGM struct
PGM read_image_to_pgm(const string& path) {
    Mat img = imread(path, IMREAD_GRAYSCALE);
    if (img.empty()) {
        throw runtime_error("Erro ao abrir imagem: " + path);
    }
    PGM out;
    out.w = img.cols;
    out.h = img.rows;
    out.maxv = 255;
    out.data.assign(img.data, img.data + (img.cols * img.rows));
    return out;
}

// Helper: write pgm (not strictly needed in sender)
void write_pgm(const string& path, const PGM& pgm) {
    Mat m(pgm.h, pgm.w, CV_8UC1, const_cast<uint8_t*>(pgm.data.data()));
    imwrite(path, m);
}

int main(int argc, char** argv) {
    // usage: img_sender <fifo_path> <input_image>
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <fifo_path> <input_image>\n";
        return 1;
    }
    string fifo = argv[1];
    string inpath = argv[2];

    // 1) Garante existência do FIFO (mkfifo se necessário)
    struct stat st;
    if (stat(fifo.c_str(), &st) != 0) {
        if (mkfifo(fifo.c_str(), 0666) != 0) {
            perror("mkfifo");
            return 1;
        }
        cout << "FIFO criado em: " << fifo << "\n";
    }

    // 2) Lê imagem via OpenCV e converte para PGM
    PGM img;
    try {
        img = read_image_to_pgm(inpath);
    } catch (const exception& e) {
        cerr << e.what() << "\n";
        return 1;
    }

    // 3) Prepara o cabeçalho simples
    Header hdr{img.w, img.h, img.maxv};

    // 4) Abre FIFO para escrita (bloqueia até worker abrir leitura)
    int fd = open(fifo.c_str(), O_WRONLY);
    if (fd < 0) {
        perror("open fifo for write");
        return 1;
    }

    // 5) Envia cabeçalho (binário) + pixels
    ssize_t written;
    written = write(fd, &hdr, sizeof(hdr));
    if (written != (ssize_t)sizeof(hdr)) {
        perror("write header");
        close(fd);
        return 1;
    }
    // Escreve bytes de pixels
    size_t payload = img.data.size();
    const uint8_t* buf = img.data.data();
    size_t total = 0;
    while (total < payload) {
        ssize_t w = write(fd, buf + total, payload - total);
        if (w <= 0) { perror("write data"); close(fd); return 1; }
        total += w;
    }

    // 6) Fecha FIFO e fim
    close(fd);
    cout << "Envio concluído. Bytes enviados: " << total << "\n";
    return 0;
}
