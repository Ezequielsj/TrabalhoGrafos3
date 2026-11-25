#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include "biblioteca_grafos.h"
#include "minheap.h"
#include "fila.h"

#define ARENA_BLOCO_CAPACIDADE 65536

typedef struct {
    int tamanho;
    int *vertices;
} ComponenteConexa;

static int comparar_componentes(const void *a, const void *b) {
    const ComponenteConexa *compA = (const ComponenteConexa *)a;
    const ComponenteConexa *compB = (const ComponenteConexa *)b;
    return compB->tamanho - compA->tamanho;
}

static NoListaAdj *alocar_no_arena(Grafo *grafo) {
    BlocoArenaNos *bloco = grafo->arena_nos;
    if (!bloco || bloco->usado >= bloco->capacidade) {
        BlocoArenaNos *novo_bloco = (BlocoArenaNos *)malloc(sizeof(BlocoArenaNos));
        if (!novo_bloco) return NULL;
        novo_bloco->dados = (NoListaAdj *)malloc((size_t)ARENA_BLOCO_CAPACIDADE * sizeof(NoListaAdj));
        if (!novo_bloco->dados) {
            free(novo_bloco);
            return NULL;
        }
        novo_bloco->usado = 0;
        novo_bloco->capacidade = ARENA_BLOCO_CAPACIDADE;
        novo_bloco->proximo = grafo->arena_nos;
        grafo->arena_nos = novo_bloco;
        bloco = novo_bloco;
    }
    return &bloco->dados[bloco->usado++];
}

Grafo *criar_grafo_lista_adj(int num_vertices) {
    Grafo *grafo = (Grafo *)malloc(sizeof(Grafo));
    if (!grafo) {
        printf("Erro ao alocar memoria para o grafo.\n");
        return NULL;
    }
    grafo->num_vertices = num_vertices;
    grafo->num_arestas = 0;
    grafo->lista_adj = (NoListaAdj **)calloc((size_t)num_vertices, sizeof(NoListaAdj *));
    grafo->graus = (int *)calloc((size_t)num_vertices, sizeof(int));
    grafo->matriz_adj = NULL;
    grafo->tipo_representacao = LISTA_ADJACENCIA;
    grafo->arena_nos = NULL;

    if (!grafo->lista_adj || !grafo->graus) {
        printf("Erro ao alocar memoria para lista de adjacencia ou graus.\n");
        free(grafo->lista_adj);
        free(grafo->graus);
        free(grafo);
        return NULL;
    }
    return grafo;
}

Grafo *criar_grafo_matriz_adj(int num_vertices) {
    Grafo *grafo = (Grafo *)malloc(sizeof(Grafo));
    if (!grafo) {
        printf("Erro ao alocar memoria para o grafo.\n");
        return NULL;
    }
    grafo->num_vertices = num_vertices;
    grafo->num_arestas = 0;
    grafo->lista_adj = NULL;
    grafo->graus = (int *)calloc((size_t)num_vertices, sizeof(int));
    grafo->matriz_adj = (float **)malloc((size_t)num_vertices * sizeof(float *));
    grafo->tipo_representacao = MATRIZ_ADJACENCIA;
    grafo->arena_nos = NULL;

    if (!grafo->matriz_adj || !grafo->graus) {
        printf("Erro ao alocar memoria para matriz de adjacencia ou graus.\n");
        free(grafo->matriz_adj);
        free(grafo->graus);
        free(grafo);
        return NULL;
    }
    for (int i = 0; i < num_vertices; i++) {
        grafo->matriz_adj[i] = (float *)malloc((size_t)num_vertices * sizeof(float));
        if (!grafo->matriz_adj[i]) {
            printf("Erro ao alocar memoria para linha da matriz de adjacencia.\n");
            for (int j = 0; j < i; j++) {
                free(grafo->matriz_adj[j]);
            }
            free(grafo->matriz_adj);
            free(grafo->graus);
            free(grafo);
            return NULL;
        }
        for (int j = 0; j < num_vertices; j++) {
            grafo->matriz_adj[i][j] = NAN;
        }
    }
    return grafo;
}

void adicionar_aresta(Grafo *grafo, int v1, int v2, float peso, int direcionado) {
    if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
        NoListaAdj *novoNo = alocar_no_arena(grafo);
        if (!novoNo) {
            printf("Erro ao alocar memoria para nova aresta.\n");
            return;
        }
        novoNo->vertice = v2;
        novoNo->peso = peso;
        novoNo->proximo = grafo->lista_adj[v1];
        grafo->lista_adj[v1] = novoNo;
        grafo->graus[v1]++;

        if (!direcionado) {
            novoNo = alocar_no_arena(grafo);
            if (!novoNo) {
                printf("Erro ao alocar memoria para nova aresta (v2 -> v1).\n");
                return;
            }
            novoNo->vertice = v1;
            novoNo->peso = peso;
            novoNo->proximo = grafo->lista_adj[v2];
            grafo->lista_adj[v2] = novoNo;
            grafo->graus[v2]++;
        }
    } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
        grafo->matriz_adj[v1][v2] = peso;
        grafo->graus[v1]++;
        if (!direcionado) {
            grafo->matriz_adj[v2][v1] = peso;
            grafo->graus[v2]++;
        }
    }
    grafo->num_arestas++;
}

