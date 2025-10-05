# Processamento Paralelo de Dados do Spotify

Este projeto implementa três aplicações para processar um grande dataset de músicas do Spotify:

1. **Contagem de Palavras** (MPI/C): Conta frequência de palavras nas letras
2. **Contagem de Artistas** (MPI/C): Conta músicas por artista  
3. **Análise de Sentimentos** (Python/Ollama): Classifica sentimentos das letras

## Pré-requisitos

- **MS-MPI**: Microsoft MPI instalado e configurado
- **GCC**: Compilador GCC disponível 
- **Python 3.8+**: Com pip instalado
- **Ollama**: Para inferência local de LLM
- **Dataset**: Arquivo `data/spotify_millsongdata.csv` no diretório do projeto

## Compilação e Execução

### Programas MPI (C)

```bash
# Compilar ambos os programas
make

# Executar contagem de palavras e artistas
make run-all

# Contagem de palavras com N processos
mpiexec -n N ./word_count_mpi.exe

# Contagem de artistas com N processos  
mpiexec -n N ./artist_count_mpi.exe
```

### Análise de Sentimentos (Python)

```bash
# Executar análise de sentimentos
.\run_sentiment_analysis.bat
```
