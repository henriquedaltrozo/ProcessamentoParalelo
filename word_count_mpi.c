#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 10000
#define MAX_WORD_LENGTH 100
#define MAX_WORDS 100000

typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordCount;

// Função para converter string para minúsculas
void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Função para limpar palavra de caracteres especiais
void clean_word(char *word) {
    int len = strlen(word);
    int i, j = 0;
    
    for (i = 0; i < len; i++) {
        if (isalnum(word[i])) {
            word[j++] = word[i];
        }
    }
    word[j] = '\0';
}

// Função para adicionar ou incrementar palavra no array
void add_word(WordCount *words, int *word_count, char *word) {
    if (strlen(word) == 0) return;
    
    // Procura se a palavra já existe
    for (int i = 0; i < *word_count; i++) {
        if (strcmp(words[i].word, word) == 0) {
            words[i].count++;
            return;
        }
    }
    
    // Se não existe, adiciona nova palavra
    if (*word_count < MAX_WORDS) {
        strcpy(words[*word_count].word, word);
        words[*word_count].count = 1;
        (*word_count)++;
    }
}

// Função para processar o texto de uma música
void process_lyrics(char *lyrics, WordCount *words, int *word_count) {
    char *token = strtok(lyrics, " \t\n\r,.-!?;:()[]{}\"'");
    
    while (token != NULL) {
        clean_word(token);
        to_lowercase(token);
        
        if (strlen(token) > 0) {  // Inclui todas as palavras
            add_word(words, word_count, token);
        }
        
        token = strtok(NULL, " \t\n\r,.-!?;:()[]{}\"'");
    }
}

// Função para mesclar contagens de palavras
void merge_word_counts(WordCount *global_words, int *global_count, 
                      WordCount *local_words, int local_count) {
    for (int i = 0; i < local_count; i++) {
        int found = 0;
        for (int j = 0; j < *global_count; j++) {
            if (strcmp(global_words[j].word, local_words[i].word) == 0) {
                global_words[j].count += local_words[i].count;
                found = 1;
                break;
            }
        }
        
        if (!found && *global_count < MAX_WORDS) {
            strcpy(global_words[*global_count].word, local_words[i].word);
            global_words[*global_count].count = local_words[i].count;
            (*global_count)++;
        }
    }
}

// Função de comparação para qsort
int compare_word_counts(const void *a, const void *b) {
    WordCount *wa = (WordCount *)a;
    WordCount *wb = (WordCount *)b;
    return wb->count - wa->count;
}

int main(int argc, char *argv[]) {
    int rank, size;
    FILE *file;
    char line[MAX_LINE_LENGTH];
    WordCount *local_words, *global_words;
    int local_word_count = 0, global_word_count = 0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Aloca memória para contagem de palavras
    local_words = (WordCount *)malloc(MAX_WORDS * sizeof(WordCount));
    global_words = (WordCount *)malloc(MAX_WORDS * sizeof(WordCount));
    
    // Abre o arquivo CSV
    file = fopen("data/spotify_millsongdata.csv", "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo CSV\n");
        MPI_Finalize();
        return 1;
    }
    
    // Pula o cabeçalho (todos os processos precisam fazer isso)
    fgets(line, MAX_LINE_LENGTH, file);
    
    int line_number = 0;
    int total_lines_read = 0;
    int records_processed = 0;
    
    // Processa TODAS as linhas do arquivo
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        total_lines_read++;
        
        // Verifica se esta linha parece ser início de um registro CSV
        // (não começa com espaço/tab e tem vírgulas)
        int is_record_start = (line[0] != ' ' && line[0] != '\t' && 
                              line[0] != '\n' && line[0] != '\r' &&
                              strchr(line, ',') != NULL);
        
        if (is_record_start) {
            // Esta linha pertence a este processo?
            if (line_number % size == rank) {
                // Faz uma cópia para não destruir a linha original
                char line_copy[MAX_LINE_LENGTH];
                strcpy(line_copy, line);
                
                // Parse da linha CSV para extrair o texto
                strtok(line_copy, ",");        // artist
                strtok(NULL, ",");           // song  
                strtok(NULL, ",");           // link
                char *text = strtok(NULL, "");
                
                if (text != NULL) {
                    // Remove aspas do início e fim se existirem
                    if (text[0] == '"') {
                        text++;
                        int len = strlen(text);
                        if (len > 0 && text[len-1] == '"') {
                            text[len-1] = '\0';
                        }
                    }
                    
                    // Processa as letras da música
                    char text_copy[MAX_LINE_LENGTH];
                    strcpy(text_copy, text);
                    process_lyrics(text_copy, local_words, &local_word_count);
                    records_processed++;
                }
            }
            line_number++;
        }
        // Continua lendo independentemente se é início de registro ou não
    }
    
    fclose(file);
    
    // Log removido para saída mais limpa
    
    // Coleta todos os resultados no processo 0
    if (rank == 0) {
        // Copia contagem local para global
        for (int i = 0; i < local_word_count; i++) {
            global_words[i] = local_words[i];
        }
        global_word_count = local_word_count;
        
        // Recebe contagens de outros processos
        for (int p = 1; p < size; p++) {
            int received_count;
            MPI_Recv(&received_count, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            WordCount *received_words = (WordCount *)malloc(received_count * sizeof(WordCount));
            MPI_Recv(received_words, received_count * sizeof(WordCount), MPI_BYTE, 
                    p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            merge_word_counts(global_words, &global_word_count, received_words, received_count);
            free(received_words);
        }
        
        // Ordena palavras por frequência
        qsort(global_words, global_word_count, sizeof(WordCount), compare_word_counts);
        
        // Escreve resultado em arquivo
        FILE *output_file = fopen("word_count_results.txt", "w");
        fprintf(output_file, "Contagem de Palavras nas Letras do Spotify\n");
        fprintf(output_file, "==========================================\n\n");
        
        for (int i = 0; i < global_word_count && i < 1000; i++) {  // Top 1000 palavras
            fprintf(output_file, "%d. %s: %d\n", i+1, global_words[i].word, global_words[i].count);
        }
        
        fclose(output_file);
        printf("Resultado salvo em word_count_results.txt\n");
        printf("Total de palavras unicas: %d\n", global_word_count);
        
    } else {
        // Envia contagem local para o processo 0
        MPI_Send(&local_word_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_words, local_word_count * sizeof(WordCount), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
    }
    
    free(local_words);
    free(global_words);
    
    MPI_Finalize();
    return 0;
}