void liberar_grafo(Grafo *grafo) {
    if (!grafo) return;
    if (grafo->arena_nos) {
        BlocoArenaNos *bloco = grafo->arena_nos;
        while (bloco) {
            BlocoArenaNos *prox = bloco->proximo;
            free(bloco->dados);
            free(bloco);
            bloco = prox;
        }
    }
    free(grafo->lista_adj);
    if (grafo->matriz_adj) {
        for (int i = 0; i < grafo->num_vertices; i++) {
            free(grafo->matriz_adj[i]);
        }
        free(grafo->matriz_adj);
    }
    free(grafo->graus);
    free(grafo);
}

Grafo *ler_grafo_do_arquivo(const char *nome_arquivo, RepresentacaoGrafo tipo_representacao, int direcionado) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo '%s'.\n", nome_arquivo);
        return NULL;
    }
    int num_vertices;
    if (fscanf(arquivo, "%d", &num_vertices) != 1) {
        printf("Erro ao ler o numero de vertices.\n");
        fclose(arquivo);
        return NULL;
    }
    Grafo *grafo;
    if (tipo_representacao == LISTA_ADJACENCIA) {
        grafo = criar_grafo_lista_adj(num_vertices);
    } else if (tipo_representacao == MATRIZ_ADJACENCIA) {
        grafo = criar_grafo_matriz_adj(num_vertices);
    } else {
        printf("Tipo de representacao desconhecido.\n");
        fclose(arquivo);
        return NULL;
    }
    if (!grafo) {
        fclose(arquivo);
        return NULL;
    }
    int v1, v2;
    float peso;
    long linhas_lidas = 0;
    int campos;
    while ((campos = fscanf(arquivo, "%d %d %f", &v1, &v2, &peso)) == 3) {
        linhas_lidas++;
        v1--;
        v2--;

        if (v1 < 0 || v1 >= num_vertices || v2 < 0 || v2 >= num_vertices) {
            printf("Erro: vertices fora dos limites (%d, %d).\n", v1 + 1, v2 + 1);
            liberar_grafo(grafo);
            fclose(arquivo);
            return NULL;
        }
        adicionar_aresta(grafo, v1, v2, peso, direcionado);
    }
    if (campos != EOF) {
        /* parou antes do fim do arquivo: linha com menos de 3 colunas (ex.: grafo sem pesos) */
        printf("Erro: linha %ld invalida (esperado 'v1 v2 peso'); o arquivo nao tem a coluna de pesos?\n", linhas_lidas + 2);
        liberar_grafo(grafo);
        fclose(arquivo);
        return NULL;
    }
    fclose(arquivo);
    return grafo;
}

static int comparar_inteiros(const void *a, const void *b) {
    return (*(const int *)a - *(const int *)b);
}

void salvar_informacoes_grafo(Grafo *grafo, const char *nome_arquivo_saida) {
    FILE *arquivo_saida = fopen(nome_arquivo_saida, "w");
    if (!arquivo_saida) {
        printf("Erro ao abrir o arquivo de saida.\n");
        return;
    }
    fprintf(arquivo_saida, "Numero de vertices: %d\n", grafo->num_vertices);
    fprintf(arquivo_saida, "Numero de arestas: %d\n", grafo->num_arestas);
    int grau_min = grafo->graus[0], grau_max = grafo->graus[0];
    long long soma_graus = 0;
    for (int i = 0; i < grafo->num_vertices; i++) {
        if (grafo->graus[i] < grau_min) grau_min = grafo->graus[i];
        if (grafo->graus[i] > grau_max) grau_max = grafo->graus[i];
        soma_graus += grafo->graus[i];
    }
    double grau_medio = (double)soma_graus / grafo->num_vertices;
    int *graus_ordenados = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
    for (int i = 0; i < grafo->num_vertices; i++) {
        graus_ordenados[i] = grafo->graus[i];
    }
    qsort(graus_ordenados, (size_t)grafo->num_vertices, sizeof(int), comparar_inteiros);
    double mediana_grau;
    if (grafo->num_vertices % 2 == 0) {
        mediana_grau = (graus_ordenados[grafo->num_vertices / 2 - 1] + graus_ordenados[grafo->num_vertices / 2]) / 2.0;
    } else {
        mediana_grau = graus_ordenados[grafo->num_vertices / 2];
    }
    free(graus_ordenados);
    fprintf(arquivo_saida, "Grau minimo: %d\n", grau_min);
    fprintf(arquivo_saida, "Grau maximo: %d\n", grau_max);
    fprintf(arquivo_saida, "Grau medio: %.2f\n", grau_medio);
    fprintf(arquivo_saida, "Mediana do grau: %.2f\n", mediana_grau);
    fclose(arquivo_saida);
}

void busca_em_largura(Grafo *grafo, int vertice_inicial, int *pais, int *niveis) {
    vertice_inicial--;
    bool *visitados = (bool *)calloc((size_t)grafo->num_vertices, sizeof(bool));
    int *fila = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
    int inicio = 0, fim = 0;
    for (int i = 0; i < grafo->num_vertices; i++) {
        niveis[i] = -1;
        pais[i] = -1;
    }
    visitados[vertice_inicial] = true;
    niveis[vertice_inicial] = 0;
    fila[fim++] = vertice_inicial;
    while (inicio < fim) {
        int v = fila[inicio++];
        if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
            NoListaAdj *adj = grafo->lista_adj[v];
            while (adj != NULL) {
                if (!visitados[adj->vertice]) {
                    visitados[adj->vertice] = true;
                    pais[adj->vertice] = v;
                    niveis[adj->vertice] = niveis[v] + 1;
                    fila[fim++] = adj->vertice;
                }
                adj = adj->proximo;
            }
        } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
            for (int i = 0; i < grafo->num_vertices; i++) {
                if (!isnan(grafo->matriz_adj[v][i]) && !visitados[i]) {
                    visitados[i] = true;
                    pais[i] = v;
                    niveis[i] = niveis[v] + 1;
                    fila[fim++] = i;
                }
            }
        }
    }
    free(visitados);
    free(fila);
}

