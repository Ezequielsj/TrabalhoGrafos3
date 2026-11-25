#ifndef MINHEAP_H
#define MINHEAP_H

typedef struct {
    int vertice;
    float distancia;
} HeapNode;

typedef struct {
    int capacidade;
    int qtd;
    HeapNode *dados;
    int *pos;
} MinHeap;

MinHeap* cria_MinHeap(int capacidade);
void libera_MinHeap(MinHeap* heap);
int heapVazia_MinHeap(MinHeap* heap);
void insere_MinHeap(MinHeap* heap, int vertice, float distancia);
HeapNode extrai_min_MinHeap(MinHeap* heap);
void diminuir_chave_MinHeap(MinHeap* heap, int vertice, float nova_distancia);
int estaNaHeap_MinHeap(MinHeap *heap, int vertice);

#endif
