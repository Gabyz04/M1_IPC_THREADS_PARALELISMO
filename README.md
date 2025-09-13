# 📷 Processamento Paralelo de Imagens com IPC (FIFO + Threads)

Este projeto implementa um **sistema de processamento paralelo de imagens** com:

* **Dois processos** (Sender e Worker) que se comunicam via **FIFO (named pipe)**.
* **Threads** para aplicar filtros de imagem em paralelo.
* **Filtros implementados**: Negativo e Slice (limiarização por fatiamento).

---

## 🚀 Requisitos

* **Linux/Ubuntu** (nativo, WSL ou GitHub Codespaces).
* **C++17** + **CMake ≥ 3.10**
* **OpenCV ≥ 3.4**
* **Imagemagick** (opcional, para gerar imagens de teste)

### Instalação no Ubuntu/Codespaces

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libopencv-dev imagemagick
```

---

## ⚙️ Compilação

```bash
git clone <seu_repositorio>
cd M1_IPC_THREADS_PARALELISMO
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Isso gera os executáveis:

* `img_sender`
* `img_worker`

---

## 🖼️ Criar imagem de teste

Você pode usar uma imagem real em escala de cinza ou gerar uma artificial com gradiente:

```bash
convert -size 256x256 gradient: entrada.png
```

Isso cria `entrada.png` dentro de `build/`.

---

## ▶️ Execução

### 1. Worker (Terminal 1)

Exemplo de execução do filtro negativo com 4 threads:

```bash
cd build
./img_worker /tmp/imgpipe saida.png negativo 4
```

Exemplo do filtro slice (mantendo pixels entre 50 e 200):

```bash
cd build
./img_worker /tmp/imgpipe saida_slice.png slice 50 200 4
```

### 2. Sender (Terminal 2)

Envio da imagem para processamento:

```bash
cd build
./img_sender /tmp/imgpipe entrada.png
```

---

## 🔎 Descrição dos filtros

### Filtro Negativo

Cada pixel é transformado em:

```
out = 255 - in
```

Ou seja, pixels claros viram escuros e vice-versa.

### Filtro Slice (limiarização por fatiamento)

Mantém pixels dentro de `[T1, T2]` e transforma os outros em branco (255).
Exemplo com T1=50 e T2=200:

```bash
./img_worker /tmp/imgpipe saida_slice.png slice 50 200 4
./img_sender /tmp/imgpipe entrada.png
```

Isso gera uma imagem destacando apenas a faixa de interesse.

---

## ⚡ Benchmarks de Desempenho

Cada execução imprime o tempo gasto. Exemplo:

```
Processamento concluído. Salvo em: saida.png
Tempo de processamento: 230 ms usando 4 threads.
```

### Testes manuais

Execute mudando o número de threads (1, 2, 4, 8) e compare os tempos.

| Threads | Tempo (ms) | Speedup |
| ------- | ---------- | ------- |
| 1       | 820        | 1.0x    |
| 2       | 430        | 1.9x    |
| 4       | 230        | 3.5x    |
| 8       | 140        | 5.8x    |

🔎 **Conclusão**: o paralelismo acelera bastante até o limite de núcleos físicos. Após isso, o ganho diminui devido à sobrecarga de sincronização.

---

## 📂 Estrutura do Projeto

```
.
├── include/      # headers comuns (common.hpp)
├── src/          # sender.cpp e worker.cpp
├── build/        # gerado pelo cmake (executáveis e intermediários)
└── README.md
```

---

## 📌 Notas Importantes

* O **worker** deve ser iniciado **antes** do **sender**, pois o FIFO bloqueia até ambos se conectarem.
* Funciona direto em **Codespaces/Ubuntu/WSL**.
* Em **Windows nativo** seria necessário adaptar `mkfifo` para `CreateNamedPipe`.
* O sistema foi validado com imagens PGM/PNG em escala de cinza usando OpenCV.
* O relatório em PDF acompanha este projeto, contendo fundamentação teórica, explicação, benchmarks e análise.
