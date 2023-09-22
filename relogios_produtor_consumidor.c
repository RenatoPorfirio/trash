#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <mpi.h>

#define PRODUCER_QNT 3
#define CONSUMER_QNT 3
#define BUFFER_SIZE 9

// Como compilar:
// mpicc -o relogios_produtor_consumidor relogios_produtor_consumidor.c -lpthread
// mpiexec -n 3 ./relogios_produtor_consumidor

typedef struct Clock {
    int p[3];
} Clock;

typedef struct Task {
    Clock clk;
    int src_pid;
    int dest_pid;
} Task;

Task taskQueue[BUFFER_SIZE];
int taskCount = 0;

pthread_mutex_t mutex;

pthread_cond_t condFull;
pthread_cond_t condEmpty;

void executeProducerTask(Task* task, int id) {
    // Esta funcao representa a acao do produtor (P0, P1, P2) enviando mensagens
    printf("Produtor %d - Clock: (%d, %d, %d) - Enviando mensagem de P%d para P%d\n", id, task->clk.p[0], task->clk.p[1], task->clk.p[2], task->src_pid, task->dest_pid);
}

void executeConsumerTask(Task* task, int id) {
    // Esta funcao representa a acao do consumidor (P0, P1, P2) recebendo mensagens
    printf("Consumidor %d - Clock: (%d, %d, %d) - Recebendo mensagem de P%d para P%d\n", id, task->clk.p[0], task->clk.p[1], task->clk.p[2], task->src_pid, task->dest_pid);
}

Task getTask(int id) {
    pthread_mutex_lock(&mutex);

    while (taskCount == 0) {
        printf("Consumidor %d - A fila ta vazia.\n", id);
        pthread_cond_wait(&condEmpty, &mutex);
    }

    Task task = taskQueue[0];
    int i;
    for (i = 0; i < taskCount - 1; i++) {
        taskQueue[i] = taskQueue[i + 1];
    }
    taskCount--;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condFull);
    return task;
}

void submitTask(Task task, int id) {
    pthread_mutex_lock(&mutex);

    while (taskCount == BUFFER_SIZE) {
        printf("Produtor %d - FILA CHEIA!\n", id);
        pthread_cond_wait(&condFull, &mutex);
    }

    taskQueue[taskCount] = task;
    taskCount++;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condEmpty);
}

void* startProducerThread(void* args) {
    int* new_args = (int*)args;

    // Defina as acoes de envio de mensagens conforme especificado
    if (new_args[0] == 0) {
        // P0: b envia para i
        Task task;
        task.clk.p[0] = 2;
        task.clk.p[1] = 0;
        task.clk.p[2] = 0;
        task.src_pid = 1;
        task.dest_pid = 0;
        submitTask(task, new_args[0]);
        executeProducerTask(&task, new_args[0]);
    }
    else if (new_args[0] == 1) {
        // P1: h envia para c
        Task task;
        task.clk.p[0] = 3;
        task.clk.p[1] = 1;
        task.clk.p[2] = 0;
        task.src_pid = 0;
        task.dest_pid = 2;
        submitTask(task, new_args[0]);
        executeProducerTask(&task, new_args[0]);

        // P1: d envia para m
        Task task2;
        task2.clk.p[0] = 4;
        task2.clk.p[1] = 1;
        task2.clk.p[2] = 0;
        task2.src_pid = 0;
        task2.dest_pid = 2;
        submitTask(task2, new_args[0]);
        executeProducerTask(&task2, new_args[0]);

        // P1: l envia para e
        Task task3;
        task3.clk.p[0] = 5;
        task3.clk.p[1] = 1;
        task3.clk.p[2] = 2;
        task3.src_pid = 2;
        task3.dest_pid = 0;
        submitTask(task3, new_args[0]);
        executeProducerTask(&task3, new_args[0]);

        // P1: f envia para j
        Task task4;
        task4.clk.p[0] = 6;
        task4.clk.p[1] = 1;
        task4.clk.p[2] = 2;
        task4.src_pid = 2;
        task4.dest_pid = 0;
        submitTask(task4, new_args[0]);
        executeProducerTask(&task4, new_args[0]);
    }
    // Implemente outras acoes de envio de mensagens para os demais produtores aqui

    return NULL;
}

void* startConsumerThread(void* args) {
    int* new_args = (int*)args;
    for (int i = 0; i < 15; i++) {
        Task task = getTask(new_args[0]);
        executeConsumerTask(&task, new_args[0]);
        usleep(new_args[1] * 1000);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condEmpty, NULL);
    pthread_cond_init(&condFull, NULL);

    pthread_t producer_thread[PRODUCER_QNT];
    pthread_t consumer_thread[CONSUMER_QNT];

    int producer_args[PRODUCER_QNT][2];
    int consumer_args[CONSUMER_QNT][2];

    size_t i;

    srand(time(NULL));

    for (i = 0; i < PRODUCER_QNT; i++) {
        producer_args[i][0] = i;
        producer_args[i][1] = 10;
        if (pthread_create(&producer_thread[i], NULL, &startProducerThread, (void*)producer_args[i]) != 0) {
            perror("Failed to create the thread");
        }
    }

    for (i = 0; i < CONSUMER_QNT; i++) {
        consumer_args[i][0] = i;
        consumer_args[i][1] = 60;
        if (pthread_create(&consumer_thread[i], NULL, &startConsumerThread, (void*)consumer_args[i]) != 0) {
            perror("Failed to create the thread");
        }
    }

    for (i = 0; i < PRODUCER_QNT; i++) {
        if (pthread_join(producer_thread[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

    for (i = 0; i < CONSUMER_QNT; i++) {
        if (pthread_join(consumer_thread[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condEmpty);
    pthread_cond_destroy(&condFull);
    return 0;
}
