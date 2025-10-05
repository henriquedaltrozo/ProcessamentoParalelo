CC = gcc
CFLAGS = -Wall -O2 -std=c99
MPIINC = -I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
MPILIB = -L"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi

WORD_COUNT_EXEC = word_count_mpi.exe
ARTIST_COUNT_EXEC = artist_count_mpi.exe

WORD_COUNT_SRC = word_count_mpi.c
ARTIST_COUNT_SRC = artist_count_mpi.c

all: $(WORD_COUNT_EXEC) $(ARTIST_COUNT_EXEC)

$(WORD_COUNT_EXEC): $(WORD_COUNT_SRC)
	@echo Compilando programa de contagem de palavras...
	$(CC) $(CFLAGS) $(MPIINC) -o $(WORD_COUNT_EXEC) $(WORD_COUNT_SRC) $(MPILIB)
	@echo Programa $(WORD_COUNT_EXEC) compilado com sucesso!

$(ARTIST_COUNT_EXEC): $(ARTIST_COUNT_SRC)
	@echo Compilando programa de contagem de artistas...
	$(CC) $(CFLAGS) $(MPIINC) -o $(ARTIST_COUNT_EXEC) $(ARTIST_COUNT_SRC) $(MPILIB)
	@echo Programa $(ARTIST_COUNT_EXEC) compilado com sucesso!

run-words: $(WORD_COUNT_EXEC)
	@echo Executando contagem de palavras com 4 processos...
	mpiexec -n 4 ./$(WORD_COUNT_EXEC)

run-artists: $(ARTIST_COUNT_EXEC)
	@echo Executando contagem de artistas com 4 processos...
	mpiexec -n 4 ./$(ARTIST_COUNT_EXEC)

run-all: run-words run-artists

clean:
	@echo Removendo arquivos compilados...
	-del $(WORD_COUNT_EXEC) $(ARTIST_COUNT_EXEC) 2>nul || true
	-del word_count_results.txt artist_count_results.txt 2>nul || true
	@echo Limpeza concluída!

clean-exec:
	@echo Removendo executáveis...
	-del $(WORD_COUNT_EXEC) $(ARTIST_COUNT_EXEC) 2>nul || true

clean-results:
	@echo Removendo arquivos de resultado...
	-del word_count_results.txt artist_count_results.txt sentiment_analysis_results.txt 2>nul || true

sentiment-analysis:
	@echo Executando análise de sentimentos...
	@if not exist sentiment_analysis_env (echo Criando ambiente virtual Python... && python -m venv sentiment_analysis_env)
	@echo Ativando ambiente e executando análise...
	@call run_sentiment_analysis.bat

check-mpi:
	@echo Verificando instalação do MPI...
	@mpiexec
	@echo.
	@echo Verificando compilador gcc...
	@gcc --version

check-python:
	@echo Verificando instalação do Python...
	@python --version
	@echo.
	@echo Verificando Ollama...
	@ollama --version

help:
	@echo.
	@echo Makefile para programas de processamento de dados do Spotify
	@echo.
	@echo Comandos disponíveis:
	@echo   make                    - Compila ambos os programas MPI
	@echo   make all                - Compila ambos os programas MPI
	@echo   make word_count_mpi.exe - Compila apenas o programa de contagem de palavras
	@echo   make artist_count_mpi.exe - Compila apenas o programa de contagem de artistas
	@echo   make run-words          - Executa contagem de palavras com 4 processos
	@echo   make run-artists        - Executa contagem de artistas com 4 processos
	@echo   make run-all            - Executa ambos os programas MPI
	@echo   make sentiment-analysis - Executa análise de sentimentos (Python + Ollama)
	@echo   make clean              - Remove executáveis e arquivos de resultado
	@echo   make clean-exec         - Remove apenas os executáveis
	@echo   make clean-results      - Remove apenas os arquivos de resultado
	@echo   make check-mpi          - Verifica instalação do MPI
	@echo   make check-python       - Verifica instalação do Python e Ollama
	@echo   make help               - Mostra esta ajuda
	@echo.
	@echo Para executar com número diferente de processos:
	@echo   mpiexec -n [num_processos] ./word_count_mpi.exe
	@echo   mpiexec -n [num_processos] ./artist_count_mpi.exe
	@echo.

.PHONY: all run-words run-artists run-all sentiment-analysis clean clean-exec clean-results check-mpi check-python help