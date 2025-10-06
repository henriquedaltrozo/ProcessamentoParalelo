#!/usr/bin/env python3
"""
Análise de Sentimentos das Letras do Spotify - Versão Final
Classifica letras em: Positiva, Neutra, Negativa usando Ollama
"""

import csv
import os
import re
import time
import io
from collections import defaultdict

try:
    import ollama
except ImportError:
    print("Erro: Biblioteca 'ollama' não encontrada.")
    print("Execute: pip install ollama")
    exit(1)

def extract_lyrics_from_csv(line):
    """Extrai letras da linha CSV usando csv.reader"""
    try:
        reader = csv.reader(io.StringIO(line))
        row = next(reader)
        if len(row) >= 4:
            return row[3]  # Campo 4 = letras
    except:
        pass
    return None

def clean_lyrics(text):
    """Limpa e prepara o texto das letras"""
    if not text:
        return ""
    
    # Remove tags HTML
    text = re.sub(r'<[^>]+>', '', text)
    # Remove caracteres de escape
    text = text.replace('\\n', '\n').replace('\\r', '\r')
    text = text.replace('\\"', '"').replace("\\'", "'")
    # Normaliza espaços
    text = re.sub(r'\s+', ' ', text)
    
    return text.strip()

def classify_sentiment(lyrics, model="llama3:latest"):
    """Classifica o sentimento das letras usando Ollama"""
    if not lyrics or len(lyrics) < 15:
        return "Neutra"
    
    # Limita tamanho para evitar timeout
    if len(lyrics) > 1000:
        lyrics = lyrics[:1000] + "..."
    
    prompt = f"""Analise o sentimento geral desta letra de música e classifique em uma categoria:

LETRA:
{lyrics}

Classifique como:
- Positiva: se expressa alegria, amor, esperança, felicidade
- Negativa: se expressa tristeza, raiva, dor, desespero  
- Neutra: se não tem sentimento forte ou é ambígua

Responda APENAS com uma palavra: Positiva, Negativa ou Neutra"""

    try:
        response = ollama.generate(
            model=model,
            prompt=prompt,
            options={
                'temperature': 0.1,
                'num_predict': 5
            }
        )
        
        result = response['response'].strip().lower()
        
        # Normaliza resposta
        if 'positiva' in result or 'positive' in result:
            return "Positiva"
        elif 'negativa' in result or 'negative' in result:
            return "Negativa"
        else:
            return "Neutra"
            
    except Exception as e:
        print(f"Erro na classificação: {e}")
        return "Neutra"

