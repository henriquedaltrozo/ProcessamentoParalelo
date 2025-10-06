#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define MAX_LINE_LENGTH 10000
#define MAX_WORD_LENGTH 100
#define MAX_WORDS 100000

void create_results_dir() {
#ifdef _WIN32
    _mkdir("results");
#else
    mkdir("results", 0755);
#endif
}

typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordCount;

void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void clean_word(char *word) {
    int len = strlen(word);
    int i, j = 0;
    
    for (i = 0; i < len; i++) {
        if (isalnum(word[i]) || word[i] == '\'') { 
            word[j++] = word[i];
        }
    }
    word[j] = '\0';
    
    if (word[0] == '\'') {
        memmove(word, word + 1, strlen(word));
    }
    len = strlen(word);
    if (len > 0 && word[len-1] == '\'') {
        word[len-1] = '\0';
    }
}

void add_word(WordCount *words, int *word_count, char *word) {
    if (strlen(word) == 0) return;
    
    for (int i = 0; i < *word_count; i++) {
        if (strcmp(words[i].word, word) == 0) {
            words[i].count++;
            return;
        }
    }
    
    if (*word_count < MAX_WORDS) {
        strcpy(words[*word_count].word, word);
        words[*word_count].count = 1;
        (*word_count)++;
    }
}

void process_lyrics(char *lyrics, WordCount *words, int *word_count) {
    char *token = strtok(lyrics, " \t\n\r,.-!?;:()[]{}\"");
    
    while (token != NULL) {
        clean_word(token);
        to_lowercase(token);
        
        if (strlen(token) > 0) { 
            add_word(words, word_count, token);
        }
        
        token = strtok(NULL, " \t\n\r,.-!?;:()[]{}\"");
    }
}

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

int compare_word_counts(const void *a, const void *b) {
    WordCount *wa = (WordCount *)a;
    WordCount *wb = (WordCount *)b;
    return wb->count - wa->count;
}

int main(int argc, char *argv[]) {
    int rank, size;
    double start_time, end_time, total_time;
    FILE *file;
    char line[MAX_LINE_LENGTH];
    WordCount *local_words, *global_words;
    int local_word_count = 0, global_word_count = 0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    local_words = (WordCount *)malloc(MAX_WORDS * sizeof(WordCount));
    global_words = (WordCount *)malloc(MAX_WORDS * sizeof(WordCount));
    
    file = fopen("data/spotify_millsongdata.csv", "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo CSV\n");
        MPI_Finalize();
        return 1;
    }
    
    fgets(line, MAX_LINE_LENGTH, file);
    
    int line_number = 0;
    int total_lines_read = 0;
    int records_processed = 0;
    
    char csv_buffer[MAX_LINE_LENGTH * 10];
    csv_buffer[0] = '\0';
    int in_quotes = 0;
    
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        total_lines_read++;
        
        strcat(csv_buffer, line);
        
        for (int i = 0; line[i]; i++) {
            if (line[i] == '"') {
                in_quotes = !in_quotes;
            }
        }
        
        if (!in_quotes && strlen(csv_buffer) > 0) {
            if (strchr(csv_buffer, ',') != NULL && strspn(csv_buffer, " \t\n\r") != strlen(csv_buffer)) {
                if (line_number % size == rank) {
                    char buffer_copy[MAX_LINE_LENGTH * 10];
                    strcpy(buffer_copy, csv_buffer);
                    
                    strtok(buffer_copy, ","); 
                    strtok(NULL, ",");         
                    strtok(NULL, ",");        
                    char *text = strtok(NULL, "");
                    
                    if (text != NULL && strlen(text) > 10) { 
                        if (text[0] == '"') {
                            text++;
                            int len = strlen(text);
                            if (len > 0 && text[len-1] == '"') {
                                text[len-1] = '\0';
                            }
                        }
                        
                        for (int i = 0; text[i]; i++) {
                            if (text[i] == '\n' || text[i] == '\r') {
                                text[i] = ' ';
                            }
                        }
                        
                        char text_copy[MAX_LINE_LENGTH * 10];
                        strcpy(text_copy, text);
                        process_lyrics(text_copy, local_words, &local_word_count);
                        records_processed++;
                    }
                }
                line_number++;
            }
            
            csv_buffer[0] = '\0';
        }
    }
    
    fclose(file);
    
    
    if (rank == 0) {
        for (int i = 0; i < local_word_count; i++) {
            global_words[i] = local_words[i];
        }
        global_word_count = local_word_count;
        
        for (int p = 1; p < size; p++) {
            int received_count;
            MPI_Recv(&received_count, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            WordCount *received_words = (WordCount *)malloc(received_count * sizeof(WordCount));
            MPI_Recv(received_words, received_count * sizeof(WordCount), MPI_BYTE, 
                    p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            merge_word_counts(global_words, &global_word_count, received_words, received_count);
            free(received_words);
        }
        
        qsort(global_words, global_word_count, sizeof(WordCount), compare_word_counts);
        
        create_results_dir();
        
        FILE *output_file = fopen("results/word_count_results.txt", "w");
        fprintf(output_file, "Contagem de Palavras nas Letras do Spotify\n");
        fprintf(output_file, "==========================================\n\n");
        
        for (int i = 0; i < global_word_count; i++) {  
            fprintf(output_file, "%d. %s: %d\n", i+1, global_words[i].word, global_words[i].count);
        }
        
        fclose(output_file);
        
        MPI_Barrier(MPI_COMM_WORLD);
        end_time = MPI_Wtime();
        total_time = end_time - start_time;
        
        printf("Resultado salvo em results/word_count_results.txt\n");
        printf("Total de palavras unicas: %d\n", global_word_count);
        printf("Tempo de execução: %.2f segundos\n", total_time);
        printf("Número de processos: %d\n", size);
        
    } else {
        MPI_Send(&local_word_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_words, local_word_count * sizeof(WordCount), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    free(local_words);
    free(global_words);
    
    MPI_Finalize();
    return 0;
}