void busca_em_profundidade(Grafo *grafo, int vertice_inicial, int *pais, int *niveis) {
    vertice_inicial--;
    bool *visitados = (bool *)calloc((size_t)grafo->num_vertices, sizeof(bool));
    int *pilha = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
    int topo = -1;
    for (int i = 0; i < grafo->num_vertices; i++) {
        niveis[i] = -1;
        pais[i] = -1;
    }
    pilha[++topo] = vertice_inicial;
    visitados[vertice_inicial] = true;
    niveis[vertice_inicial] = 0;
    while (topo >= 0) {
        int v = pilha[topo--];
        if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
            NoListaAdj *adj = grafo->lista_adj[v];
            while (adj != NULL) {
                if (!visitados[adj->vertice]) {
                    visitados[adj->vertice] = true;
                    pais[adj->vertice] = v;
                    niveis[adj->vertice] = niveis[v] + 1;
                    pilha[++topo] = adj->vertice;
                }
                adj = adj->proximo;
            }
        } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
            for (int i = 0; i < grafo->num_vertices; i++) {
                if (!isnan(grafo->matriz_adj[v][i]) && !visitados[i]) {
                    visitados[i] = true;
                    pais[i] = v;
                    niveis[i] = niveis[v] + 1;
                    pilha[++topo] = i;
                }
            }
        }
    }
    free(visitados);
    free(pilha);
}

int calcular_distancia(Grafo *grafo, int origem, int destino) {
    origem--;
    destino--;
    if (origem == destino) return 0;
    if (origem < 0 || origem >= grafo->num_vertices || destino < 0 || destino >= grafo->num_vertices) return -1;
    bool *visitados = (bool *)calloc((size_t)grafo->num_vertices, sizeof(bool));
    int *distancia = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
    int *fila = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
    int inicio = 0, fim = 0;
    for (int i = 0; i < grafo->num_vertices; i++) {
        distancia[i] = -1;
    }
    visitados[origem] = true;
    distancia[origem] = 0;
    fila[fim++] = origem;
    while (inicio < fim) {
        int u = fila[inicio++];
        if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
            NoListaAdj *adj = grafo->lista_adj[u];
            while (adj) {
                int v = adj->vertice;
                if (!visitados[v]) {
                    visitados[v] = true;
                    distancia[v] = distancia[u] + 1;
                    fila[fim++] = v;
                    if (v == destino) {
                        int dist = distancia[destino];
                        free(visitados);
                        free(distancia);
                        free(fila);
                        return dist;
                    }
                }
                adj = adj->proximo;
            }
        } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
            for (int v = 0; v < grafo->num_vertices; v++) {
                if (!isnan(grafo->matriz_adj[u][v]) && !visitados[v]) {
                    visitados[v] = true;
                    distancia[v] = distancia[u] + 1;
                    fila[fim++] = v;
                    if (v == destino) {
                        int dist = distancia[destino];
                        free(visitados);
                        free(distancia);
                        free(fila);
                        return dist;
                    }
                }
            }
        }
    }
    free(visitados);
    free(distancia);
    free(fila);
    return -1;
}

int calcular_diametro(Grafo *grafo) {
    if (grafo->num_vertices <= 1000) {
        int max_distancia = 0;
        for (int i = 0; i < grafo->num_vertices; i++) {
            for (int j = i + 1; j < grafo->num_vertices; j++) {
                int dist = calcular_distancia(grafo, i + 1, j + 1);
                if (dist != -1 && dist > max_distancia) {
                    max_distancia = dist;
                }
            }
        }
        return max_distancia;
    } else {
        int max_distancia = 0;
        int amostra_size = 100;
        srand((unsigned int)time(NULL));
        for (int k = 0; k < amostra_size; k++) {
            int vertice_inicial = (rand() % grafo->num_vertices) + 1;
            bool *visitados = (bool *)calloc((size_t)grafo->num_vertices, sizeof(bool));
            int *distancia = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
            int *fila = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
            int inicio = 0, fim = 0;
            for (int i = 0; i < grafo->num_vertices; i++) {
                distancia[i] = -1;
            }
            visitados[vertice_inicial - 1] = true;
            distancia[vertice_inicial - 1] = 0;
            fila[fim++] = vertice_inicial - 1;
            while (inicio < fim) {
                int u = fila[inicio++];
                if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
                    NoListaAdj *adj = grafo->lista_adj[u];
                    while (adj) {
                        int v = adj->vertice;
                        if (!visitados[v]) {
                            visitados[v] = true;
                            distancia[v] = distancia[u] + 1;
                            fila[fim++] = v;
                        }
                        adj = adj->proximo;
                    }
                } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
                    for (int v = 0; v < grafo->num_vertices; v++) {
                        if (!isnan(grafo->matriz_adj[u][v]) && !visitados[v]) {
                            visitados[v] = true;
                            distancia[v] = distancia[u] + 1;
                            fila[fim++] = v;
                        }
                    }
                }
            }
            int local_max_dist = 0;
            for (int i = 0; i < grafo->num_vertices; i++) {
                if (distancia[i] > local_max_dist) {
                    local_max_dist = distancia[i];
                }
            }
            if (local_max_dist > max_distancia) {
                max_distancia = local_max_dist;
            }
            free(visitados);
            free(distancia);
            free(fila);
        }
        return max_distancia;
    }
}

