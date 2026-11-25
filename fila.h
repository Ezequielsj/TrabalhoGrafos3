#ifndef FILA_H
#define FILA_H

#include <stdbool.h>

typedef struct Fila {
    int capacidade;
    int tamanho_atual;
    int frente;
    int tras;
    int *elementos;
    bool *em_fila;
} Fila;

Fila *criar_fila(int capacidade_maxima);
void destruir_fila(Fila *fila);
void limpar_fila(Fila *fila);
bool fila_vazia(const Fila *fila);

/* Enfileiramento padrao FIFO com verificacao O(1) de pertinencia (em_fila) */
bool enfileirar(Fila *fila, int vertice);

/* Enfileiramento com heuristica SLF (Small Label First):
 * insere na frente da fila se distancias[vertice] < distancias[frente],
 * reduzindo reprocessamentos no Bellman-Ford (SPFA). */
bool enfileirar_slf(Fila *fila, int vertice, const float *distancias);

int desenfileirar(Fila *fila);

#endif
