# 📷 Processamento Paralelo de Imagens com IPC (FIFO + Threads)

Este projeto implementa um **pipeline de processamento de imagens em paralelo** usando **threads** e comunicação entre processos via **FIFO (Named Pipe)**.  
Ele é dividido em dois executáveis:

- **img_sender** → lê a imagem de entrada e envia pelo FIFO.
- **img_worker** → recebe a imagem, aplica o filtro (negativo ou slice) em paralelo, e salva o resultado.

O projeto foi projetado para rodar em **Linux/Ubuntu** (incluindo **GitHub Codespaces** e **WSL no Windows**).  
Para Windows nativo, seria necessário adaptar o código para usar **Named Pipes do Windows (`CreateNamedPipe`)** em vez de `mkfifo`.

---

## 🚀 Requisitos

- **C++17**
- **CMake ≥ 3.10**
- **OpenCV ≥ 3.4** (headers e libs)
- **Threads (pthread)** → já incluso no Linux

### Instalação no Ubuntu / Codespaces
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libopencv-dev imagemagick