void encontrar_componentes_conexas(Grafo *grafo, const char *nome_arquivo_saida) {
    FILE *arquivo_saida = fopen(nome_arquivo_saida, "w");
    if (!arquivo_saida) {
        printf("Erro ao abrir o arquivo de saida.\n");
        return;
    }
    bool *visitados = (bool *)calloc((size_t)grafo->num_vertices, sizeof(bool));
    ComponenteConexa *componentes = (ComponenteConexa *)malloc((size_t)grafo->num_vertices * sizeof(ComponenteConexa));
    int num_componentes = 0;
    for (int i = 0; i < grafo->num_vertices; i++) {
        if (!visitados[i]) {
            componentes[num_componentes].vertices = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
            componentes[num_componentes].tamanho = 0;
            int *fila = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
            int inicio = 0, fim = 0;
            fila[fim++] = i;
            visitados[i] = true;
            while (inicio < fim) {
                int vertice = fila[inicio++];
                componentes[num_componentes].vertices[componentes[num_componentes].tamanho++] = vertice;
                if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
                    NoListaAdj *adj = grafo->lista_adj[vertice];
                    while (adj != NULL) {
                        if (!visitados[adj->vertice]) {
                            visitados[adj->vertice] = true;
                            fila[fim++] = adj->vertice;
                        }
                        adj = adj->proximo;
                    }
                } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
                    for (int j = 0; j < grafo->num_vertices; j++) {
                        if (!isnan(grafo->matriz_adj[vertice][j]) && !visitados[j]) {
                            visitados[j] = true;
                            fila[fim++] = j;
                        }
                    }
                }
            }
            free(fila);
            num_componentes++;
        }
    }
    qsort(componentes, (size_t)num_componentes, sizeof(ComponenteConexa), comparar_componentes);
    fprintf(arquivo_saida, "\nComponentes Conexas:\n");
    for (int i = 0; i < num_componentes; i++) {
        fprintf(arquivo_saida, "Tamanho: %d, Vertices: ", componentes[i].tamanho);
        for (int j = 0; j < componentes[i].tamanho; j++) {
            fprintf(arquivo_saida, "%d ", componentes[i].vertices[j] + 1);
        }
        fprintf(arquivo_saida, "\n");
        free(componentes[i].vertices);
    }
    fprintf(arquivo_saida, "Numero total de componentes conexas: %d\n", num_componentes);
    free(visitados);
    free(componentes);
    fclose(arquivo_saida);
}

/* ============================================================================
 * DIJKSTRA - uma unica implementacao do algoritmo.
 *
 * O que varia entre as duas versoes exigidas (vetor x heap) e apenas COMO as estimativas de
 * distancia sao guardadas e COMO o minimo e extraido. Isso e abstraido em FilaPrio; o laco
 * principal e o relaxamento das arestas (lista ou matriz) existem uma unica vez em dijkstra_core.
 * ========================================================================== */
typedef struct FilaPrio {
    void *estado;
    /* chamada quando distancias[v] diminui (v e o vertice, d a nova estimativa) */
    void (*atualizar)(struct FilaPrio *fp, int v, float d);
    /* remove e devolve o vertice nao finalizado de menor estimativa; -1 se nao houver */
    int (*extrair_min)(struct FilaPrio *fp);
} FilaPrio;

/* --- Versao 1: vetor. A propria distancias[] e a estrutura; extrair_min varre os V vertices --- */
typedef struct {
    int V;
    const float *distancias;
    bool *extraido;
} EstadoVetor;

static void vetor_atualizar(FilaPrio *fp, int v, float d) {
    (void)fp; (void)v; (void)d; /* nada a fazer: distancias[v] ja foi atualizado pelo nucleo */
}

static int vetor_extrair_min(FilaPrio *fp) {
    EstadoVetor *e = (EstadoVetor *)fp->estado;
    float menor = FLT_MAX;
    int u = -1;
    for (int v = 0; v < e->V; v++) {
        if (!e->extraido[v] && e->distancias[v] < menor) {
            menor = e->distancias[v];
            u = v;
        }
    }
    if (u != -1) e->extraido[u] = true;
    return u;
}

/* --- Versao 2: MinHeap com diminuir_chave em O(log V) (posicoes rastreadas em heap->pos) --- */
static void heap_atualizar(FilaPrio *fp, int v, float d) {
    MinHeap *h = (MinHeap *)fp->estado;
    if (estaNaHeap_MinHeap(h, v)) diminuir_chave_MinHeap(h, v, d);
    else insere_MinHeap(h, v, d);
}

static int heap_extrair_min(FilaPrio *fp) {
    MinHeap *h = (MinHeap *)fp->estado;
    if (heapVazia_MinHeap(h)) return -1;
    return extrai_min_MinHeap(h).vertice;
}

