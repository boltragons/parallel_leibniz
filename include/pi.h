#pragma once

#include <pthread.h> // Requerido pela API Pthreads (POSIX Threads).

// Constantes lógicas.
#define TRUE 1
#define FALSE 0

// Tamanho do nome do arquivo.
#define FILE_NAME_SIZE 10

// Número de threads do processo filho.
#define NUMBER_OF_THREADS 16

// Tamanho padrão de string.
#define STRING_DEFAULT_SIZE 128

// Número de casas decimais do número pi.
#define DECIMAL_PLACES 9

// Número máximo de termos da série de Leibniz.
#define MAXIMUM_NUMBER_OF_TERMS 2000000000

// Número parcial de termos da série de Leibniz.
#define PARTIAL_NUMBER_OF_TERMS 125000000 

// Define uma string de tamanho padrão T, onde T é igual STRING_DEFAULT_SIZE.
typedef char String[STRING_DEFAULT_SIZE];

// Nome do arquivo.
typedef char FileName[FILE_NAME_SIZE];

// Estrutura do relatório a ser gerado pelo processo.
typedef struct {
   // Os comentários abaixo são apenas exemplos de valores a serem armazenados nos campos desta estrutura.
   String 
      identification, // Processo Filho: pi1 (PID 6924)
      numberOfThreads, // Nº de threads: 16
      start, // Início: 10:45:12
      end, // Fim: 10:45:21
      duration, // Duração: 9,59 s
      pi; // Pi = 3,141592653   
} ProcessReport;

// Estrutura do relatório a ser gerado pelo programa.
typedef struct {
   // Os comentários abaixo são apenas exemplos de valores a serem armazenados nos campos desta estrutura.
   String 
      programName, // Cálculo do Número π
      message1, // Criando os processos filhos pi1 e pi2...
      message2; // Processo pai (PID 6923) finalizou sua execução.
      
   ProcessReport processReport1, processReport2;     
} Report;

// Representa a identificação da thread e o seu tempo de execução em segundos.
typedef struct  {
   pthread_t threadID; // Identificação da thread obtida com pthread_create.
   pid_t tid;          // Identificação da thread obtida com gettid.
   double time; 
} Thread;

// Relação de threads do processo filho.
typedef Thread Threads[NUMBER_OF_THREADS]; 

/* Cria o relatório do programa escrevendo na tela as informações da estrutura Report.
 * Retorna TRUE se o relatório foi escrito com sucesso ou FALSE se os dados da estrutura Report são vazios ou nulos.
*/
int createReport(const Report *report);

/* Cria o arquivo texto no diretório atual usando o nome do arquivo, a descrição e os dados do vetor do tipo Threads.
 * Retorna TRUE se o arquivo foi criado com sucesso ou FALSE se ocorreu algum erro.
 */
int createFile(const FileName fileName, String description, const Threads threads);

/* Cria uma thread para fazer a soma parcial de n termos da série de Leibniz. 
   Esta função deve usar a função sumPartial para definir qual a função a ser executada por cada uma das x threads 
   do programa, onde x é igual a NUMBER_OF_THREADS. 
   Retorna a identificação da thread.
*/
pthread_t createThread(unsigned int *terms); 

/* Realiza a soma parcial de n (n é definido por PARTIAL_NUMBER_OF_TERMS) termos da série de Leibniz
   começando em x, por exemplo, como PARTIAL_NUMBER_OF_TERMS é 125.000.000, então se x é:

             0 -> calcula a soma parcial de 0 até 124.999.999;
   125.000.000 -> calcula a soma parcial de 125.000.000 até 249.999.999;
   250.000.000 -> calcula a soma parcial de 250.000.000 até 374.999.999; 
       
   e assim por diante. 

   O resultado dessa soma parcial deve ser um valor do tipo double a ser retornado 
   por esta função para o processo que criou a thread.
*/
void* sumPartial(void *terms);

/* Calcula o número pi com n (n é definido por DECIMAL_PLACES) casas decimais usando o número máximo de
   termos da série de Leibniz, que é definido por MAXIMUM_NUMBER_OF_TERMS. Esta função deve criar x threads
   usando a função createThread, onde x é igual a NUMBER_OF_THREADS. 
*/
double calculationOfNumberPi(unsigned int terms);

