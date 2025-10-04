# Processamento Paralelo de Dados do Spotify com MPI

Este projeto implementa duas aplicações MPI em C para processar em paralelo um grande dataset de músicas do Spotify.

## Pré-requisitos

- **MS-MPI**: Microsoft MPI instalado e configurado
- **GCC**: Compilador GCC disponível
- **Dataset**: Arquivo `data/spotify_millsongdata.csv` no diretório do projeto

## Compilação

```bash
# Compilar ambos os programas
make
```

## Execução

```bash
# Executar ambos
make run-all

# Contagem de palavras com N processos
mpiexec -n N ./word_count_mpi.exe

# Contagem de artistas com N processos
mpiexec -n N ./artist_count_mpi.exe
```