/* Nucleo unico: pressupoe pesos nao negativos (quem chama deve checar possui_arestas_negativas). */
static void dijkstra_core(const Grafo *grafo, int vertice_inicial, float *distancias, int *pais,
                          FilaPrio *fp, bool *finalizado) {
    int V = grafo->num_vertices;
    for (int i = 0; i < V; i++) {
        distancias[i] = FLT_MAX;
        pais[i] = -1;
        finalizado[i] = false;
    }
    int s = vertice_inicial - 1;
    distancias[s] = 0.0f;
    fp->atualizar(fp, s, 0.0f);

    int u;
    while ((u = fp->extrair_min(fp)) != -1) {
        finalizado[u] = true;
        float du = distancias[u];

        if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
            for (NoListaAdj *a = grafo->lista_adj[u]; a != NULL; a = a->proximo) {
                int v = a->vertice;
                if (!finalizado[v] && du + a->peso < distancias[v]) {
                    distancias[v] = du + a->peso;
                    pais[v] = u;
                    fp->atualizar(fp, v, distancias[v]);
                }
            }
        } else {
            for (int v = 0; v < V; v++) {
                float w = grafo->matriz_adj[u][v];
                if (!isnan(w) && !finalizado[v] && du + w < distancias[v]) {
                    distancias[v] = du + w;
                    pais[v] = u;
                    fp->atualizar(fp, v, distancias[v]);
                }
            }
        }
    }
}

void dijkstra(Grafo *grafo, int vertice_inicial, float *distancias, int *pais) {
    int V = grafo->num_vertices;
    MinHeap *heap = cria_MinHeap(V);
    bool *finalizado = (bool *)malloc((size_t)V * sizeof(bool));
    if (!heap || !finalizado) {
        printf("Erro: falha ao alocar memoria para o Dijkstra (com heap).\n");
        libera_MinHeap(heap);
        free(finalizado);
        return;
    }
    FilaPrio fp = { heap, heap_atualizar, heap_extrair_min };
    dijkstra_core(grafo, vertice_inicial, distancias, pais, &fp, finalizado);
    libera_MinHeap(heap);
    free(finalizado);
}

void dijkstra_sem_heap(Grafo *grafo, int vertice_inicial, float *distancias, int *pais) {
    int V = grafo->num_vertices;
    bool *finalizado = (bool *)malloc((size_t)V * sizeof(bool));
    if (!finalizado) {
        printf("Erro: falha ao alocar memoria para o Dijkstra (sem heap).\n");
        return;
    }
    /* o mesmo vetor 'finalizado' serve de marca de "ja extraido" para a varredura linear */
    EstadoVetor ev = { V, distancias, finalizado };
    FilaPrio fp = { &ev, vetor_atualizar, vetor_extrair_min };
    dijkstra_core(grafo, vertice_inicial, distancias, pais, &fp, finalizado);
    free(finalizado);
}

float get_peso_aresta(const Grafo *grafo, int u_idx, int v_idx) {
    if (!grafo || u_idx < 0 || v_idx < 0 || u_idx >= grafo->num_vertices || v_idx >= grafo->num_vertices) {
        return NAN;
    }

    if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
        NoListaAdj *adj = grafo->lista_adj[u_idx];
        float melhor_peso = NAN;
        while (adj != NULL) {
            if (adj->vertice == v_idx) {
                if (isnan(melhor_peso) || adj->peso < melhor_peso) {
                    melhor_peso = adj->peso;
                }
            }
            adj = adj->proximo;
        }
        return melhor_peso;
    } else {
        return grafo->matriz_adj[u_idx][v_idx];
    }
}

void salvar_caminho_minimo(Grafo *grafo, const int *pais, int origem_idx, int destino_idx, int num_vertices, FILE *arquivo) {
    if (pais[destino_idx] == -1 && destino_idx != origem_idx) {
        fprintf(arquivo, "Nenhum caminho encontrado.\n");
        return;
    }

    if (destino_idx == origem_idx) {
        fprintf(arquivo, "%d\n", origem_idx + 1);
        return;
    }

    int *caminho = (int *)malloc((size_t)num_vertices * sizeof(int));
    if (caminho == NULL) {
        fprintf(arquivo, "Erro de memoria ao gerar caminho.\n");
        return;
    }

    int count = 0;
    int curr = destino_idx;

    while (curr != -1 && count < num_vertices) {
        caminho[count] = curr;
        count++;
        curr = pais[curr];
    }

    for (int i = count - 1; i >= 0; i--) {
        fprintf(arquivo, "%d", caminho[i] + 1);
        if (i > 0) {
            fprintf(arquivo, " -> ");
        }
    }
    fprintf(arquivo, "\n");

    fprintf(arquivo, "Custo das arestas: (");
    for (int i = count - 1; i > 0; i--) {
        int u = caminho[i];
        int v = caminho[i - 1];
        float peso = get_peso_aresta(grafo, u, v);
        fprintf(arquivo, "%.2f", peso);
        if (i > 1) fprintf(arquivo, ", ");
    }
    fprintf(arquivo, ")\n");

    free(caminho);
}

/* Nucleo interno do Bellman-Ford sobre Grafo com reutilizacao de workspace */
static bool bellman_ford_grafo_ws(const Grafo *grafo, int origem, float *distancias, int *pais,
                                  Fila *fila, int *arestas_caminho) {
    int V = grafo->num_vertices;
    int s = origem - 1;
    if (s < 0 || s >= V) return false;

    for (int i = 0; i < V; i++) {
        distancias[i] = FLT_MAX;
        pais[i] = -1;
        arestas_caminho[i] = 0;
    }
    limpar_fila(fila);

    distancias[s] = 0.0f;
    enfileirar_slf(fila, s, distancias);

    while (!fila_vazia(fila)) {
        int u = desenfileirar(fila);
        if (u < 0 || u >= V) continue;

        float dist_u = distancias[u];
        int hops_u = arestas_caminho[u];

        if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
            NoListaAdj *atual = grafo->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                float peso = atual->peso;

                if (v >= 0 && v < V && dist_u + peso < distancias[v] - EPS_DIST) {
                    distancias[v] = dist_u + peso;
                    pais[v] = u;
                    arestas_caminho[v] = hops_u + 1;

                    /* Caminho simples tem no maximo V - 1 arestas (Aula 14, Slide 6) */
                    if (arestas_caminho[v] >= V) {
                        return false;
                    }
                    enfileirar_slf(fila, v, distancias);
                }
                atual = atual->proximo;
            }
        } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
            for (int v = 0; v < V; v++) {
                float peso = grafo->matriz_adj[u][v];
                if (!isnan(peso) && dist_u + peso < distancias[v] - EPS_DIST) {
                    distancias[v] = dist_u + peso;
                    pais[v] = u;
                    arestas_caminho[v] = hops_u + 1;

                    if (arestas_caminho[v] >= V) {
                        return false;
                    }
                    enfileirar_slf(fila, v, distancias);
                }
            }
        }
    }

    return true;
}

