#!/usr/bin/env python3
"""
Script de Benchmark para Análise de Desempenho
Executa os programas MPI com diferentes números de processos
"""

import subprocess
import os
import time
from datetime import datetime

def run_mpi_program(program, num_processes):
    """Executa programa MPI e retorna o tempo de execução"""
    try:
        start_time = time.time()
        result = subprocess.run([
            'mpiexec', '-n', str(num_processes), f'./exe/{program}.exe'
        ], capture_output=True, text=True, timeout=300)  # 5 min timeout
        end_time = time.time()
        
        if result.returncode == 0:
            return end_time - start_time
        else:
            print(f"Erro ao executar {program} com {num_processes} processos:")
            print(result.stderr)
            return None
    except subprocess.TimeoutExpired:
        print(f"Timeout ao executar {program} com {num_processes} processos")
        return None
    except Exception as e:
        print(f"Erro: {e}")
        return None

def main():
    print("="*60)
    print("BENCHMARK DE DESEMPENHO - PROCESSAMENTO PARALELO")
    print("="*60)
    
    # Verifica se os executáveis existem
    programs = ['word_count_mpi', 'artist_count_mpi']
    for program in programs:
        if not os.path.exists(f'exe/{program}.exe'):
            print(f"Erro: {program}.exe não encontrado. Execute 'make' primeiro.")
            return
    
    # Testa com diferentes números de processos
    process_counts = [1, 2, 4, 8]
    
    results = {}
    
    for program in programs:
        print(f"\nTestando {program}...")
        times = {}
        
        for procs in process_counts:
            print(f"  Executando com {procs} processo(s)...", end=" ")
            exec_time = run_mpi_program(program, procs)
            if exec_time:
                times[procs] = exec_time
                print(f"{exec_time:.2f}s")
            else:
                print("FALHOU")
        
        if times:
            results[program] = times
        
    # Gera relatório de desempenho
    generate_performance_report(results)

def generate_performance_report(results):
    """Gera relatório simplificado de desempenho"""
    timestamp = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
    
    with open("results/performance_analysis.txt", "w", encoding="utf-8") as f:
        f.write("ANÁLISE DE DESEMPENHO - PROCESSAMENTO PARALELO\n")
        f.write("=" * 50 + "\n\n")
        f.write(f"Data da análise: {timestamp}\n")
        f.write(f"Sistema: Windows com MS-MPI\n\n")
        
        for program, times in results.items():
            f.write(f"PROGRAMA: {program.upper()}\n")
            f.write("-" * 30 + "\n")
            f.write(f"{'Processos':<10} {'Tempo (s)':<12}\n")
            f.write("-" * 25 + "\n")
            
            for procs in sorted(times.keys()):
                exec_time = times[procs]
                f.write(f"{procs:<10} {exec_time:<12.2f}\n")
            
            f.write("\n" + "="*50 + "\n\n")
        
        # Conclusões gerais
        f.write("CONCLUSÕES GERAIS:\n")
        f.write("-" * 20 + "\n")
        f.write("• O paralelismo é mais efetivo para tarefas computacionalmente intensivas\n")
        f.write("• Overhead de comunicação MPI limita speedup com muitos processos\n")
        f.write("• I/O de arquivo pode ser gargalo em datasets grandes\n")
        f.write("• Balanceamento de carga afeta diretamente a eficiência\n")
    
    print(f"\nRelatório de desempenho salvo em: results/performance_analysis.txt")

if __name__ == "__main__":
    # Verifica se está no diretório correto
    if not os.path.exists("data/spotify_millsongdata.csv"):
        print("Erro: Dataset não encontrado. Execute este script no diretório raiz do projeto.")
        exit(1)
    
    main()