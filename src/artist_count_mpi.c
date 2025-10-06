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

#define MAX_LINE_LENGTH 1000
#define MAX_ARTIST_NAME 200
#define MAX_ARTISTS 50000

// Função para criar diretório se não existir
void create_results_dir() {
#ifdef _WIN32
    _mkdir("results");
#else
    mkdir("results", 0755);
#endif
}

typedef struct {
    char name[MAX_ARTIST_NAME];
    int song_count;
} ArtistCount;

// Declarações das funções
int is_valid_artist_name(char *name);

// Função para adicionar ou incrementar artista no array
void add_artist(ArtistCount *artists, int *artist_count, char *artist_name) {
    if (!is_valid_artist_name(artist_name)) return;
    
    // Procura se o artista já existe
    for (int i = 0; i < *artist_count; i++) {
        if (strcmp(artists[i].name, artist_name) == 0) {
            artists[i].song_count++;
            return;
        }
    }
    
    // Se não existe, adiciona novo artista
    if (*artist_count < MAX_ARTISTS) {
        strcpy(artists[*artist_count].name, artist_name);
        artists[*artist_count].song_count = 1;
        (*artist_count)++;
    }
}

// Função para mesclar contagens de artistas
void merge_artist_counts(ArtistCount *global_artists, int *global_count, 
                        ArtistCount *local_artists, int local_count) {
    for (int i = 0; i < local_count; i++) {
        int found = 0;
        for (int j = 0; j < *global_count; j++) {
            if (strcmp(global_artists[j].name, local_artists[i].name) == 0) {
                global_artists[j].song_count += local_artists[i].song_count;
                found = 1;
                break;
            }
        }
        
        if (!found && *global_count < MAX_ARTISTS) {
            strcpy(global_artists[*global_count].name, local_artists[i].name);
            global_artists[*global_count].song_count = local_artists[i].song_count;
            (*global_count)++;
        }
    }
}

// Função de comparação para qsort
int compare_artist_counts(const void *a, const void *b) {
    ArtistCount *aa = (ArtistCount *)a;
    ArtistCount *ab = (ArtistCount *)b;
    return ab->song_count - aa->song_count;
}

// Função para extrair o artista de uma linha CSV completa
char* extract_artist_from_csv(char *line, char *artist_buffer) {
    int i = 0;
    int j = 0;
    
    // Limpa o buffer
    artist_buffer[0] = '\0';
    
    // O artista é o primeiro campo, vai até a primeira vírgula
    // Se começar com aspas, vai até as aspas de fechamento + vírgula
    if (line[0] == '"') {
        i = 1; // Pula a primeira aspa
        while (line[i] && j < MAX_ARTIST_NAME - 1) {
            if (line[i] == '"') {
                if (line[i+1] == '"') {
                    // Aspas duplas escapadas
                    artist_buffer[j++] = '"';
                    i += 2;
                } else {
                    // Fim do campo com aspas
                    break;
                }
            } else {
                artist_buffer[j++] = line[i++];
            }
        }
    } else {
        // Campo sem aspas, vai até a primeira vírgula
        while (line[i] && line[i] != ',' && j < MAX_ARTIST_NAME - 1) {
            artist_buffer[j++] = line[i++];
        }
    }
    
    artist_buffer[j] = '\0';
    
    // Remove espaços do início e fim
    char *start = artist_buffer;
    while (*start == ' ' || *start == '\t') start++;
    
    if (start != artist_buffer) {
        memmove(artist_buffer, start, strlen(start) + 1);
    }
    
    int len = strlen(artist_buffer) - 1;
    while (len >= 0 && (artist_buffer[len] == ' ' || artist_buffer[len] == '\t' || 
                        artist_buffer[len] == '\n' || artist_buffer[len] == '\r')) {
        artist_buffer[len] = '\0';
        len--;
    }
    
    return artist_buffer;
}

// Função para verificar se uma linha parece ser início de um novo registro CSV
int is_new_csv_record(char *line) {
    // Verifica se tem o padrão básico de um registro CSV
    // Não deve começar com espaço/tab (que indica continuação de texto)
    if (line[0] == ' ' || line[0] == '\t' || line[0] == '\n' || line[0] == '\r') return 0;
    
    // Linha muito curta provavelmente não é um registro
    if (strlen(line) < 10) return 0;
    
    // Deve ter pelo menos 2 vírgulas (mínimo para artist,song,link)
    int comma_count = 0;
    int in_quotes = 0;
    
    for (int i = 0; line[i] && i < 500; i++) {  // Limita busca para performance
        if (line[i] == '"') {
            in_quotes = !in_quotes;
        } else if (line[i] == ',' && !in_quotes) {
            comma_count++;
            if (comma_count >= 2) return 1;  // Retorna logo que encontra 2 vírgulas
        }
    }
    
    return comma_count >= 2;
}