bool bellman_ford_otimizado(const Grafo *grafo, int origem, float *distancias, int *pais) {
    if (!grafo || !distancias || !pais) return false;
    int V = grafo->num_vertices;
    if (origem < 1 || origem > V) return false;

    Fila *fila = criar_fila(V);
    int *arestas_caminho = (int *)malloc((size_t)V * sizeof(int));
    if (!fila || !arestas_caminho) {
        destruir_fila(fila);
        free(arestas_caminho);
        return false;
    }

    bool ok = bellman_ford_grafo_ws(grafo, origem, distancias, pais, fila, arestas_caminho);

    destruir_fila(fila);
    free(arestas_caminho);
    return ok;
}

bool bellman_ford_csr(const GrafoCSR *csr, int origem, float *distancias, int *pais,
                      Fila *fila, int *arestas_caminho) {
    if (!csr || !distancias || !pais || !fila || !arestas_caminho) return false;
    int V = csr->num_vertices;
    int s = origem - 1;
    if (s < 0 || s >= V) return false;

    for (int i = 0; i < V; i++) {
        distancias[i] = FLT_MAX;
        pais[i] = -1;
        arestas_caminho[i] = 0;
    }
    limpar_fila(fila);

    distancias[s] = 0.0f;
    enfileirar_slf(fila, s, distancias);

    while (!fila_vazia(fila)) {
        int u = desenfileirar(fila);
        float dist_u = distancias[u];
        int hops_u = arestas_caminho[u];

        int inicio = csr->offsets[u];
        int fim = csr->offsets[u + 1];
        for (int e = inicio; e < fim; e++) {
            int v = csr->destinos[e];
            float peso = csr->pesos[e];

            if (dist_u + peso < distancias[v] - EPS_DIST) {
                distancias[v] = dist_u + peso;
                pais[v] = u;
                arestas_caminho[v] = hops_u + 1;

                if (arestas_caminho[v] >= V) {
                    return false;
                }
                enfileirar_slf(fila, v, distancias);
            }
        }
    }

    return true;
}

bool bellman_ford_para_destino(const Grafo *grafo, int destino, float *distancias, int *pais) {
    if (!grafo || !distancias || !pais || destino < 1 || destino > grafo->num_vertices) {
        return false;
    }

    GrafoCSR *csr = construir_grafo_transposto_csr(grafo);
    if (!csr) return false;

    int V = grafo->num_vertices;
    Fila *fila = criar_fila(V);
    int *arestas_caminho = (int *)malloc((size_t)V * sizeof(int));
    if (!fila || !arestas_caminho) {
        destruir_fila(fila);
        free(arestas_caminho);
        liberar_grafo_csr(csr);
        return false;
    }

    bool ok = bellman_ford_csr(csr, destino, distancias, pais, fila, arestas_caminho);

    destruir_fila(fila);
    free(arestas_caminho);
    liberar_grafo_csr(csr);
    return ok;
}

bool bellman_ford_todos_pares(const Grafo *grafo, float *distancias_1d, int *pred_1d) {
    if (!grafo || !distancias_1d || !pred_1d) return false;
    int V = grafo->num_vertices;

    GrafoCSR *csr = construir_grafo_csr(grafo);
    Fila *fila = criar_fila(V);
    int *arestas_caminho = (int *)malloc((size_t)V * sizeof(int));
    if (!csr || !fila || !arestas_caminho) {
        liberar_grafo_csr(csr);
        destruir_fila(fila);
        free(arestas_caminho);
        return false;
    }

    bool sem_ciclo = true;
    for (int s = 0; s < V; s++) {
        float *dist_s = &distancias_1d[(size_t)s * (size_t)V];
        int *pred_s = &pred_1d[(size_t)s * (size_t)V];
        if (!bellman_ford_csr(csr, s + 1, dist_s, pred_s, fila, arestas_caminho)) {
            sem_ciclo = false;
            break;
        }
    }

    destruir_fila(fila);
    free(arestas_caminho);
    liberar_grafo_csr(csr);
    return sem_ciclo;
}

float *alocar_matriz_distancias_1d(int num_vertices) {
    if (num_vertices <= 0) return NULL;
    size_t total = (size_t)num_vertices * (size_t)num_vertices;
    return (float *)malloc(total * sizeof(float));
}

int *alocar_matriz_predecessores_1d(int num_vertices) {
    if (num_vertices <= 0) return NULL;
    size_t total = (size_t)num_vertices * (size_t)num_vertices;
    return (int *)malloc(total * sizeof(int));
}

