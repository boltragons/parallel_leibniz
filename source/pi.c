#include "pi.h"

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>

int main() {
    // Obtém a localidade pelo SO (dinamiza o uso do . ou ,).
    setlocale(LC_ALL, "");

    // Executa o processamento.
    return pi();
}

int createReport(const Report *report) {
    // Verifica se as strings não são vazias ou nulas.
    if (!validateReport(report)) {
        return FALSE;
    }

    // Imprime o relatório na tela.
    printf(
        "%s\n\n"    // Cálculo do Número π
        "%s\n"      // Criando os processos filhos pi1 e pi2...
        "%s\n\n"    // Processo pai (PID 6923) finalizou sua execução.
        "%s\n\n"    // - Processo Filho: pi1 (PID 6924)
        "\t%s\n\n"  // Nº de threads: 16
        "\t%s\n"    // Início: 10:45:12
        "\t%s\n"    // Fim: 10:45:21
        "\t%s\n\n"  // Duração: 9,59 s
        "\t%s\n\n"  // Pi = 3,141592653
        "%s\n\n"    // - Processo Filho: pi2 (PID 6925)
        "\t%s\n\n"  // Nº de threads: 16
        "\t%s\n"    // Início: 10:45:14
        "\t%s\n"    // Fim: 10:45:24
        "\t%s\n\n"  // Duração: 10,00 s
        "\t%s\n",   // Pi = 3,141592653
    report->programName, report->message1, report->message2,
    report->processReport1.identification, report->processReport1.numberOfThreads, report->processReport1.start,
    report->processReport1.end, report->processReport1.duration, report->processReport1.pi,
    report->processReport2.identification, report->processReport2.numberOfThreads, report->processReport2.start,
    report->processReport2.end, report->processReport2.duration, report->processReport2.pi);
    
    return TRUE;
}

int createFile(const FileName fileName, String description, const Threads threads) {
    // Verifica se as strings não estão vazias ou nulas. 
    const char *strings[] = {fileName, description};
    if (!validateStrings(strings, 2) || !threads) {
        return FALSE;
    }

    FILE *file = fopen(fileName, "w+");
    if (!file) {
        return FALSE;
    }

    // Escreve o cabeçalho do arquivo.
    fprintf(file, "Arquivo: %s\nDescription: %s\n\n", fileName, description);
    
    // Escreve os tempos de cada thread nos arquivos.
    fillThreadsTimes(file, threads);

    fclose(file);
    return TRUE;
}

pthread_t createThread(unsigned int *terms) {
    ThreadArgs *thread_args = (ThreadArgs *) terms;

    // Cria uma thread para operar com os argumentos passados e executar a função sumPartial.
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, sumPartial, thread_args);

    return thread_id;
}

void* sumPartial(void *terms) {
    // Tempo anterior ao processamento da thread.
    Time start_time;
    getTime(&start_time);

    // Obtém os parâmetros da Thread.
    ThreadArgs *thread_args = (ThreadArgs *) terms;
    
    // Processa PARTIAL_NUMBER_OF_TERMS após thread_args->first_term.
    double pi_approximation = partialLeibnizFormula(thread_args->first_term);

    // Obtém o lock do mutem para modificar o acumulador do pi.
    pthread_mutex_lock(thread_args->pi_mutex);
    *thread_args->pi_approximation += pi_approximation;
    pthread_mutex_unlock(thread_args->pi_mutex);
    
    // Tempo após todo o processamento da thread.
    Time end_time;
    getTime(&end_time);

    // Obtém o tempo empregado na thread.
    thread_args->thread_infos->time = timeDifference(&start_time, &end_time);

    thread_args->thread_infos->tid = gettid();
    return NULL;
}

double calculationOfNumberPi(unsigned int terms) {
    // As informações a serem preenchidas pelas threads.
    Threads threads_infos;

    // Cria 16 threads para calcular as threads.
    double pi_approximation = createPiThreads(threads_infos);

    FileName file_name;
    snprintf(file_name, FILE_NAME_SIZE, "pi%d.txt", terms);

    String description;
    snprintf(description, STRING_DEFAULT_SIZE, "Tempo em segundos das %d threads do processo filho pi%d.", NUMBER_OF_THREADS, terms);

    // Cria o arquivo com as informações preenchidas pelas threads.
    if (!createFile(file_name, description, threads_infos)) {
        fprintf(stderr, "Houve um erro ao gerar o relatório do processo pi%d.\n", terms);
        exit(FALSE);
    }

    return pi_approximation;
}