/*
 * Esta função inicia o programa.
 * Retorna EXIT_SUCCESS.
 */
int pi();

// ====================
// Adições:
// ====================

#define _GNU_SOURCE
#include <unistd.h>

pid_t gettid(void);

#include <stdio.h>

/*
 * Contém os argumentos passados a cada thread pelos
 * processos filhos.
 */
typedef struct {
   unsigned int first_term;   // Termo inicial da série 
   double *pi_approximation;  // Acumulador do resultado dos termos da série, nos processos filho
   pthread_mutex_t *pi_mutex; // Mutex binário para acessar 'pi_approximation'
   Thread *thread_infos;      // Dados da Thread em questão
} ThreadArgs; 

// Enumeraçãp dos processos empregados. 
typedef enum {
   PROCESS_1 = 1,
   PROCESS_2
} ProcessNumber;

// Mantém os tempos usados no benchmark.
typedef struct timeval Time;

// Mantém os elementos de tempo e.g. data, hora.
typedef struct tm TimeInfos;

// Permissões gerais do Linux para ler e escrever
#define READ_WRITE_PERMISSIONS 0666

// Arquivo usado para gerar a chave de acesso à memória compartilhada entre processos.
#define SHARED_MEMORY_KEY_PATH "./shared_memory.tmp"

/**
 * Cria uma área de memória a ser compartilhada entre processos
 * e retorna o seu ID.
*/
int createSharedMemory(void);

/**
 * Obtém o endereço da área de memória compartilhada, identificada
 * pelo ID.
*/
Report *getSharedMemory(int shared_memory_id);

/**
 * Realiza o processamento do processo filho.
*/
int yieldToChildProcess(int shared_memory_id, ProcessNumber process_number);

/**
 * Realiza o processamento do processo pai.
*/
int yieldToFatherProcess(int shared_memory_id);

/*
 * Verifica se as strings do relatório são válidas (não nulas
 * e não vazias).
 */
int validateReport(const Report *report);

/*
 * Verifica se as n strings fornecidas são válidas (não nulas
 * e não vazias).
 */
int validateStrings(const char **strings, size_t n);

/*
 * Preenche as informações relativas a tempo das threads do arquivo
 * de cada processo.
 */
void fillThreadsTimes(FILE *file, const Threads threads);

/*
 * Realiza o cálculo do Pi usando a formula de Leibniz, calculando
 * PARTIAL_NUMBER_OF_TERMS termos parciais a partir de first_therm.
 */
double partialLeibnizFormula(int first_therm);

/*
 * Calcula o valor do pi, obtendo os marcos de tempo antes e depois 
 * do cálculo, de forma a preenchê-los no relatório, bem cimo a sua
 * diferença.
 */
void performCalculation(ProcessReport *report, ProcessNumber process);

/*
 * Cria 16 threads para aproximarem o Pi, esperando que
 * elas terminem, e retornando o valor do pi. São mandados
 * argumentos para as threads (ThreadArgs), de forma que elas
 * preencham informações importantes, como o TID e acumulem o
 * pi.
 */
double createPiThreads(Threads threads_infos);

/*
 * Retorna a diferença entre dois tempos, em segundos, com precisão de
 * de microsegundos.
 */
double timeDifference(const Time *start_time, const Time *end_time);

/*
 * Obtém o tempo atual.
 */
void getTime(Time *time);

/*
 * Obtem informações adicionais referente ao tempo (hora, minuto, segundo). 
 */
TimeInfos *getTimeInfos(const Time *time);

/*
 * Preenche informações do relatório referentes ao tempo: início, fim e 
 * tempo empregado.
 */
void fillTimeReport(ProcessReport *report, const Time *start_time, const Time *end_time);

/*
 * Cria os processos filhos, que por sua vez irão criar 16 threads e calcular
 * o número Pi, usando performCalculation(), e espera que os processos filhos
 * terminem a execução.
 */
int manageProcesses(int shared_memory_id);

/**
 * Pilha de chamadas até chegar ao processamento do pi:
 *  
 * main -> pi -> manageProcesses -> performCalculation -> calculationOfNumberPi -> createPiThreads -> createThread -> sumPartial
*/