/*
 * Algoritmo de Floyd-Warshall (Aula 13):
 * - Otimizacao de memoria: utiliza apenas uma matriz 2D contigua distancias_1d[V * V]
 *   atualizada in-place (Theta(V^2) em vez de Theta(V^3)) com float (4 bytes).
 * - Arvores de caminhos minimos: matriz pred_1d[V * V] (pred[i][j] = pred[k][j]).
 * - Deteccao de ciclos negativos: parada antecipada no laco k se d[i][i] < 0.
 */
/* Executa os passos k em [k_ini, k_fim) do Floyd-Warshall, in-place, sobre matrizes V x V ja
 * inicializadas. Retorna false se detectar ciclo negativo. Separado de floyd_warshall() para que o
 * main possa cronometrar alguns passos numa matriz grande (calibracao da estimativa de tempo). */
bool floyd_warshall_passos(int V, float *distancias_1d, int *pred_1d, int k_ini, int k_fim) {
    size_t n = (size_t)V;
    for (size_t k = (size_t)k_ini; k < (size_t)k_fim; k++) {
        const float *dist_k = &distancias_1d[k * n];
        const int *pred_k = &pred_1d[k * n];

        if (dist_k[k] < -EPS_DIST) {
            return false;
        }

        for (size_t i = 0; i < n; i++) {
            float *dist_i = &distancias_1d[i * n];
            float dik = dist_i[k];
            if (isinf(dik)) continue;

            int *pred_i = &pred_1d[i * n];
            for (size_t j = 0; j < n; j++) {
                float nova_dist = dik + dist_k[j];
                if (nova_dist < dist_i[j] - EPS_DIST) {
                    dist_i[j] = nova_dist;
                    pred_i[j] = pred_k[j];
                }
            }

            /* Aula 13, Slide 18: se d(i, i) < 0 em qualquer passo k, ha ciclo negativo */
            if (dist_i[i] < -EPS_DIST) {
                return false;
            }
        }
    }
    return true;
}

bool floyd_warshall(const Grafo *grafo, float *distancias_1d, int *pred_1d) {
    if (!grafo || !distancias_1d || !pred_1d) return false;
    int V = grafo->num_vertices;
    size_t n = (size_t)V;

    /* 1. Inicializacao das matrizes contiguas usando INFINITY (IEEE-754)
     * para eliminar ramificacoes no laco interno j e permitir vetorizacao SIMD. */
    for (size_t i = 0; i < n; i++) {
        float *dist_i = &distancias_1d[i * n];
        int *pred_i = &pred_1d[i * n];
        for (size_t j = 0; j < n; j++) {
            dist_i[j] = (i == j) ? 0.0f : INFINITY;
            pred_i[j] = -1;
        }
    }

    if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            NoListaAdj *atual = grafo->lista_adj[u];
            float *dist_u = &distancias_1d[(size_t)u * n];
            int *pred_u = &pred_1d[(size_t)u * n];
            while (atual != NULL) {
                int v = atual->vertice;
                float w = atual->peso;
                if (w < dist_u[v]) {
                    dist_u[v] = w;
                    pred_u[v] = (u != v) ? u : -1;
                }
                atual = atual->proximo;
            }
        }
    } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            float *dist_u = &distancias_1d[(size_t)u * n];
            int *pred_u = &pred_1d[(size_t)u * n];
            for (int v = 0; v < V; v++) {
                float w = grafo->matriz_adj[u][v];
                if (!isnan(w) && w < dist_u[v]) {
                    dist_u[v] = w;
                    pred_u[v] = (u != v) ? u : -1;
                }
            }
        }
    }

    /* 2. Programacao dinamica in-place (todos os passos k) */
    if (!floyd_warshall_passos(V, distancias_1d, pred_1d, 0, V)) {
        return false;
    }

    /* 3. Normaliza INFINITY de volta para FLT_MAX para manter compatibilidade com a API */
    size_t total_pares = n * n;
    for (size_t idx = 0; idx < total_pares; idx++) {
        if (isinf(distancias_1d[idx])) {
            distancias_1d[idx] = FLT_MAX;
        }
    }

    return true;
}

