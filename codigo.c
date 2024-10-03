#include <stdio.h> // Biblioteca padrão para funções de entrada e saída
#include <pthread.h> // Biblioteca para manipulação de threads
#include <semaphore.h> // Biblioteca para semáforos
#include <unistd.h> // Biblioteca para a função sleep 

#define NUM_PEDIDOS 5
#define NUM_COZINHEIROS 2

int pedidos[NUM_PEDIDOS]; // Armazena os pedidos feitos pelos clientes
int pedidos_status[NUM_PEDIDOS]; // Armazena os status dos pedidos (0 - não finalizado, 1 - finalizado)
int pedidos_disponiveis = 0; // Indica quantos pedidos estão disponíveis para serem processados pelos cozinheiros
int proximo_pedido_a_ser_processado = 0; // Aponta para o próximo pedido que será processado
int proximo_pedido_a_ser_feito = 0; // Aponta para a posição no buffer do próximo pedido que será feito

sem_t espacos_disponiveis; // Semáforo para controlar o número de espaços disponíveis no buffer de pedidos
sem_t pedido_disp; // Semáforo para controlar a disponibilidade de pedidos para os cozinheiros
pthread_mutex_t mutex; // Mutex para evitar condições de corrida

void* cliente(void* arg){
    intptr_t cliente_id = (intptr_t)arg; // Converte o argumento em ID do cliente

    sem_wait(&espacos_disponiveis); // Espera até que haja espaço disponível no buffer para fazer o pedido

    pthread_mutex_lock(&mutex); // Trava o mutex para garantir que apenas um cliente por vez possa acessar o buffer
    
    int indice = proximo_pedido_a_ser_feito % NUM_PEDIDOS; // Posição do pedido no buffer
    
    pedidos[indice] = cliente_id; // Armazena o ID do cliente no buffer de pedidos
    pedidos_status[indice] = 0;  // Marca o status do pedido como "não finalizado"
    
    printf("O cliente %ld fez o pedido %d\n", cliente_id, pedidos[indice]);

    // Atualiza o contador de pedidos disponíveis e o índice do próximo pedido, assim...
    pedidos_disponiveis++; // ... indica que tem mais um pedido disponível para ser processado
    proximo_pedido_a_ser_feito++; // ... indica qual o próximo pedido a ser feito
    
    pthread_mutex_unlock(&mutex); // Destrava o mutex para que outros clientes/cozinheiros possam acessar o buffer
    
    sem_post(&pedido_disp); // Notifica que há um pedido disponível, permitindo que um cozinheiro o processe
    
    while (pedidos_status[indice] == 0) sleep(2); // Aguarda até que o status do pedido seja alterado para "finalizado" (1)

    printf("Cliente %ld recebeu o pedido %d.\n", cliente_id, pedidos[indice]);
    
    return NULL; // Termina a execução da thread cliente
}

void* cozinheiro(void* arg){
    intptr_t cozinheiro_id = (intptr_t)arg; // Converte o argumento em ID do cozinheiro

    while (1){
        sem_wait(&pedido_disp); // Espera até que haja um pedido disponível
        
        pthread_mutex_lock(&mutex); // Trava o mutex para garantir que o buffer seja acessado de forma segura

        if (pedidos_disponiveis > 0){ // Se houver pedidos disponíveis para processar:
            int pedido_id = pedidos[proximo_pedido_a_ser_processado % NUM_PEDIDOS]; // Encontra o próximo pedido a ser processado
            
            printf("O cozinheiro %ld está preparando o pedido %d\n", cozinheiro_id, pedido_id);

            sleep(3); // Simula o tempo necessário para preparar o pedido

            printf("O cozinheiro %ld terminou o pedido %d\n", cozinheiro_id, pedido_id);
            
            pedidos_status[proximo_pedido_a_ser_processado % NUM_PEDIDOS] = 1; // Marca o pedido como "finalizado"

            proximo_pedido_a_ser_processado++; // Atualiza o índice do próximo pedido a ser processado
            pedidos_disponiveis--; // Decrementa o número de pedidos disponíveis
        }
        
        pthread_mutex_unlock(&mutex); //Libera o mutex para que outros cozinheiros/clientes possam acessar o buffer

        sem_post(&espacos_disponiveis); // Libera um espaço no buffer, permitindo que novos pedidos sejam feitos
    }
    return NULL; // Termina a execução da thread cozinheiro
}

int main(){
    pthread_t thread_clientes[NUM_PEDIDOS], thread_cozinheiros[NUM_COZINHEIROS];

    sem_init(&espacos_disponiveis, 0, NUM_PEDIDOS); // Inicializa o semáforo de espaços no buffer com o valor NUM_PEDIDOS
    sem_init(&pedido_disp, 0, 0); // Inicializa o semáforo de pedidos disponíveis com 0 (nenhum pedido disponível no início)
    pthread_mutex_init(&mutex, NULL); // Inicializa o mutex para sincronizar o acesso ao buffer

    // Criação das threads de clientes
    for (int i = 0; i < NUM_PEDIDOS; i++){
        pthread_create(&thread_clientes[i], NULL, cliente, (void*)(intptr_t)i); // Cria uma thread para cada cliente
    }

    // Criação das threads de cozinheiros
    for (int i = 0; i < NUM_COZINHEIROS; i++){
        pthread_create(&thread_cozinheiros[i], NULL, cozinheiro, (void*)(intptr_t)(i + 1)); // Cria uma thread para cada cozinheiro
    }

    // Aguarda que todas as threads de clientes terminem
    for (int i = 0; i < NUM_PEDIDOS; i++){
        pthread_join(thread_clientes[i], NULL);
    }

    // Destruindo o mutex e os semáforos
    sem_destroy(&espacos_disponiveis);
    sem_destroy(&pedido_disp);
    pthread_mutex_destroy(&mutex);

    printf("Todos os pedidos foram processados\n");

    return 0;
}
