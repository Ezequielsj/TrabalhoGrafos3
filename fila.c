#include "fila.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Fila *criar_fila(int capacidade_maxima) {
    Fila *fila = (Fila *)malloc(sizeof(Fila));
    if (!fila) {
        perror("Erro ao alocar memoria para a Fila");
        return NULL;
    }

    fila->capacidade = capacidade_maxima;
    fila->tamanho_atual = 0;
    fila->frente = 0;
    fila->tras = 0;

    fila->elementos = (int *)malloc((size_t)capacidade_maxima * sizeof(int));
    fila->em_fila = (bool *)calloc((size_t)capacidade_maxima, sizeof(bool));

    if (!fila->elementos || !fila->em_fila) {
        perror("Erro ao alocar memoria para os elementos/flags da Fila");
        free(fila->elementos);
        free(fila->em_fila);
        free(fila);
        return NULL;
    }

    return fila;
}

void destruir_fila(Fila *fila) {
    if (fila) {
        free(fila->elementos);
        free(fila->em_fila);
        free(fila);
    }
}

void limpar_fila(Fila *fila) {
    if (!fila) return;
    fila->tamanho_atual = 0;
    fila->frente = 0;
    fila->tras = 0;
    memset(fila->em_fila, 0, (size_t)fila->capacidade * sizeof(bool));
}

bool fila_vazia(const Fila *fila) {
    if (fila == NULL) return true;
    return fila->tamanho_atual == 0;
}

bool enfileirar(Fila *fila, int vertice) {
    if (fila == NULL || vertice < 0 || vertice >= fila->capacidade) {
        return false;
    }
    if (fila->em_fila[vertice]) {
        return true;
    }
    if (fila->tamanho_atual == fila->capacidade) {
        return false;
    }

    fila->elementos[fila->tras] = vertice;
    fila->tras = (fila->tras + 1) % fila->capacidade;
    fila->tamanho_atual++;
    fila->em_fila[vertice] = true;

    return true;
}

bool enfileirar_slf(Fila *fila, int vertice, const float *distancias) {
    if (fila == NULL || vertice < 0 || vertice >= fila->capacidade) {
        return false;
    }
    if (fila->em_fila[vertice]) {
        return true;
    }
    if (fila->tamanho_atual == fila->capacidade) {
        return false;
    }

    if (fila->tamanho_atual > 0 && distancias != NULL &&
        distancias[vertice] < distancias[fila->elementos[fila->frente]]) {
        fila->frente = (fila->frente - 1 + fila->capacidade) % fila->capacidade;
        fila->elementos[fila->frente] = vertice;
    } else {
        fila->elementos[fila->tras] = vertice;
        fila->tras = (fila->tras + 1) % fila->capacidade;
    }

    fila->tamanho_atual++;
    fila->em_fila[vertice] = true;
    return true;
}

int desenfileirar(Fila *fila) {
    if (fila_vazia(fila)) {
        return -1;
    }

    int vertice_removido = fila->elementos[fila->frente];
    fila->frente = (fila->frente + 1) % fila->capacidade;
    fila->tamanho_atual--;
    fila->em_fila[vertice_removido] = false;

    return vertice_removido;
}