int possui_arestas_negativas(const Grafo *grafo) {
    if (!grafo) return 0;

    if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
        for (int i = 0; i < grafo->num_vertices; i++) {
            NoListaAdj *atual = grafo->lista_adj[i];
            while (atual != NULL) {
                if (atual->peso < 0.0f) return 1;
                atual = atual->proximo;
            }
        }
    } else if (grafo->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int i = 0; i < grafo->num_vertices; i++) {
            for (int j = 0; j < grafo->num_vertices; j++) {
                if (!isnan(grafo->matriz_adj[i][j]) && grafo->matriz_adj[i][j] < 0.0f) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

GrafoCSR *construir_grafo_csr(const Grafo *original) {
    if (!original) return NULL;
    int V = original->num_vertices;

    GrafoCSR *csr = (GrafoCSR *)malloc(sizeof(GrafoCSR));
    if (!csr) return NULL;

    csr->num_vertices = V;
    csr->num_arestas = 0;
    csr->offsets = (int *)calloc((size_t)(V + 1), sizeof(int));
    csr->destinos = NULL;
    csr->pesos = NULL;

    if (!csr->offsets) {
        free(csr);
        return NULL;
    }

    int total_arestas = 0;
    if (original->tipo_representacao == LISTA_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            NoListaAdj *atual = original->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                if (v >= 0 && v < V) {
                    csr->offsets[u + 1]++;
                    total_arestas++;
                }
                atual = atual->proximo;
            }
        }
    } else if (original->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            for (int v = 0; v < V; v++) {
                if (!isnan(original->matriz_adj[u][v])) {
                    csr->offsets[u + 1]++;
                    total_arestas++;
                }
            }
        }
    }

    csr->num_arestas = total_arestas;
    for (int i = 0; i < V; i++) {
        csr->offsets[i + 1] += csr->offsets[i];
    }

    csr->destinos = (int *)malloc((size_t)(total_arestas > 0 ? total_arestas : 1) * sizeof(int));
    csr->pesos = (float *)malloc((size_t)(total_arestas > 0 ? total_arestas : 1) * sizeof(float));
    int *cursor = (int *)malloc((size_t)V * sizeof(int));

    if (!csr->destinos || !csr->pesos || !cursor) {
        free(cursor);
        liberar_grafo_csr(csr);
        return NULL;
    }
    memcpy(cursor, csr->offsets, (size_t)V * sizeof(int));

    if (original->tipo_representacao == LISTA_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            NoListaAdj *atual = original->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                if (v >= 0 && v < V) {
                    int pos = cursor[u]++;
                    csr->destinos[pos] = v;
                    csr->pesos[pos] = atual->peso;
                }
                atual = atual->proximo;
            }
        }
    } else if (original->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            for (int v = 0; v < V; v++) {
                float w = original->matriz_adj[u][v];
                if (!isnan(w)) {
                    int pos = cursor[u]++;
                    csr->destinos[pos] = v;
                    csr->pesos[pos] = w;
                }
            }
        }
    }

    free(cursor);
    return csr;
}

GrafoCSR *construir_grafo_transposto_csr(const Grafo *original) {
    if (!original) return NULL;
    int V = original->num_vertices;

    GrafoCSR *csr = (GrafoCSR *)malloc(sizeof(GrafoCSR));
    if (!csr) return NULL;

    csr->num_vertices = V;
    csr->num_arestas = 0;
    csr->offsets = (int *)calloc((size_t)(V + 1), sizeof(int));
    csr->destinos = NULL;
    csr->pesos = NULL;

    if (!csr->offsets) {
        free(csr);
        return NULL;
    }

    /* 1. Contagem do grau de entrada de cada vertice (= grau de saida no transposto) */
    int total_arestas = 0;
    if (original->tipo_representacao == LISTA_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            NoListaAdj *atual = original->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                if (v >= 0 && v < V) {
                    csr->offsets[v + 1]++;
                    total_arestas++;
                }
                atual = atual->proximo;
            }
        }
    } else if (original->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            for (int v = 0; v < V; v++) {
                if (!isnan(original->matriz_adj[u][v])) {
                    csr->offsets[v + 1]++;
                    total_arestas++;
                }
            }
        }
    }

    csr->num_arestas = total_arestas;
    for (int i = 0; i < V; i++) {
        csr->offsets[i + 1] += csr->offsets[i];
    }

    csr->destinos = (int *)malloc((size_t)(total_arestas > 0 ? total_arestas : 1) * sizeof(int));
    csr->pesos = (float *)malloc((size_t)(total_arestas > 0 ? total_arestas : 1) * sizeof(float));
    int *cursor = (int *)malloc((size_t)V * sizeof(int));

    if (!csr->destinos || !csr->pesos || !cursor) {
        free(cursor);
        liberar_grafo_csr(csr);
        return NULL;
    }
    memcpy(cursor, csr->offsets, (size_t)V * sizeof(int));

    /* 2. Preenchimento contiguo das arestas invertidas (v -> u) */
    if (original->tipo_representacao == LISTA_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            NoListaAdj *atual = original->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                if (v >= 0 && v < V) {
                    int pos = cursor[v]++;
                    csr->destinos[pos] = u;
                    csr->pesos[pos] = atual->peso;
                }
                atual = atual->proximo;
            }
        }
    } else if (original->tipo_representacao == MATRIZ_ADJACENCIA) {
        for (int u = 0; u < V; u++) {
            for (int v = 0; v < V; v++) {
                float w = original->matriz_adj[u][v];
                if (!isnan(w)) {
                    int pos = cursor[v]++;
                    csr->destinos[pos] = u;
                    csr->pesos[pos] = w;
                }
            }
        }
    }

    free(cursor);
    return csr;
}

void liberar_grafo_csr(GrafoCSR *csr) {
    if (!csr) return;
    free(csr->offsets);
    free(csr->destinos);
    free(csr->pesos);
    free(csr);
}

Grafo *inverter_grafo(const Grafo *original) {
    if (!original) return NULL;

    Grafo *inverso;
    if (original->tipo_representacao == LISTA_ADJACENCIA)
        inverso = criar_grafo_lista_adj(original->num_vertices);
    else
        inverso = criar_grafo_matriz_adj(original->num_vertices);

    if (!inverso) return NULL;

    for (int u = 0; u < original->num_vertices; u++) {
        if (original->tipo_representacao == LISTA_ADJACENCIA) {
            NoListaAdj *atual = original->lista_adj[u];
            while (atual != NULL) {
                int v = atual->vertice;
                adicionar_aresta(inverso, v, u, atual->peso, 1);
                atual = atual->proximo;
            }
        } else if (original->tipo_representacao == MATRIZ_ADJACENCIA) {
            for (int v = 0; v < original->num_vertices; v++) {
                float peso = original->matriz_adj[u][v];
                if (!isnan(peso)) {
                    adicionar_aresta(inverso, v, u, peso, 1);
                }
            }
        }
    }
    return inverso;
}
