#ifndef BIBLIOTECA_GRAFOS_H
#define BIBLIOTECA_GRAFOS_H

#include <stdio.h>
#include <stdbool.h>
#include "fila.h"

/* Tolerancia numerica para comparar distancias em float.
 * Sem ela, o arredondamento (ex.: (d + w) - w < d por 1 ulp) cria "melhorias" falsas em ciclos de
 * soma zero: ciclo negativo inexistente e cadeia de pais em laco. Com pesos de 2 casas decimais,
 * melhorias reais sao >= 0.01, muito acima de EPS_DIST. */
#define EPS_DIST 1e-4f

typedef enum { LISTA_ADJACENCIA, MATRIZ_ADJACENCIA } RepresentacaoGrafo;

typedef struct NoListaAdj {
    int vertice;
    float peso;
    struct NoListaAdj *proximo;
} NoListaAdj;

typedef struct BlocoArenaNos {
    NoListaAdj *dados;
    int usado;
    int capacidade;
    struct BlocoArenaNos *proximo;
} BlocoArenaNos;

typedef struct {
    int num_vertices;
    int num_arestas;
    int *graus;
    NoListaAdj **lista_adj;
    float **matriz_adj;
    RepresentacaoGrafo tipo_representacao;
    BlocoArenaNos *arena_nos;
} Grafo;

/* Representacao compacta CSR (Compressed Sparse Row) para o grafo transposto */
typedef struct {
    int num_vertices;
    int num_arestas;
    int *offsets;
    int *destinos;
    float *pesos;
} GrafoCSR;

Grafo* criar_grafo_lista_adj(int num_vertices);
Grafo* criar_grafo_matriz_adj(int num_vertices);
Grafo* inverter_grafo(const Grafo *original);
GrafoCSR* construir_grafo_csr(const Grafo *original);
GrafoCSR* construir_grafo_transposto_csr(const Grafo *original);
void liberar_grafo_csr(GrafoCSR *csr);

void adicionar_aresta(Grafo *grafo, int v1, int v2, float peso, int direcionado);
void liberar_grafo(Grafo *grafo);

void busca_em_largura(Grafo *grafo, int vertice_inicial, int *pais, int *niveis);
void busca_em_profundidade(Grafo *grafo, int vertice_inicial, int *pais, int *niveis);

int calcular_distancia(Grafo *grafo, int origem, int destino);
int calcular_diametro(Grafo *grafo);
int possui_arestas_negativas(const Grafo *grafo);

void encontrar_componentes_conexas(Grafo *grafo, const char *nome_arquivo_saida);
void dijkstra(Grafo *grafo, int vertice_inicial, float *distancias, int *pais);
void dijkstra_sem_heap(Grafo *grafo, int vertice_inicial, float *distancias, int *pais);

/* Bellman-Ford (1-para-todos a partir de 'origem' 1-indexado) com:
 * 1) Vetor 1D de distancias O(V) em vez de matriz O(V^2) (Aula 14)
 * 2) Fila de vertices ativos com heuristica SLF + parada antecipada (Aula 14)
 * 3) Deteccao exata de ciclo negativo via comprimento do caminho (arestas >= V) */
bool bellman_ford_otimizado(const Grafo *grafo, int origem, float *distancias, int *pais);

/* Bellman-Ford sobre grafo CSR reutilizando buffers pre-alocados */
bool bellman_ford_csr(const GrafoCSR *csr, int origem, float *distancias, int *pais,
                      Fila *fila, int *arestas_caminho);

/* Bellman-Ford (todos-para-1 rumo ao 'destino' 1-indexado) usando grafo transposto CSR */
bool bellman_ford_para_destino(const Grafo *grafo, int destino, float *distancias, int *pais);

/* Bellman-Ford para todos os pares (matrizes 1D contiguas V x V, reutilizando workspace) */
bool bellman_ford_todos_pares(const Grafo *grafo, float *distancias_1d, int *pred_1d);

/* Floyd-Warshall (todos os pares) com matrizes 1D contiguas V x V (Aula 13):
 * - distancias_1d[i * V + j]: menor distancia de i para j (0-indexado)
 * - pred_1d[i * V + j]: pai de j no caminho minimo partindo de i (0-indexado)
 * - Parada antecipada no laco k se distancias_1d[i * V + i] < 0 */
bool floyd_warshall(const Grafo *grafo, float *distancias_1d, int *pred_1d);

/* Executa apenas os passos k em [k_ini, k_fim) (matrizes ja inicializadas); false se ha ciclo negativo */
bool floyd_warshall_passos(int V, float *distancias_1d, int *pred_1d, int k_ini, int k_fim);

float* alocar_matriz_distancias_1d(int num_vertices);
int* alocar_matriz_predecessores_1d(int num_vertices);

void salvar_caminho_minimo(Grafo *grafo, const int *pais, int origem_idx, int destino_idx, int num_vertices, FILE *arquivo);
float get_peso_aresta(const Grafo *grafo, int u_idx, int v_idx);

Grafo* ler_grafo_do_arquivo(const char* nome_arquivo, RepresentacaoGrafo tipo_representacao, int direcionado);
void salvar_informacoes_grafo(Grafo *grafo, const char *nome_arquivo_saida);

#endif
