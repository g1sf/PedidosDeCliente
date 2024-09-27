#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_PEDIDOS 5
#define NUM_COZINHEIROS 2

int pedidos[NUM_PEDIDOS]; // Armazena os pedidos
int pedidos_status[NUM_PEDIDOS]; // Armazena os status dos pedidos (0 - não finalizado, 1 - finalizado)
int pedidos_disponiveis = 0; // Indica quantos pedidos estão disponíveis para serem prcoessados
int proximo_pedido_a_ser_processado = 0; // Indica o próximo pedido a ser processado
int proximo_pedido_a_ser_feito = 0; // Indica o próximo pedido a ser feito

sem_t espacos_disponiveis; // Semáforo para controlar espaço disponível no buffer
sem_t pedido_disp; // Semáforo para controlar a disponibilidade de pedidos
pthread_mutex_t mutex; // Mutex para sincronização

void* cliente(void* arg){
    intptr_t cliente_id = (intptr_t)arg; // ID do cliente

    sem_wait(&espacos_disponiveis); // Espera ter espaço no buffer

    pthread_mutex_lock(&mutex); // Mutex de sincronização indica que há uma thread de cliente já rodando
    
    int indice = proximo_pedido_a_ser_feito % NUM_PEDIDOS; // Posição do pedido no buffer
    // Variável indice criada para poder ser utilizada após o mutex ser liberado
    // Se utilizar apenas a variável proximo_pedido_a_ser_feito, pode haver sobrescrição de dados
    
    pedidos[indice] = cliente_id; // Armazena o pedido do cliente
    pedidos_status[indice] = 0; // Status do pedido como não finalizado
    
    printf("O cliente %ld fez o pedido %d\n", cliente_id, pedidos[indice]);
    
    pedidos_disponiveis = (pedidos_disponiveis + 1) % NUM_PEDIDOS; // Indica que tem mais um pedido disponível para ser processado
    proximo_pedido_a_ser_feito = (proximo_pedido_a_ser_feito + 1) % NUM_PEDIDOS; // Indica qual o próximo pedido a ser feito
    
    pthread_mutex_unlock(&mutex); // Mutex de sincronização indica que outras threads podem rodar, não há risco de sobrescrição de dados

    sem_post(&pedido_disp); // Notifica um cozinheiro que tem um pedido disponível

    while (pedidos_status[indice] == 0) sleep(2); // Aguardando o pedido ser finalizado

    printf("Cliente %ld recebeu o pedido %d.\n", cliente_id, pedidos[indice]);
    
    return NULL;
}

void* cozinheiro(void* arg){
    intptr_t cozinheiro_id = (intptr_t)arg; // ID do cozinheiro

    while (1){
        sem_wait(&pedido_disp); // Espera até um pedido estar disponível

        pthread_mutex_lock(&mutex); // Mutex de sincronização indica que há uma thread de cozinheiro já rodando

        if (pedidos_disponiveis > 0){ // Se tiver pedidos disponíveis, o cozinheiro irá processar um pedido
            int pedido_id = pedidos[proximo_pedido_a_ser_processado % NUM_PEDIDOS]; // Encontra o pedido a ser processado
            
            printf("O cozinheiro %ld está preparando o pedido %d\n", cozinheiro_id, pedido_id);

            sleep(3); // Simula tempo de preparação do pedido

            printf("O cozinheiro %ld terminou o pedido %d\n", cozinheiro_id, pedido_id);
            
            pedidos_status[pedido_id] = 1; // Indica que o pedido foi finalizado

            proximo_pedido_a_ser_processado = (proximo_pedido_a_ser_processado + 1) % NUM_PEDIDOS; // Indica qual o próximo pedido a ser processado
            pedidos_disponiveis = (pedidos_disponiveis - 1) % NUM_PEDIDOS; // Indica que um pedido já foi processado
        }
        pthread_mutex_unlock(&mutex); // Mutex de sincronização indica que outras threads podem rodar, não há risco de sobrescrição de dados
        sem_post(&espacos_disponiveis); // Libera espaço no buffer para novos pedidos
    }
    return NULL;
}

int main(){
    pthread_t thread_clientes[NUM_PEDIDOS], thread_cozinheiros[NUM_COZINHEIROS];

    sem_init(&espacos_disponiveis, 0, NUM_PEDIDOS); // Buffer inicialmente com espaço total
    sem_init(&pedido_disp, 0, 0); // Inicialmente sem pedidos disponíveis
    pthread_mutex_init(&mutex, NULL); // Inicializa o mutex para sincronização

    // Criando threads de clientes
    for (int i = 0; i < NUM_PEDIDOS; i++){
        pthread_create(&thread_clientes[i], NULL, cliente, (void*)(intptr_t)i);
    }

    // Criando threads de cozinheiros
    for (int i = 0; i < NUM_COZINHEIROS; i++){
        pthread_create(&thread_cozinheiros[i], NULL, cozinheiro, (void*)(intptr_t)(i + 1));
    }

    // Aguardando todas as threads de clientes terminarem
    for (int i = 0; i < NUM_PEDIDOS; i++){
        pthread_join(thread_clientes[i], NULL);
    }

    // Como os cozinheiros são loops infinitos, não há necessidade de join

    // Destruindo mutex e semáforos
    sem_destroy(&espacos_disponiveis);
    sem_destroy(&pedido_disp);
    pthread_mutex_destroy(&mutex);

    printf("Todos os pedidos foram processados\n");

    return 0;
}