int pi() {
    int shared_memory_id = createSharedMemory();
    if (!shared_memory_id) {
        return EXIT_FAILURE;
    }

    // Cria os processos pi1 e pi2.
    if (!manageProcesses(shared_memory_id)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// ====================
// Adições:
// ====================

int createSharedMemory(void) {
    // Cria o arquivo temporario que será utilizado para criar a chave de acesso à memória compartilhada.
    FILE *file = fopen(SHARED_MEMORY_KEY_PATH, "w+");
    if (!file) {
        perror("Não foi possível criar o arquivo temporario utilizado para criar a chave de acesso para compartilhar memória");
        return FALSE;
    }
    fclose(file);

    // Cria uma chave de acesso à memória compartilhada entre processos
    key_t shared_memory_key = ftok(SHARED_MEMORY_KEY_PATH, 1);
    if (shared_memory_key == -1) {
        perror("Não foi possível criar uma chave de acesso para compartilhar memória");
        remove(SHARED_MEMORY_KEY_PATH);
        return FALSE;
    }

    // Remove o arquivo temporário.
    remove(SHARED_MEMORY_KEY_PATH);

    // Cria uma área de memória (referente à struct Report) a ser compartilhada entre processos.
    int shared_memory_id = shmget(shared_memory_key, sizeof(Report), IPC_CREAT | READ_WRITE_PERMISSIONS);
    if (shared_memory_id == -1) {
        perror("Não foi possível criar uma área de memória compartilhada");
        return FALSE;
    }

    return shared_memory_id;
}

Report *getSharedMemory(int shared_memory_id) {
    Report *report = shmat(shared_memory_id, NULL, 0);
    if (report == (void *) -1) {
        perror("Não foi possível obter o endereço da área de memória compartilhada");
        return NULL;
    }
    return report;
}

int yieldToChildProcess(int shared_memory_id, ProcessNumber process_number) {
    // Obtém o endereço da memória compartilhada.
    Report *report = getSharedMemory(shared_memory_id);
    if(!report) {
        return FALSE;
    }

    // Obtém o report do processo em questão.
    ProcessReport *process_report = (process_number == PROCESS_1)?
        &report->processReport1 : &report->processReport2;

    // Processo filho executa e preenche o Report compartilhado.
    performCalculation(process_report, process_number);

    // Remove a área compartilhada do espaço de endereçamento do processo em execução.
    shmdt(report);  

    return TRUE;
}

int yieldToFatherProcess(int shared_memory_id) {
    // Processo pai espera os filhos terminarem a execução.
    while(wait(NULL) > 0);

    // Obtém o endereço da memória compartilhada.
    Report *report = getSharedMemory(shared_memory_id);
    if(!report) {
        return FALSE;
    }

    // Preenche Report com os dados do processo pai.
    strncpy(report->programName, "Cálculo do Número π", STRING_DEFAULT_SIZE);
    strncpy(report->message1, "Criando os processos filhos pi1 e pi2...", STRING_DEFAULT_SIZE);
    snprintf(report->message2, STRING_DEFAULT_SIZE, "Processo pai (PID %d) finalizou sua execução.", getpid());

    // Mostra o relatório compartilhado no stdout.
    if (!createReport(report)) {
        perror("Não foi possível criar o relatório pois os dados são inválidos");
        return FALSE;
    }

    // Remove a área compartilhada do espaço de endereçamento do processo em execução.
    shmdt(report);

    // Remove a área compartilhada da memória.
    shmctl(shared_memory_id, IPC_RMID, NULL);

    return TRUE;
}

int validateReport(const Report *report) {
    if (!report) {
        return FALSE;
    }

    // Vetor das strings a validar.
    const char *strings[] = {report->programName, report->message1, report->message2,
        report->processReport1.identification, report->processReport1.numberOfThreads, report->processReport1.start,
        report->processReport1.end, report->processReport1.duration, report->processReport1.pi,
        report->processReport2.identification, report->processReport2.numberOfThreads, report->processReport2.start,
        report->processReport2.end, report->processReport2.duration, report->processReport2.pi
    };

    return validateStrings(strings, sizeof(strings)/sizeof(char *));   
}

int validateStrings(const char **strings, size_t n) {
    if (!strings) {
        return FALSE;
    }

    // Verifica se alguma string é nula ou vazia
    for (size_t i = 0; i < n; i++) {
        if (!strings[i] || strlen(strings[i]) < 1) {
            return FALSE;
        }
    }

    return TRUE;
}

void fillThreadsTimes(FILE *file, const Threads threads) {
    if (!file || !threads) {
        return;
    }

    // Acumula os tempos de conclusão de cada thread.
    double total_time = 0;
    for (int thread_num = 0; thread_num < NUMBER_OF_THREADS; thread_num++) {
        total_time += threads[thread_num].time;
        fprintf(file, "TID %d: %.2lf\n", threads[thread_num].tid, threads[thread_num].time);
    }

    // Escreve o tempo total calculado no arquivo.
    fprintf(file, "\nTotal: %.2lf s\n", total_time);
}

double partialLeibnizFormula(int first_therm) {
    // A função irá processar PARTIAL_NUMBER_OF_TERMS termos.
    const unsigned int number_terms = first_therm + PARTIAL_NUMBER_OF_TERMS;
    
    // Aproxima o pi, de first_therm até number_terms - 1.
    double pi_approximation = 0;
    double signal = 1.0;
    for (unsigned int n = first_therm; n < number_terms; n++) {
        pi_approximation += signal/(2*n + 1);
        signal *= -1.0;
    }

    return pi_approximation;
}

void performCalculation(ProcessReport *report, ProcessNumber process) {
    if (!report) {
        return;
    }

    // Obtém o tempo antes do cálculo.
    Time start_time;
    getTime(&start_time);

    double pi = calculationOfNumberPi(process);
    
    // Obtém o tempo após do cálculo.
    Time end_time;
    getTime(&end_time);

    // Preenche dados do relatório do processo filho.
    snprintf(report->identification, STRING_DEFAULT_SIZE, "- Processo Filho: pi%d (PID %d)", process, getpid());
    snprintf(report->numberOfThreads, STRING_DEFAULT_SIZE, "Nº de Threads: %d", NUMBER_OF_THREADS);
    snprintf(report->pi, STRING_DEFAULT_SIZE, "Pi = %.9lf", pi);

    // Preenche dados do relatório do processo filho relacionados ao tempo.
    fillTimeReport(report, &start_time, &end_time);
}

double createPiThreads(Threads threads_infos) {
    if (!threads_infos) {
        return 0.0;
    }

    // Os NUMBER_OF_THREADS argumentos das threads.
    ThreadArgs thread_args[NUMBER_OF_THREADS];

    // O mutex binário para proteger "pi_approximation".
    pthread_mutex_t pi_mutex;
    pthread_mutex_init(&pi_mutex, NULL);

    double pi_approximation = 0;

    // Cria cada thread para operar a PARTIAL_NUMBER_OF_TERMS termos de distância uma da outra,
    // acessando pi_approximation via o mutex, e preenchendo os argumentos.
    for (int thread_num = 0; thread_num < NUMBER_OF_THREADS; thread_num++) {
        thread_args[thread_num].first_term = thread_num*PARTIAL_NUMBER_OF_TERMS;
        thread_args[thread_num].pi_approximation = &pi_approximation;
        thread_args[thread_num].pi_mutex = &pi_mutex;
        thread_args[thread_num].thread_infos = &threads_infos[thread_num];
        
        threads_infos[thread_num].threadID = createThread((unsigned int *) &thread_args[thread_num]);
    }

    // Espera que toda as threads terminem. 
    for (int thread_num = 0; thread_num < NUMBER_OF_THREADS; thread_num++) {
        pthread_join(threads_infos[thread_num].threadID, NULL);
    }

    return 4*pi_approximation;
}

double timeDifference(const Time *start_time, const Time *end_time) {
    if (!start_time || !end_time) {
        return 0.0;
    }
    return (end_time->tv_sec - start_time->tv_sec) + (end_time->tv_usec - start_time->tv_usec)/1000000.0;
}

void getTime(Time *time) {
    if (!time) {
        return;
    }
    gettimeofday(time, NULL);
}

TimeInfos *getTimeInfos(const Time *time) {
    if (!time) {
        return NULL;
    }
    return localtime(&time->tv_sec);
}

void fillTimeReport(ProcessReport *report, const Time *start_time, const Time *end_time) {
    if (!report || !start_time || !end_time) {
        return;
    }

    // Preenche o tempo de início no formato HH:MM:SS.
    TimeInfos *start_time_infos = getTimeInfos(start_time);
    snprintf(report->start, STRING_DEFAULT_SIZE, "Inicio: %02d:%02d:%02d", start_time_infos->tm_hour, start_time_infos->tm_min, start_time_infos->tm_sec);

    // Preenche o tempo de fim no formato HH:MM:SS.
    TimeInfos *end_time_infos = getTimeInfos(end_time);
    snprintf(report->end, STRING_DEFAULT_SIZE, "Fim: %02d:%02d:%02d", end_time_infos->tm_hour, end_time_infos->tm_min, end_time_infos->tm_sec);

    // Preenche o tempo empregado.
    double elapsed_time = timeDifference(start_time, end_time);
    snprintf(report->duration, STRING_DEFAULT_SIZE, "Duração: %.2lf s", elapsed_time);
}

int manageProcesses(int shared_memory_id) {
    if (fork() == 0) {
        // Realiza o processamento do processo pi1.
        if (!yieldToChildProcess(shared_memory_id, PROCESS_1)) {
            return FALSE;
        }
    }
    else if (fork() == 0) {
        // Realiza o processamento do processo pi2.
        if (!yieldToChildProcess(shared_memory_id, PROCESS_2)) {
            return FALSE;
        }
    }
    else {
        // Realiza o processamento do processo pai.
        if (!yieldToFatherProcess(shared_memory_id)) {
            return FALSE;
        }
    }

    return TRUE;
}
