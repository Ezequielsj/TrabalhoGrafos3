#include <stdio.h>
#include <stdlib.h>
#include "minheap.h"

static void trocarNos(MinHeap* heap, int i, int j) {
    HeapNode temp = heap->dados[i];
    heap->pos[heap->dados[i].vertice] = j;
    heap->pos[heap->dados[j].vertice] = i;
    heap->dados[i] = heap->dados[j];
    heap->dados[j] = temp;
}

static void promover(MinHeap* heap, int indice) {
    int pai = (indice - 1) / 2;
    while (indice > 0 && heap->dados[indice].distancia < heap->dados[pai].distancia) {
        trocarNos(heap, indice, pai);
        indice = pai;
        pai = (indice - 1) / 2;
    }
}

static void rebaixar(MinHeap* heap, int pai) {
    int filho = 2 * pai + 1;
    while (filho < heap->qtd) {
        if (filho + 1 < heap->qtd && heap->dados[filho + 1].distancia < heap->dados[filho].distancia) {
            filho = filho + 1;
        }
        if (heap->dados[pai].distancia <= heap->dados[filho].distancia) {
            break;
        }
        trocarNos(heap, pai, filho);
        pai = filho;
        filho = 2 * pai + 1;
    }
}

MinHeap* cria_MinHeap(int capacidade) {
    MinHeap *heap = (MinHeap*) malloc(sizeof(MinHeap));
    if (heap == NULL) return NULL;

    heap->pos = (int*) malloc((size_t)capacidade * sizeof(int));
    if (heap->pos == NULL) {
        free(heap);
        return NULL;
    }

    heap->dados = (HeapNode*) malloc((size_t)capacidade * sizeof(HeapNode));
    if (heap->dados == NULL) {
        free(heap->pos);
        free(heap);
        return NULL;
    }

    heap->capacidade = capacidade;
    heap->qtd = 0;

    for (int i = 0; i < capacidade; i++) {
        heap->pos[i] = -1;
    }

    return heap;
}

void libera_MinHeap(MinHeap* heap) {
    if (heap == NULL) return;
    free(heap->dados);
    free(heap->pos);
    free(heap);
}

int heapVazia_MinHeap(MinHeap* heap) {
    return (heap->qtd == 0);
}

void insere_MinHeap(MinHeap* heap, int vertice, float distancia) {
    if (heap->qtd == heap->capacidade) {
        fprintf(stderr, "Erro: Tentativa de inserir em heap cheia.\n");
        return;
    }

    int indice = heap->qtd;
    heap->dados[indice].vertice = vertice;
    heap->dados[indice].distancia = distancia;
    heap->pos[vertice] = indice;
    heap->qtd++;
    promover(heap, indice);
}

HeapNode extrai_min_MinHeap(MinHeap* heap) {
    if (heapVazia_MinHeap(heap)) {
        HeapNode no_invalido = {-1, -1.0f};
        return no_invalido;
    }

    HeapNode no_min = heap->dados[0];
    HeapNode ultimo_no = heap->dados[heap->qtd - 1];

    heap->pos[no_min.vertice] = -1;
    heap->qtd--;
    /* Com 1 unico elemento, ultimo_no == no_min: nao ha o que reposicionar (antes, pos[v] voltava
     * a 0 e estaNaHeap() passava a retornar verdadeiro para um vertice ja extraido). */
    if (heap->qtd > 0) {
        heap->dados[0] = ultimo_no;
        heap->pos[ultimo_no.vertice] = 0;
        rebaixar(heap, 0);
    }
    return no_min;
}

void diminuir_chave_MinHeap(MinHeap* heap, int vertice, float nova_distancia) {
    int indice = heap->pos[vertice];
    if (indice == -1) return;
    if (nova_distancia >= heap->dados[indice].distancia) return;

    heap->dados[indice].distancia = nova_distancia;
    promover(heap, indice);
}

int estaNaHeap_MinHeap(MinHeap *heap, int vertice) {
    return (heap->pos[vertice] != -1);
}
