#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

//#define DEBUG 1
#define PERC 1
#define ARRAY_SIZE 300032
#define GRAIN 4000

void insertion_sort(int *vector, int size) {
    int c,d,t;
    for (c = 1; c <= size - 1; c++) {
        d = c;

        while (d > 0 && vector[d - 1] > vector[d]) {
            t = vector[d];
            vector[d] = vector[d - 1];
            vector[d - 1] = t;

            d--;
        }
    }
}

int main(int argc, char **argv) {

    int vector_size;
    int aux_size;

    int *vector;
    int *status;

    int count = 0;

    MPI_Init(&argc , &argv);

    int my_rank;
    int proc_n;
    MPI_Status statusProc;;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);

    // Vetor dos status
    status = malloc(proc_n*sizeof(int));

    // Cada vetor tera uma quantidade igual dividida pelos processos
    // Usa-se a estrategia discutida de estender seu tamanho pelo grao das mensagens
    // Definimos tambem a POSICAO extra do buffer onde deve ser recebido o vetor parcial
    // para comparacao. Tambem e definida a posicao no vetor principal onde devem ser
    // guardados os valores da ordenacao parcial (ou seja, sera grao*2, pois tera os novos
    // valores como os valores do vetor atual, logo, devem ser guardados antes)

    // Adotada a estrategia discutida em aula (estender o buffer do vetor atual pelo grao)
    // partial_to_transit = ponteiro que aponta para a parte do vetor onde sera recebido
    // os novos valores, assim como a parte que sera enviada de volta a apos ordencao

    // partial_to_sort = ponteiro que aponta para a parte do vetor que fara o corte
    // para mandar os valores do vetor atual como os recebidos para a ordenacao parcial
    // (a qual tera tamanho grao*2)
    vector_size = ARRAY_SIZE/proc_n;
    aux_size = vector_size+GRAIN;
    vector = malloc(aux_size*sizeof(int));
    int *partial_to_transit = &vector[vector_size];
    int *partial_to_sort = &vector[vector_size - GRAIN];

    // Preenche o vetor em ordem decrescente (pior caso)
    int j;
    int first = (proc_n - my_rank) * vector_size;
    for(j=0; j<vector_size; j++) {
        vector[j] = first-j;
    }

    // Printa inicialmente o vetor
    #ifdef DEBUG
    int w;
    for(w=0; w<vector_size; w++) {
        printf("Vetor %d contem:\n", my_rank);
        printf("%d, ", vector[w]);
    }
    printf("\n");
    #endif

    // Estamos adotando a estrategia de passar as mensagens da direita para a esquerda
    // e os vetores parciais da esquerda para a direita, afim de facilitar alocacao
    // e nao precisar criar mais vetores auxiliares desnecessarios
    // Portanto, o processo final SEMPRE estara pronto. Se existir um numero
    // que pertence a ele, o seu nodo da esquerda que nao estara pronto
    status[proc_n-1] = 1;

    int done = 0;

    //Variavel auxiliar que guarda o elemento a comparar (primeira troca)
    int comp_n;

    double start = MPI_Wtime();

    int i;
    while(done == 0) {
        // PARTE 1: Ordena total e manda um so valor para fazer verificacao (alteracao do status)

        count++; // Iteracao sera contada para fins de analise

        // ALGORITMO
        insertion_sort(vector, vector_size);

        // Se nao sou o lider, mando meu menor valor para a esquerda
        if(my_rank!=0) MPI_Send(vector, 1, MPI_INT, my_rank-1, 1, MPI_COMM_WORLD);

        // Se nao sou o ultimo processo, recebo o valor e comparo com meu maior
        if(my_rank!=proc_n-1) {
            MPI_Recv(&comp_n, GRAIN, MPI_INT, my_rank+1, 1, MPI_COMM_WORLD, &statusProc);

            // Verifica se ele esta pronto ou nao
            if(vector[vector_size-1] > comp_n) status[my_rank] = 0;
            else status[my_rank] = 1;
        }

        // PARTE 2: Broadcast o seu status
        // Da Broadcast do seu estado para os outros nucleos
        done = 1;
        for(i=0; i<proc_n; i++) {
            MPI_Bcast(&status[i], 1, MPI_INT, i, MPI_COMM_WORLD);

            //Se alguem nao estiver com 1, significa que nao esta pronto
            if(status[i]!=1) {
                done = 0;
                break;
            }
        }

        // PARTE 3: Ordenacao Parcial
        // Se nao estiverem prontos, cai na parte de fazer a ordenacao parcial
        if(done == 0){
            // Todos tirado o primeiro processo devem mandar seus menores valores ([0...grao-1])
            // para o processo da direita
            if(my_rank!=0) MPI_Send(vector, GRAIN, MPI_INT, my_rank-1, 1, MPI_COMM_WORLD);

            // Todos exceto o ultimo processo devem receber esta parte do vetor
            // e inserir na parte extra do buffer
            // Agora ele precisa pegar a outra parte parcial do vetor atual (os valores finais)
            // justificando a escolha da troca da direita para a esquerda (mais facil de juntar os parciais)
            // ordenar. Deve depois mandar os valores parciais do vetor ordenado, que ja estarao demarcados
            // pelo proprio ponteiro
            if(my_rank!=proc_n-1) {
                MPI_Recv(partial_to_transit, GRAIN, MPI_INT, my_rank+1, 1, MPI_COMM_WORLD, &statusProc);
                insertion_sort(partial_to_sort, GRAIN*2);
                MPI_Send(partial_to_transit, GRAIN, MPI_INT, my_rank+1, 1, MPI_COMM_WORLD);
            }

            // Como que o processo final so enviou, ele precisa receber aqui
            // Se botar la em cima, o programa quebra
            if(my_rank!=0) MPI_Recv(vector, GRAIN, MPI_INT, my_rank-1, 1, MPI_COMM_WORLD, &statusProc);
        }
    }

    // Imprime o tempo
    double end = MPI_Wtime();
    if(my_rank==0) printf("\nTempo: %.2f segundos\n",end-start);

    // Imprime o vetor ordenado
    #ifdef DEBUG
    int k;
    for(k=0; k<vector_size; k++) {
        printf("Vetor %d contem:\n", my_rank);
        printf("%d, ", vector[k]);
    }
    printf("\n");
    #endif

    // Para fins pedidos em aula
    // aqui imprime quantos % dos valores dos vetores foram trocados entre
    // os processos executando
    #ifdef PERC
    if(my_rank == 0) printf("Iteracoes: %d\nPercentual de troca: %.2f%%\n",count,(GRAIN/((double)vector_size))*100.0);
    #endif

    MPI_Finalize();
    return 0;
}