def main():
    start_time = time.time()
    print("=" * 60)
    print("ANÁLISE DE SENTIMENTOS DAS LETRAS DO SPOTIFY")
    print("=" * 60)
    
    # Verifica conexão Ollama
    try:
        models = ollama.list()
        available_models = [m.get('name', '') for m in models.get('models', [])]
        print(f"✓ Ollama conectado")
        print(f"  Modelos disponíveis: {available_models}")
        
        # Usa primeiro modelo disponível que contenha 'llama'
        model_to_use = "llama3:latest"
        for model in available_models:
            if 'llama' in model.lower():
                model_to_use = model
                break
        
        print(f"  Usando modelo: {model_to_use}")
        
    except Exception as e:
        print(f"✗ Erro no Ollama: {e}")
        print("Certifique-se de que o Ollama está rodando com: ollama serve")
        return
    
    # Configurações
    csv_file = "../data/spotify_millsongdata.csv"
    max_songs = 20  # Limite otimizado para demonstração
    
    counts = defaultdict(int)
    processed = 0
    skipped = 0
    
    print(f"\nProcessando até {max_songs} músicas do arquivo: {csv_file}")
    print("Iniciando análise...\n")
    
    try:
        with open(csv_file, 'r', encoding='utf-8', errors='ignore') as f:
            # Pula cabeçalho
            header = next(f)
            
            for line_num, line in enumerate(f, 1):
                if processed >= max_songs:
                    break
                
                # Extrai letras
                lyrics = extract_lyrics_from_csv(line)
                if not lyrics:
                    skipped += 1
                    continue
                
                # Limpa letras
                clean_text = clean_lyrics(lyrics)
                if len(clean_text) < 20:  # Muito curta
                    skipped += 1
                    continue
                
                # Classifica sentimento
                sentiment = classify_sentiment(clean_text, model_to_use)
                counts[sentiment] += 1
                processed += 1
                
                # Log progresso
                if processed % 10 == 0:
                    print(f"Processadas {processed} músicas - "
                          f"Positiva: {counts['Positiva']}, "
                          f"Neutra: {counts['Neutra']}, "
                          f"Negativa: {counts['Negativa']}")
                
                # Pausa para não sobrecarregar
                time.sleep(0.3)
    
    except FileNotFoundError:
        print(f"Erro: Arquivo {csv_file} não encontrado")
        return
    except Exception as e:
        print(f"Erro durante processamento: {e}")
        return
    
    # Resultados finais
    total = sum(counts.values())
    
    print("\n" + "=" * 60)
    print("RESULTADOS DA ANÁLISE DE SENTIMENTOS")
    print("=" * 60)
    print(f"Músicas processadas: {processed}")
    print(f"Músicas ignoradas: {skipped}")
    print(f"Modelo utilizado: {model_to_use}")
    print()
    print("Distribuição de Sentimentos:")
    print("-" * 30)
    
    for sentiment in ["Positiva", "Neutra", "Negativa"]:
        count = counts[sentiment]
        percentage = (count / total * 100) if total > 0 else 0
        print(f"{sentiment:>10}: {count:>4} músicas ({percentage:>5.1f}%)")
    
    print("-" * 30)
    print(f"{'Total':>10}: {total:>4} músicas")
    
    # Salva resultados
    output_dir = os.path.join(os.path.dirname(__file__), "..", "results")
    os.makedirs(output_dir, exist_ok=True)  # cria a pasta se não existir
    output_file = os.path.join(output_dir, "sentiment_analysis_results.txt")
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("Análise de Sentimentos das Letras do Spotify\n")
            f.write("=" * 50 + "\n\n")
            f.write(f"Data da análise: {time.strftime('%d/%m/%Y %H:%M:%S')}\n")
            f.write(f"Modelo utilizado: {model_to_use}\n")
            f.write(f"Músicas processadas: {processed}\n")
            f.write(f"Músicas ignoradas: {skipped}\n\n")
            
            f.write("Distribuição de Sentimentos:\n")
            f.write("-" * 30 + "\n")
            
            for sentiment in ["Positiva", "Neutra", "Negativa"]:
                count = counts[sentiment]
                percentage = (count / total * 100) if total > 0 else 0
                f.write(f"{sentiment}: {count} músicas ({percentage:.1f}%)\n")
            
            f.write(f"\nTotal: {total} músicas\n")
            
            # Adiciona interpretação dos resultados
            f.write("\nInterpretação dos Resultados:\n")
            f.write("-" * 30 + "\n")
            
            if counts['Positiva'] > counts['Negativa']:
                f.write("• Predominância de letras com sentimento POSITIVO\n")
            elif counts['Negativa'] > counts['Positiva']:
                f.write("• Predominância de letras com sentimento NEGATIVO\n")
            else:
                f.write("• Equilíbrio entre sentimentos positivos e negativos\n")
            
            neutra_pct = counts['Neutra'] / total * 100 if total > 0 else 0
            if neutra_pct > 40:
                f.write("• Alta porcentagem de letras neutras/ambíguas\n")
        
        print(f"\n✓ Resultados salvos em: {output_file}")
        
    except Exception as e:
        print(f"Erro ao salvar resultados: {e}")
    
    # Finaliza medição de tempo
    end_time = time.time()
    total_time = end_time - start_time
    print(f"Tempo de execução: {total_time:.2f} segundos")
    print("=" * 60)

if __name__ == "__main__":
    main()