// Função para verificar se é um nome de artista válido
int is_valid_artist_name(char *name) {
    if (strlen(name) == 0) return 0;
    
    // Muito curto provavelmente não é nome de artista
    if (strlen(name) < 2) return 0;
    
    // Lista expandida de palavras/frases comuns que não são artistas
    const char* common_phrases[] = {
        "Oh", "Yeah", "No", "Hey", "Well", "Ooh", "Yes", "Baby", "La", "Ah", 
        "So", "I", "You", "And", "The", "A", "An", "In", "On", "At", "To", 
        "For", "With", "By", "Of", "Is", "Go", "Do", "Be", "We", "He", "She", 
        "It", "My", "Our", "His", "Her", "Come on", "I said", "But", "Whoa", 
        "Now", "Lord", "Who", "What", "When", "Where", "Why", "How", "All", 
        "Some", "Many", "Few", "One", "Two", "Three", "First", "Last", "Next",
        "Here", "There", "This", "That", "These", "Those", "Come", "Go", "Get",
        "Make", "Take", "Give", "Want", "Need", "Like", "Love", "Know", "Think",
        "Say", "Tell", "Ask", "Look", "See", "Find", "Feel", "Hear", "Listen",
        "Talk", "Speak", "Call", "Try", "Help", "Work", "Play", "Stop", "Start",
        "End", "Begin", "Keep", "Let", "Put", "Turn", "Move", "Run", "Walk",
        "Sit", "Stand", "Open", "Close", "Show", "Hide", "Send", "Bring",
        // Palavras adicionais que não são artistas
        "Na", "Da", "De", "Do", "Em", "Para", "Por", "Com", "Sem", "Mas",
        "Ou", "E", "O", "A", "Um", "Uma", "Uns", "Umas", "Este", "Esta",
        "Isto", "Esse", "Essa", "Isso", "Aquele", "Aquela", "Aquilo",
        "Meu", "Minha", "Seu", "Sua", "Nosso", "Nossa", "Dele", "Dela",
        "America", "América", "American", "Americain", "Americana",
        "Don't", "Can't", "Won't", "Didn't", "Couldn't", "Wouldn't", "Shouldn't",
        "Hasn't", "Haven't", "Isn't", "Aren't", "Wasn't", "Weren't", "Will",
        "Would", "Should", "Could", "Might", "Must", "Shall", "May", "Can",
        "Up", "Out", "Down", "Over", "Under", "Through", "Into", "Onto", "From",
        "Back", "Away", "Home", "Off", "Around", "Along", "Across", "Past",
        "Never", "Always", "Sometimes", "Often", "Usually", "Rarely", "Seldom",
        "Just", "Only", "Even", "Still", "Yet", "Already", "Soon", "Later",
        "Today", "Tomorrow", "Yesterday", "Tonight", "Morning", "Evening",
        "Way", "Time", "Day", "Night", "Year", "Life", "World", "People",
        "Place", "Thing", "Man", "Woman", "Girl", "Boy", "Child", "Mother",
        "Father", "Friend", "Heart", "Mind", "Soul", "Body", "Eyes", "Face",
        "Hands", "Voice", "Words", "Music", "Song", "Dance", "Party", "Fun",
        "Good", "Bad", "Right", "Wrong", "True", "False", "Real", "New", "Old",
        "Big", "Small", "Long", "Short", "Hot", "Cold", "Sweet", "Pretty",
        "Beautiful", "Wonderful", "Amazing", "Special", "Perfect", "Great",
        "Money", "Power", "Fame", "Success", "Failure", "Dreams", "Hope",
        "Pain", "Love", "Hate", "Peace", "War", "Freedom", "Truth", "Lies",
        // Interjeições e expressões comuns em letras
        "Uh", "Uhh", "Uhhh", "Mm", "Mmm", "Mmmm", "Hm", "Hmm", "Hmmm",
        "Aha", "Uh-huh", "Uh-oh", "Whoa", "Wow", "Yay", "Yep", "Nope",
        // Frases comuns em letras de música
        "I know", "I think", "I feel", "I want", "I need", "I love", "I hate",
        "She said", "He said", "They said", "We said", "You said", "I said",
        "She's got", "He's got", "I've got", "You've got", "We've got",
        "Don't know", "Don't care", "Don't want", "Don't need", "Don't stop",
        "Can't stop", "Won't stop", "Can't wait", "Won't wait", "Don't wait",
        NULL
    };
    
    // Verifica se é uma palavra/frase comum
    for (int i = 0; common_phrases[i]; i++) {
        if (strcmp(name, common_phrases[i]) == 0) {
            return 0;
        }
    }
    
    // Se for muito curto e só minúsculas, provavelmente não é artista
    if (strlen(name) <= 3) {
        int has_upper = 0;
        for (int i = 0; name[i]; i++) {
            if (isupper(name[i])) {
                has_upper = 1;
                break;
            }
        }
        if (!has_upper) return 0;
    }
    
    // Se tem espaços mas parece ser apenas palavras comuns, rejeita
    if (strchr(name, ' ')) {
        char temp[MAX_ARTIST_NAME];
        strcpy(temp, name);
        char *word = strtok(temp, " ");
        int common_word_count = 0;
        int total_words = 0;
        
        while (word) {
            total_words++;
            for (int i = 0; common_phrases[i]; i++) {
                if (strcmp(word, common_phrases[i]) == 0) {
                    common_word_count++;
                    break;
                }
            }
            word = strtok(NULL, " ");
        }
        
        // Se mais de 50% das palavras são comuns, provavelmente não é artista
        if (total_words > 0 && (common_word_count * 2) > total_words) {
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    int rank, size;
    FILE *file;
    char line[MAX_LINE_LENGTH];
    ArtistCount *local_artists, *global_artists;
    int local_artist_count = 0, global_artist_count = 0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Aloca memória para contagem de artistas
    local_artists = (ArtistCount *)malloc(MAX_ARTISTS * sizeof(ArtistCount));
    global_artists = (ArtistCount *)malloc(MAX_ARTISTS * sizeof(ArtistCount));
    
    // Abre o arquivo CSV
    file = fopen("data/spotify_millsongdata.csv", "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo CSV\n");
        MPI_Finalize();
        return 1;
    }
    
    // Pula o cabeçalho (todos os processos precisam fazer isso)
    fgets(line, MAX_LINE_LENGTH, file);
    
    int processed_records = 0;
    int total_lines = 0;
    int record_number = 0;
    char artist_buffer[MAX_ARTIST_NAME];
    
    // Processa TODAS as linhas do arquivo
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        total_lines++;
        
        // Verifica se é uma linha de início de novo registro CSV
        if (is_new_csv_record(line)) {
            // Esta linha pertence ao processo atual?
            if (record_number % size == rank) {
                // Extrai o nome do artista desta linha
                char *artist = extract_artist_from_csv(line, artist_buffer);
                if (strlen(artist) > 0) {
                    add_artist(local_artists, &local_artist_count, artist);
                    processed_records++;
                }
            }
            record_number++;
        }
        // Continua lendo mesmo se não for início de registro
    }
    
    fclose(file);
    
    // Log removido para saída mais limpa
    
    // Coleta todos os resultados no processo 0
    if (rank == 0) {
        // Copia contagem local para global
        for (int i = 0; i < local_artist_count; i++) {
            global_artists[i] = local_artists[i];
        }
        global_artist_count = local_artist_count;
        
        // Recebe contagens de outros processos
        for (int p = 1; p < size; p++) {
            int received_count;
            MPI_Recv(&received_count, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            ArtistCount *received_artists = (ArtistCount *)malloc(received_count * sizeof(ArtistCount));
            MPI_Recv(received_artists, received_count * sizeof(ArtistCount), MPI_BYTE, 
                    p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            merge_artist_counts(global_artists, &global_artist_count, received_artists, received_count);
            free(received_artists);
        }
        
        // Ordena artistas por número de músicas
        qsort(global_artists, global_artist_count, sizeof(ArtistCount), compare_artist_counts);
        
        // Cria diretório results se não existir
        create_results_dir();
        
        // Escreve resultado em arquivo
        FILE *output_file = fopen("results/artist_count_results.txt", "w");
        fprintf(output_file, "Artistas com Mais Musicas no Spotify\n");
        fprintf(output_file, "====================================\n\n");
        
        int total_songs = 0;
        for (int i = 0; i < global_artist_count; i++) {
            total_songs += global_artists[i].song_count;
        }
        
        fprintf(output_file, "Total de artistas unicos: %d\n", global_artist_count);
        fprintf(output_file, "Total de musicas: %d\n\n", total_songs);
        fprintf(output_file, "Todos os Artistas (ordenados por número de músicas):\n");
        fprintf(output_file, "--------------------------------------------------\n");
        
        for (int i = 0; i < global_artist_count; i++) {  // Todos os artistas
            fprintf(output_file, "%d. %s: %d musicas\n", 
                   i+1, global_artists[i].name, global_artists[i].song_count);
        }
        
        fclose(output_file);
        printf("Resultado salvo em results/artist_count_results.txt\n");
        printf("Total de artistas unicos: %d\n", global_artist_count);
        printf("Total de musicas processadas: %d\n", total_songs);
        
    } else {
        // Envia contagem local para o processo 0
        MPI_Send(&local_artist_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(local_artists, local_artist_count * sizeof(ArtistCount), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
    }
    
    free(local_artists);
    free(global_artists);
    
    MPI_Finalize();
    return 0;
}