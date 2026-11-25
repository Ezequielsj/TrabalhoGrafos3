#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "biblioteca_grafos.h"

#define PASTA_RESULTADOS "resultados"
#define NUM_EXECUCOES_BENCHMARK 10

/* Limite pratico para os Casos 2 e 3 (2 matrizes V x V = 8 * V^2 bytes; V=30000 -> ~6.71 GB).
 * Alem deste limite, o programa tambem confere a RAM disponivel antes de alocar. */
static int max_vertices_floyd(void);
#define MAX_VERTICES_FLOYD max_vertices_floyd()

/* Limite pratico para o Caso 4 (pico de 3 matrizes V x V = 12 * V^2 bytes + tempo cubico x 10 rodadas) */
#define MAX_VERTICES_CASO4 5000

/* Orcamento padrao (s) para repetir o Floyd-Warshall no Caso 3 (variavel de ambiente TP3_ORCAMENTO_FW) */
#define ORCAMENTO_FW_PADRAO_S 900.0

static double obter_tempo_segundos(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void garantir_diretorio_resultados(void) {
#ifdef _WIN32
    _mkdir(PASTA_RESULTADOS);
#else
    mkdir(PASTA_RESULTADOS, 0777);
#endif
}

const char *extrair_nome_base(const char *caminho, char *buffer, size_t tam_buffer) {
    const char *barra1 = strrchr(caminho, '/');
    const char *barra2 = strrchr(caminho, '\\');
    const char *inicio = caminho;

    if (barra1 && barra2) {
        inicio = (barra1 > barra2) ? barra1 + 1 : barra2 + 1;
    } else if (barra1) {
        inicio = barra1 + 1;
    } else if (barra2) {
        inicio = barra2 + 1;
    }

    strncpy(buffer, inicio, tam_buffer - 1);
    buffer[tam_buffer - 1] = '\0';

    char *ponto = strrchr(buffer, '.');
    if (ponto && ponto != buffer) {
        *ponto = '\0';
    }
    return buffer;
}

void salvar_memoria_grafo(Grafo *grafo, int direcionado, const char *nome_arquivo_saida) {
    FILE *arquivo = fopen(nome_arquivo_saida, "w");
    if (!arquivo) {
        printf("Erro ao abrir o arquivo de memoria.\n");
        return;
    }

    long long memoria_bytes = 0;

    if (grafo->tipo_representacao == LISTA_ADJACENCIA) {
        memoria_bytes += (long long)grafo->num_vertices * sizeof(NoListaAdj *);
        long long total_nos = direcionado ? (long long)grafo->num_arestas
                                          : (long long)grafo->num_arestas * 2;
        memoria_bytes += total_nos * sizeof(NoListaAdj);
    } else {
        memoria_bytes += (long long)grafo->num_vertices * sizeof(float *);
        memoria_bytes += (long long)grafo->num_vertices * (long long)grafo->num_vertices * sizeof(float);
    }
    memoria_bytes += (long long)grafo->num_vertices * sizeof(int);
    memoria_bytes += sizeof(Grafo);

    double memoria_kb = memoria_bytes / 1024.0;
    double memoria_mb = memoria_kb / 1024.0;

    fprintf(arquivo, "Memoria estimada para a representacao do grafo (%s):\n",
            direcionado ? "Direcionado" : "Nao-Direcionado");
    fprintf(arquivo, "Bytes: %lld B\n", memoria_bytes);
    fprintf(arquivo, "Kilobytes: %.2f KB\n", memoria_kb);
    fprintf(arquivo, "Megabytes: %.2f MB\n", memoria_mb);

    fclose(arquivo);
}

void mostrar_menu(void) {
    printf("\n=== MENU DE OPCOES ===\n");
    printf("1. Salvar informacoes basicas do grafo\n");
    printf("2. Realizar Busca em Largura (BFS)\n");
    printf("3. Realizar Busca em Profundidade (DFS)\n");
    printf("4. Calcular distancia entre dois vertices (sem pesos)\n");
    printf("5. Calcular diametro do grafo (sem pesos)\n");
    printf("6. Encontrar componentes conexas\n");
    printf("7. Calcular memoria da representacao\n");
    printf("8. Benchmark de tempo BFS (100 vertices)\n");
    printf("9. Benchmark de tempo DFS (100 vertices)\n");
    printf("10. Caminho Minimo (Dijkstra - Todos os destinos a partir de uma origem)\n");
    printf("11. Caminho Minimo (Dijkstra - Entre dois vertices especificos)\n");
    printf("12. Caminho Minimo (Dijkstra - Destinos {10, 20, 30, 40, 50} a partir de 1)\n");
    printf("13. Caminho Minimo (Dijkstra SEM HEAP - Destinos {10, 20, 30, 40, 50} a partir de 1)\n");
    printf("14. Benchmark Dijkstra (Com Heap vs Sem Heap)\n");
    printf("--- TRABALHO PRATICO 3 (BELLMAN-FORD & FLOYD-WARSHALL) ---\n");
    printf("15. [TP3 - Caso 1] Bellman-Ford: origens {10, 20, 30} -> destino 100\n");
    printf("16. [TP3 - Caso 2] Floyd-Warshall: pares entre {10, 20, 30, 40, 50}\n");
    printf("17. [TP3 - Caso 3] Tempo medio (10x): Bellman-Ford vs Floyd-Warshall (destino 100)\n");
    printf("18. [TP3 - Caso 4] Tempo medio (10x): Todos os pares (Bellman-Ford vs Floyd-Warshall)\n");
    printf("19. [TP3 - COMPLETO] Executar todos os Estudos de Caso (1 a 4) em sequencia\n");
    printf("0. Sair\n");
    printf("Escolha uma opcao: ");
}

void imprimir_caminho_vetor_1_indexado(const int *pais, int origem_idx, int destino_idx, int V, FILE *saida) {
    if (destino_idx == origem_idx) {
        fprintf(saida, "%d", origem_idx + 1);
        return;
    }
    if (pais[destino_idx] == -1) {
        fprintf(saida, "Inalcancavel");
        return;
    }

    int *caminho = (int *)malloc((size_t)V * sizeof(int));
    if (!caminho) {
        fprintf(saida, "Erro de memoria");
        return;
    }

    int count = 0;
    int atual = destino_idx;
    while (atual != -1 && count < V) {
        caminho[count++] = atual;
        atual = pais[atual];
    }

    for (int i = count - 1; i >= 0; i--) {
        fprintf(saida, "%d", caminho[i] + 1);
        if (i > 0) fprintf(saida, " -> ");
    }
    free(caminho);
}

void imprimir_caminho_invertido_para_destino(const int *pais_inv, int origem_idx, int destino_idx, int V, FILE *saida) {
    if (origem_idx == destino_idx) {
        fprintf(saida, "%d", origem_idx + 1);
        return;
    }
    if (pais_inv[origem_idx] == -1) {
        fprintf(saida, "Inalcancavel");
        return;
    }

    int atual = origem_idx;
    int passos = 0;
    while (atual != -1 && passos < V) {
        fprintf(saida, "%d", atual + 1);
        if (atual == destino_idx) break;
        fprintf(saida, " -> ");
        atual = pais_inv[atual];
        passos++;
    }
}

/* ============================================================================
 * UTILITARIOS DOS ESTUDOS DE CASO (memoria disponivel, calibracao do Floyd-Warshall)
 * ============================================================================ */

/* Memoria realmente disponivel (MemAvailable de /proc/meminfo; funciona no WSL/Linux).
 * Retorna 0 se nao for possivel descobrir. Necessario porque malloc de varios GB "funciona" e o
 * processo so e morto pelo sistema (OOM) quando as paginas sao tocadas na inicializacao. */
static double memoria_disponivel_bytes(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0.0;
    char linha[256];
    double kb = 0.0;
    while (fgets(linha, sizeof(linha), f)) {
        if (sscanf(linha, "MemAvailable: %lf kB", &kb) == 1) break;
    }
    fclose(f);
    return kb * 1024.0;
}

/* 1 se 'bytes' cabe na memoria disponivel (com 8% de folga) ou se nao da para saber. */
static int cabe_na_memoria(double bytes, double *disponivel_gb) {
    double disp = memoria_disponivel_bytes();
    if (disponivel_gb) *disponivel_gb = disp / (1024.0 * 1024.0 * 1024.0);
    return (disp <= 0.0) || (bytes <= disp * 0.92);
}

static double para_gb(double bytes) { return bytes / (1024.0 * 1024.0 * 1024.0); }

static int max_vertices_floyd(void) {
    const char *env = getenv("TP3_MAX_V_FLOYD");
    if (env) {
        int v = atoi(env);
        if (v > 0) return v;
    }
    return 30000;
}

/* Orcamento de tempo (s) para repetir o Floyd-Warshall no Caso 3. Pode ser mudado sem recompilar:
 *   TP3_ORCAMENTO_FW=3600 ./programa */
static double orcamento_fw_segundos(void) {
    const char *env = getenv("TP3_ORCAMENTO_FW");
    if (env) {
        double v = atof(env);
        if (v > 0.0) return v;
    }
    return ORCAMENTO_FW_PADRAO_S;
}

/* --- Calibracao do Floyd-Warshall (feita UMA vez por execucao e reaproveitada nos Casos 2, 3 e 4,
 *     para que o mesmo grafo tenha sempre a mesma estimativa) ---
 * Em V grande o Floyd-Warshall e limitado pela LARGURA DE BANDA da memoria: cada passo k percorre as
 * duas matrizes inteiras. Extrapolar de grafos pequenos (que cabem no cache) subestima o tempo em
 * varias vezes (medido: ~5x). Por isso a calibracao cronometra alguns passos k numa matriz GRANDE
 * (V_cal), com o mesmo padrao de acesso a memoria da execucao real, e escala:
 *     tempo(V) = V * (tempo_por_passo em V_cal) * (V / V_cal)^2 .
 * A matriz de calibracao e densa (nenhuma linha e pulada por dik = INF), logo a estimativa nao
 * subestima o trabalho de grafos que viram densos rapidamente (todos os grafos do trabalho). */
#define PASSOS_CALIBRACAO 24
#define V_CALIBRACAO_MAX 8000
typedef struct {
    int pronto;
    int v_cal;
    int passos;
    double seg_por_passo;
} CalibracaoFW;
static CalibracaoFW g_calib = {0, 0, 0, 0.0};

static double medir_passos_fw(int v_cal, int passos) {
    float *d = alocar_matriz_distancias_1d(v_cal);
    int *p = alocar_matriz_predecessores_1d(v_cal);
    double melhor = -1.0;
    if (d && p) {
        unsigned long long x = 88172645463325252ULL;
        size_t total = (size_t)v_cal * (size_t)v_cal;
        for (size_t i = 0; i < total; i++) {
            x ^= x << 13; x ^= x >> 7; x ^= x << 17;
            d[i] = 1.0f + (float)(x % 5000ULL) / 100.0f; /* pesos em [1, 51): todos finitos */
            p[i] = (int)(i / (size_t)v_cal);
        }
        for (int rep = 0; rep < 2; rep++) {
            double t0 = obter_tempo_segundos();
            floyd_warshall_passos(v_cal, d, p, 0, passos);
            double dt = obter_tempo_segundos() - t0;
            if (melhor < 0.0 || dt < melhor) melhor = dt; /* menor tempo = menos ruido do SO */
        }
    }
    free(d);
    free(p);
    return melhor;
}

static const CalibracaoFW *calibrar_fw(int V) {
    if (g_calib.pronto) return &g_calib;
    int v_cal = (V < V_CALIBRACAO_MAX) ? V : V_CALIBRACAO_MAX;
    /* se a RAM nao comporta a matriz de calibracao (8 * v_cal^2 bytes), reduz */
    double disp_gb;
    while (v_cal > 1000 && !cabe_na_memoria(8.0 * (double)v_cal * (double)v_cal, &disp_gb)) v_cal /= 2;
    int passos = (PASSOS_CALIBRACAO < v_cal) ? PASSOS_CALIBRACAO : v_cal;
    printf("Calibrando o Floyd-Warshall (%d passos k em matriz %d x %d)...\n", passos, v_cal, v_cal);
    double t = medir_passos_fw(v_cal, passos);
    g_calib.v_cal = v_cal;
    g_calib.passos = passos;
    g_calib.seg_por_passo = (t > 0.0) ? t / passos : 0.0;
    g_calib.pronto = 1;
    printf("  %.5f s por passo k  (%.2f G atualizacoes/s)\n", g_calib.seg_por_passo,
           (g_calib.seg_por_passo > 0.0) ? (double)v_cal * v_cal / g_calib.seg_por_passo / 1e9 : 0.0);
    return &g_calib;
}

/* Tempo estimado (s) de 1 rodada do Floyd-Warshall em V vertices. */
static double estimar_fw_segundos(int V) {
    const CalibracaoFW *c = calibrar_fw(V);
    if (c->seg_por_passo <= 0.0) return -1.0;
    double r = (double)V / (double)c->v_cal;
    return (double)V * c->seg_por_passo * r * r;
}

static void descrever_calibracao(FILE *arq) {
    if (!g_calib.pronto) return;
    fprintf(arq, "   Calibracao: %d passos k numa matriz %d x %d = %.5f s por passo (%.2f G atualizacoes/s).\n",
            g_calib.passos, g_calib.v_cal, g_calib.v_cal, g_calib.seg_por_passo,
            (g_calib.seg_por_passo > 0.0) ? (double)g_calib.v_cal * g_calib.v_cal / g_calib.seg_por_passo / 1e9 : 0.0);
    fprintf(arq, "   Estimativa = V * t_passo * (V/V_cal)^2. Assume o mesmo regime de memoria da calibracao (matrizes fora do cache).\n");
}

/* ============================================================================
 * ESTUDO DE CASO 1: Bellman-Ford (origens {10, 20, 30} -> destino 100)
 * ============================================================================ */
void executar_tp3_caso1(Grafo *grafo, const char *nome_base) {
    garantir_diretorio_resultados();
    char caminho_saida[512];
    snprintf(caminho_saida, sizeof(caminho_saida), "%s/%s_tp3_caso1_bellman_ford.txt", PASTA_RESULTADOS, nome_base);

    FILE *arq = fopen(caminho_saida, "w");
    if (!arq) {
        printf("Erro ao criar arquivo de saida '%s'.\n", caminho_saida);
        return;
    }

    int V = grafo->num_vertices;
    /* Enunciado (Parte 3, Estudo de Caso 1): distancia dos vertices 10, 20 e 30 ate o vertice 100. */
    int origens[] = {10, 20, 30};
    int num_origens = (int)(sizeof(origens) / sizeof(origens[0]));
    int destino = 100;

    printf("\n=== ESTUDO DE CASO 1: BELLMAN-FORD (Origens -> %d) ===\n", destino);
    fprintf(arq, "=== ESTUDO DE CASO 1: BELLMAN-FORD ===\n");
    fprintf(arq, "Grafo: %s (V = %d, E = %d)\n", nome_base, V, grafo->num_arestas);
    fprintf(arq, "Origens: {10, 20, 30} | Destino: %d\n\n", destino);

    if (V < 100) {
        printf("Aviso: O grafo possui apenas %d vertices (menor que o destino 100).\n", V);
        fprintf(arq, "Grafo possui apenas %d vertices; vertice 100 nao existe.\n", V);
        fclose(arq);
        return;
    }

    float *distancias = (float *)malloc((size_t)V * sizeof(float));
    int *pais = (int *)malloc((size_t)V * sizeof(int));
    int *arestas_caminho = (int *)malloc((size_t)V * sizeof(int));
    Fila *fila = criar_fila(V);

    double t_csr_ini = obter_tempo_segundos();
    GrafoCSR *csr = construir_grafo_transposto_csr(grafo);
    double tempo_csr = obter_tempo_segundos() - t_csr_ini;

    if (!distancias || !pais || !arestas_caminho || !fila || !csr) {
        printf("Erro ao alocar memoria para Bellman-Ford.\n");
        fprintf(arq, "Erro: memoria insuficiente.\n");
        free(distancias); free(pais); free(arestas_caminho);
        destruir_fila(fila); liberar_grafo_csr(csr);
        fclose(arq);
        return;
    }

    double t_ini = obter_tempo_segundos();
    bool sem_ciclo_negativo = bellman_ford_csr(csr, destino, distancias, pais, fila, arestas_caminho);
    double tempo_bf = obter_tempo_segundos() - t_ini;

    fprintf(arq, "Tempo construcao do grafo transposto (CSR): %.6f s\n", tempo_csr);

    if (!sem_ciclo_negativo) {
        printf("Resultado: CICLO NEGATIVO DETECTADO! Nao existem caminhos minimos bem definidos.\n");
        fprintf(arq, "Status: CICLO NEGATIVO DETECTADO (alcancavel no grafo transposto a partir de %d)!\n", destino);
        fprintf(arq, "Tempo de execucao (Bellman-Ford ate detectar o ciclo): %.6f s\n", tempo_bf);
        fprintf(arq, "Obs.: com ciclo negativo que alcanca o destino, a distancia nao esta definida para as origens que o alcancam.\n");
        fprintf(arq, "      O tempo acima e o de DETECCAO, nao o de calcular distancias.\n");
    } else {
        printf("Status: Concluido sem ciclo negativo (CSR: %.6f s | Bellman-Ford: %.6f s)\n\n", tempo_csr, tempo_bf);
        fprintf(arq, "Status: Concluido sem ciclo negativo\n");
        fprintf(arq, "Tempo Bellman-Ford (todos-para-%d via grafo transposto CSR): %.6f s\n\n", destino, tempo_bf);
        fprintf(arq, "%-10s | %-10s | %-15s | %s\n", "Origem", "Destino", "Distancia", "Caminho Minimo");
        fprintf(arq, "-----------+------------+-----------------+------------------------------------------\n");

        for (int i = 0; i < num_origens; i++) {
            int u = origens[i];
            int u_idx = u - 1;
            int dest_idx = destino - 1;

            if (distancias[u_idx] == FLT_MAX) {
                printf("Origem %2d -> Destino %d: INF (Inalcancavel)\n", u, destino);
                fprintf(arq, "%-10d | %-10d | %-15s | Inalcancavel\n", u, destino, "INF");
            } else {
                printf("Origem %2d -> Destino %d: %.2f | Caminho: ", u, destino, distancias[u_idx]);
                imprimir_caminho_invertido_para_destino(pais, u_idx, dest_idx, V, stdout);
                printf("\n");

                fprintf(arq, "%-10d | %-10d | %-15.2f | ", u, destino, distancias[u_idx]);
                imprimir_caminho_invertido_para_destino(pais, u_idx, dest_idx, V, arq);
                fprintf(arq, "\n");
            }
        }
    }

    free(distancias);
    free(pais);
    free(arestas_caminho);
    destruir_fila(fila);
    liberar_grafo_csr(csr);
    fclose(arq);
    printf("Resultados salvos em: %s\n", caminho_saida);
}

/* ============================================================================
 * ESTUDO DE CASO 2: Floyd-Warshall (pares entre {10, 20, 30, 40, 50})
 * ============================================================================ */
void executar_tp3_caso2(Grafo *grafo, const char *nome_base) {
    garantir_diretorio_resultados();
    char caminho_saida[512];
    snprintf(caminho_saida, sizeof(caminho_saida), "%s/%s_tp3_caso2_floyd_warshall.txt", PASTA_RESULTADOS, nome_base);

    FILE *arq = fopen(caminho_saida, "w");
    if (!arq) {
        printf("Erro ao criar arquivo de saida '%s'.\n", caminho_saida);
        return;
    }

    int V = grafo->num_vertices;
    int vertices_alvo[] = {10, 20, 30, 40, 50};
    int k_alvo = 5;

    printf("\n=== ESTUDO DE CASO 2: FLOYD-WARSHALL (Pares entre {10, 20, 30, 40, 50}) ===\n");
    fprintf(arq, "=== ESTUDO DE CASO 2: FLOYD-WARSHALL ===\n");
    fprintf(arq, "Grafo: %s (V = %d, E = %d)\n", nome_base, V, grafo->num_arestas);

    if (V < 50) {
        printf("Aviso: O grafo possui apenas %d vertices (menor que 50).\n", V);
        fprintf(arq, "Grafo possui apenas %d vertices; conjunto {10..50} incompleto.\n", V);
        fclose(arq);
        return;
    }

    /* 2 matrizes V x V (float + int) = 8 * V^2 bytes */
    double bytes_fw = (double)V * (double)V * (double)(sizeof(float) + sizeof(int));
    double gb_necessario = para_gb(bytes_fw);
    double gb_disp = 0.0;
    int excede_limite = (V > MAX_VERTICES_FLOYD);
    int sem_memoria = !excede_limite && !cabe_na_memoria(bytes_fw, &gb_disp);

    if (excede_limite || sem_memoria) {
        double est = estimar_fw_segundos(V);
        if (excede_limite) {
            printf("Status: Nao executado (V = %d excede MAX_VERTICES_FLOYD = %d; requer %.2f GB para 2 matrizes = 8*V^2 bytes).\n",
                   V, MAX_VERTICES_FLOYD, gb_necessario);
            fprintf(arq, "Status: Nao executado\n");
            fprintf(arq, "Motivo: V = %d excede o limite pratico MAX_VERTICES_FLOYD = %d (requer %.2f GB de RAM para 2 matrizes V x V = 8*V^2 bytes).\n",
                    V, MAX_VERTICES_FLOYD, gb_necessario);
        } else {
            printf("Status: Nao executado (requer %.2f GB e ha apenas %.2f GB de RAM disponivel).\n", gb_necessario, gb_disp);
            fprintf(arq, "Status: Nao executado\n");
            fprintf(arq, "Motivo: memoria insuficiente (requer %.2f GB para 2 matrizes V x V = 8*V^2 bytes; disponivel: %.2f GB).\n",
                    gb_necessario, gb_disp);
        }
        if (est > 0.0) {
            fprintf(arq, "Tempo estimado (1 rodada, extrapolacao cubica, NAO e medicao): ~%.2f s (%.2f h)\n", est, est / 3600.0);
            descrever_calibracao(arq);
            printf("Tempo estimado (1 rodada, extrapolacao): ~%.2f s (%.2f h)\n", est, est / 3600.0);
        }
        fclose(arq);
        return;
    }

    if (V > MAX_VERTICES_CASO4) {
        double est = estimar_fw_segundos(V);
        if (est > 0.0)
            printf("Aviso: o Floyd-Warshall em V = %d deve levar cerca de %.1f min (estimativa, limite inferior). Executando...\n",
                   V, est / 60.0);
    }

    float *dist = alocar_matriz_distancias_1d(V);
    int *pred = alocar_matriz_predecessores_1d(V);
    if (!dist || !pred) {
        printf("Erro: Memoria insuficiente para matrizes %d x %d do Floyd-Warshall.\n", V, V);
        fprintf(arq, "Erro: Memoria insuficiente para matrizes %d x %d.\n", V, V);
        free(dist);
        free(pred);
        fclose(arq);
        return;
    }

    double t_ini = obter_tempo_segundos();
    bool sem_ciclo_negativo = floyd_warshall(grafo, dist, pred);
    double tempo_exec = obter_tempo_segundos() - t_ini;

    if (!sem_ciclo_negativo) {
        printf("Resultado: CICLO NEGATIVO DETECTADO! Nao existem caminhos minimos bem definidos.\n");
        fprintf(arq, "Status: CICLO NEGATIVO DETECTADO!\n");
        fprintf(arq, "Tempo de execucao (ate detectar o ciclo): %.6f s\n", tempo_exec);
    } else {
        printf("Status: Concluido sem ciclo negativo (tempo: %.6f s)\n\n", tempo_exec);
        fprintf(arq, "Status: Concluido sem ciclo negativo\n");
        fprintf(arq, "Tempo de execucao: %.6f s\n\n", tempo_exec);

        fprintf(arq, "Tabela de Distancias Minimas (Origem x Destino):\n");
        fprintf(arq, "%-10s", "O \\ D");
        printf("%-10s", "O \\ D");
        for (int j = 0; j < k_alvo; j++) {
            fprintf(arq, " | %-12d", vertices_alvo[j]);
            printf(" | %-12d", vertices_alvo[j]);
        }
        fprintf(arq, "\n-----------+--------------+--------------+--------------+--------------+--------------\n");
        printf("\n-----------+--------------+--------------+--------------+--------------+--------------\n");

        for (int i = 0; i < k_alvo; i++) {
            int u = vertices_alvo[i];
            fprintf(arq, "%-10d", u);
            printf("%-10d", u);
            for (int j = 0; j < k_alvo; j++) {
                int v = vertices_alvo[j];
                float d = dist[(size_t)(u - 1) * (size_t)V + (size_t)(v - 1)];
                if (d == FLT_MAX) {
                    fprintf(arq, " | %-12s", "INF");
                    printf(" | %-12s", "INF");
                } else {
                    fprintf(arq, " | %-12.2f", d);
                    printf(" | %-12.2f", d);
                }
            }
            fprintf(arq, "\n");
            printf("\n");
        }

        fprintf(arq, "\nDetalhamento dos Caminhos Minimos:\n");
        for (int i = 0; i < k_alvo; i++) {
            for (int j = 0; j < k_alvo; j++) {
                int u = vertices_alvo[i];
                int v = vertices_alvo[j];
                size_t idx = (size_t)(u - 1) * (size_t)V + (size_t)(v - 1);
                float d = dist[idx];
                if (d == FLT_MAX) {
                    fprintf(arq, "%d -> %d: INF (Inalcancavel)\n", u, v);
                } else {
                    fprintf(arq, "%d -> %d (custo %.2f): ", u, v, d);
                    imprimir_caminho_vetor_1_indexado(&pred[(size_t)(u - 1) * (size_t)V], u - 1, v - 1, V, arq);
                    fprintf(arq, "\n");
                }
            }
        }
    }

    free(dist);
    free(pred);
    fclose(arq);
    printf("Resultados salvos em: %s\n", caminho_saida);
}

/* ============================================================================
 * ESTUDO DE CASO 3: Tempo medio (10 execucoes) - Caso 1 vs Caso 2
 * ============================================================================ */
void executar_tp3_caso3(Grafo *grafo, const char *nome_base) {
    garantir_diretorio_resultados();
    char caminho_saida[512];
    snprintf(caminho_saida, sizeof(caminho_saida), "%s/%s_tp3_caso3_tempo_destino100.txt", PASTA_RESULTADOS, nome_base);

    FILE *arq = fopen(caminho_saida, "w");
    if (!arq) {
        printf("Erro ao criar arquivo de saida '%s'.\n", caminho_saida);
        return;
    }

    int V = grafo->num_vertices;
    printf("\n=== ESTUDO DE CASO 3: TEMPO MEDIO (10 EXECUCOES) - DESTINO 100 ===\n");
    fprintf(arq, "=== ESTUDO DE CASO 3: TEMPO MEDIO DE EXECUCAO (10 RODADAS) ===\n");
    fprintf(arq, "Grafo: %s (V = %d, E = %d)\n", nome_base, V, grafo->num_arestas);
    fprintf(arq, "Nota: Tempos medidos sem leitura/escrita em disco (apenas processamento).\n\n");

    if (V < 100) {
        printf("Aviso: V = %d < 100. Destino 100 nao existe neste grafo.\n", V);
        fprintf(arq, "Nao executado: V = %d < 100.\n", V);
        fclose(arq);
        return;
    }

    /* 1. Benchmark Bellman-Ford (10 execucoes rumo ao destino 100 via CSR) */
    float *dist_bf = (float *)malloc((size_t)V * sizeof(float));
    int *pais_bf = (int *)malloc((size_t)V * sizeof(int));
    Fila *fila_bf = criar_fila(V);
    int *arestas_bf = (int *)malloc((size_t)V * sizeof(int));

    double soma_bf = 0.0, soma_csr = 0.0;
    bool bf_ok = true; /* grafo estatico: a existencia de ciclo negativo e a mesma em todas as rodadas */
    int rodadas_bf = 0;

    if (dist_bf && pais_bf && fila_bf && arestas_bf) {
        /* Cada rodada refaz a montagem do grafo transposto (CSR) e roda o Bellman-Ford. Os dois tempos
         * sao somados separadamente para o relatorio poder informar o algoritmo puro e o custo total
         * (transposicao + Bellman-Ford). Nenhum deles inclui leitura/escrita em disco. */
        for (int r = 0; r < NUM_EXECUCOES_BENCHMARK; r++) {
            double t_c0 = obter_tempo_segundos();
            GrafoCSR *csr = construir_grafo_transposto_csr(grafo);
            double t_c = obter_tempo_segundos() - t_c0;
            if (!csr) break;
            double t0 = obter_tempo_segundos();
            bf_ok = bellman_ford_csr(csr, 100, dist_bf, pais_bf, fila_bf, arestas_bf);
            soma_bf += (obter_tempo_segundos() - t0);
            soma_csr += t_c;
            rodadas_bf++;
            liberar_grafo_csr(csr);
        }
    }
    if (rodadas_bf > 0) {
        double media_bf = soma_bf / rodadas_bf;
        double media_csr = soma_csr / rodadas_bf;
        printf("Bellman-Ford (%d execucoes -> destino 100): Media apenas BF = %.6f s | Media transposicao CSR = %.6f s | Media total = %.6f s [%s]\n",
               rodadas_bf, media_bf, media_csr, media_bf + media_csr, bf_ok ? "Sem ciclo negativo" : "Ciclo negativo");
        fprintf(arq, "1. Bellman-Ford (todos-para-100 via grafo transposto CSR):\n");
        fprintf(arq, "   - Status: %s\n", bf_ok ? "Sem ciclo negativo" : "Ciclo negativo detectado (tempo = tempo de deteccao)");
        fprintf(arq, "   - Rodadas: %d\n", rodadas_bf);
        fprintf(arq, "   - Tempo medio por rodada, apenas Bellman-Ford:            %.6f s\n", media_bf);
        fprintf(arq, "   - Tempo medio por rodada, montagem do grafo transposto:   %.6f s\n", media_csr);
        fprintf(arq, "   - Tempo medio por rodada, TOTAL (transposicao + BF):      %.6f s\n\n", media_bf + media_csr);
    } else {
        printf("Bellman-Ford: memoria insuficiente.\n");
        fprintf(arq, "1. Bellman-Ford: Nao executado (memoria insuficiente).\n\n");
    }
    destruir_fila(fila_bf);
    free(arestas_bf);
    free(dist_bf);
    free(pais_bf);

    /* 2. Benchmark Floyd-Warshall (ate 10 execucoes, limitadas por um orcamento de tempo) */
    double bytes_fw = (double)V * (double)V * (double)(sizeof(float) + sizeof(int)); /* 8*V^2 */
    double gb_necessario = para_gb(bytes_fw);
    double gb_disp = 0.0;
    int excede_limite = (V > MAX_VERTICES_FLOYD);
    int sem_memoria = !excede_limite && !cabe_na_memoria(bytes_fw, &gb_disp);

    if (excede_limite || sem_memoria) {
        double est = estimar_fw_segundos(V);
        const char *motivo = excede_limite ? "excede MAX_VERTICES_FLOYD" : "memoria insuficiente";
        printf("Floyd-Warshall: Nao executado (%s; requer %.2f GB = 8*V^2 bytes).\n", motivo, gb_necessario);
        fprintf(arq, "2. Floyd-Warshall:\n");
        fprintf(arq, "   - Status: Nao executado (V = %d: %s; requer %.2f GB de RAM = 8*V^2 bytes%s).\n", V, motivo, gb_necessario,
                excede_limite ? "" : "; disponivel abaixo do necessario");
        if (est > 0.0) {
            fprintf(arq, "   - Tempo estimado (1 rodada, extrapolacao cubica, NAO e medicao): ~%.2f s (%.2f h); 10 rodadas: ~%.2f h\n",
                    est, est / 3600.0, 10.0 * est / 3600.0);
            descrever_calibracao(arq);
        }
    } else {
        float *dist_fw = alocar_matriz_distancias_1d(V);
        int *pred_fw = alocar_matriz_predecessores_1d(V);
        if (!dist_fw || !pred_fw) {
            printf("Floyd-Warshall: Memoria insuficiente para matrizes %d x %d.\n", V, V);
            fprintf(arq, "2. Floyd-Warshall: Memoria insuficiente.\n");
        } else {
            double orcamento = orcamento_fw_segundos();
            double soma_fw = 0.0;
            int rodadas = 0;
            bool fw_ok = true;
            for (int r = 0; r < NUM_EXECUCOES_BENCHMARK; r++) {
                double t0 = obter_tempo_segundos();
                fw_ok = floyd_warshall(grafo, dist_fw, pred_fw);
                double dt = obter_tempo_segundos() - t0;
                soma_fw += dt;
                rodadas++;
                printf("  Floyd-Warshall, rodada %d/%d: %.3f s\n", rodadas, NUM_EXECUCOES_BENCHMARK, dt);
                fflush(stdout);
                /* nao comeca outra rodada se a proxima estourar o orcamento de tempo */
                if (soma_fw + dt > orcamento && rodadas < NUM_EXECUCOES_BENCHMARK) break;
            }
            double media_fw = soma_fw / rodadas;
            printf("Floyd-Warshall (%d execucoes): Total = %.6f s | Media = %.6f s [%s]\n",
                   rodadas, soma_fw, media_fw, fw_ok ? "Sem ciclo negativo" : "Ciclo negativo");
            fprintf(arq, "2. Floyd-Warshall (todos os pares):\n");
            fprintf(arq, "   - Status: %s\n", fw_ok ? "Sem ciclo negativo" : "Ciclo negativo detectado (tempo = tempo de deteccao)");
            fprintf(arq, "   - Execucoes realizadas: %d de %d%s\n", rodadas, NUM_EXECUCOES_BENCHMARK,
                    rodadas < NUM_EXECUCOES_BENCHMARK ? " (limitado pelo orcamento de tempo TP3_ORCAMENTO_FW)" : "");
            fprintf(arq, "   - Tempo total (%d execucoes): %.6f s\n", rodadas, soma_fw);
            fprintf(arq, "   - Tempo medio por execucao: %.6f s\n", media_fw);
        }
        free(dist_fw);
        free(pred_fw);
    }

    fclose(arq);
    printf("Resultados salvos em: %s\n", caminho_saida);
}

/* ============================================================================
 * ESTUDO DE CASO 4: Tempo medio (10 execucoes) - Todos os pares
 * ============================================================================ */
void executar_tp3_caso4(Grafo *grafo, const char *nome_base) {
    garantir_diretorio_resultados();
    char caminho_saida[512];
    snprintf(caminho_saida, sizeof(caminho_saida), "%s/%s_tp3_caso4_todos_pares.txt", PASTA_RESULTADOS, nome_base);

    FILE *arq = fopen(caminho_saida, "w");
    if (!arq) {
        printf("Erro ao criar arquivo de saida '%s'.\n", caminho_saida);
        return;
    }

    int V = grafo->num_vertices;
    printf("\n=== ESTUDO DE CASO 4: TODOS OS PARES (10 EXECUCOES) ===\n");
    fprintf(arq, "=== ESTUDO DE CASO 4: TODOS OS PARES DE VERTICES (10 EXECUCOES) ===\n");
    fprintf(arq, "Grafo: %s (V = %d, E = %d)\n\n", nome_base, V, grafo->num_arestas);

    /* Pico de RAM no Caso 4: 3 matrizes simultaneas V x V (dist_bf + dist_fw + pred_fw)
     * = 12 * V^2 bytes (pred_bf e liberado antes de alocar o Floyd-Warshall); se as 4 coexistissem,
     * seriam 16 * V^2. Alem disso o tempo cubico se repete por 10 rodadas. */
    double bytes_pico = (double)V * (double)V * (double)(2 * sizeof(float) + sizeof(int));
    double gb_pico = para_gb(bytes_pico);
    double gb_disp = 0.0;
    int excede_limite = (V > MAX_VERTICES_CASO4);
    int sem_memoria = !excede_limite && !cabe_na_memoria(bytes_pico, &gb_disp);

    if (excede_limite || sem_memoria) {
        double gb_4_matrizes = para_gb(4.0 * 4.0 * (double)V * (double)V);

        if (excede_limite) {
            printf("Status: Nao executado (V = %d excede o limite pratico MAX_VERTICES_CASO4 = %d).\n", V, MAX_VERTICES_CASO4);
            fprintf(arq, "Status: Nao executado\n");
            fprintf(arq, "Motivo: V = %d excede o limite pratico MAX_VERTICES_CASO4 = %d.\n", V, MAX_VERTICES_CASO4);
        } else {
            printf("Status: Nao executado (requer %.2f GB e ha apenas %.2f GB de RAM disponivel).\n", gb_pico, gb_disp);
            fprintf(arq, "Status: Nao executado\n");
            fprintf(arq, "Motivo: memoria insuficiente (pico %.2f GB; disponivel %.2f GB).\n", gb_pico, gb_disp);
        }
        printf("Memoria necessaria: %.2f GB de pico (3 matrizes = 12*V^2 bytes; %.2f GB se 4 matrizes = 16*V^2 bytes).\n",
               gb_pico, gb_4_matrizes);
        fprintf(arq, "Memoria necessaria: %.2f GB de pico (3 matrizes simultaneas = 12*V^2 bytes; %.2f GB para 4 matrizes = 16*V^2 bytes).\n\n",
                gb_pico, gb_4_matrizes);

        /* 1. Bellman-Ford todos os pares: media de 10 execucoes reais 1-para-todos no grafo completo */
        GrafoCSR *csr = construir_grafo_csr(grafo);
        float *d_sample = (float *)malloc((size_t)V * sizeof(float));
        int *p_sample = (int *)malloc((size_t)V * sizeof(int));
        Fila *f_sample = criar_fila(V);
        int *a_sample = (int *)malloc((size_t)V * sizeof(int));

        double t_bf_medio_1_origem = 0.0;
        bool bf_amostra_ok = true;
        if (csr && d_sample && p_sample && f_sample && a_sample) {
            for (int i = 0; i < NUM_EXECUCOES_BENCHMARK; i++) {
                int origem_amostra = 1 + (int)(((long long)i * V) / NUM_EXECUCOES_BENCHMARK);
                double t0 = obter_tempo_segundos();
                bool ok = bellman_ford_csr(csr, origem_amostra, d_sample, p_sample, f_sample, a_sample);
                t_bf_medio_1_origem += (obter_tempo_segundos() - t0);
                if (!ok) bf_amostra_ok = false;
            }
            t_bf_medio_1_origem /= NUM_EXECUCOES_BENCHMARK;
        }
        destruir_fila(f_sample);
        free(a_sample);
        free(d_sample);
        free(p_sample);
        liberar_grafo_csr(csr);

        double est_bf_1x = t_bf_medio_1_origem * (double)V;
        double est_bf_10x = est_bf_1x * NUM_EXECUCOES_BENCHMARK;

        /* 2. Floyd-Warshall: calibracao compartilhada (500/1000/2000) e extrapolacao cubica de V_sub = 2000 */
        double est_fw_1x = estimar_fw_segundos(V);
        double est_fw_10x = est_fw_1x * NUM_EXECUCOES_BENCHMARK;

        printf("Estimativa de tempo por extrapolacao empirica nesta maquina (NAO e medicao direta do Caso 4 completo):\n");
        printf("  - Floyd-Warshall (extrapolacao cubica): ~%.2f s (%.2f h) por rodada | ~%.2f s (%.2f h) em 10 rodadas\n",
               est_fw_1x, est_fw_1x / 3600.0, est_fw_10x, est_fw_10x / 3600.0);
        if (bf_amostra_ok) {
            printf("  - Bellman-Ford todos os pares (10 origens reais x V): ~%.2f s (%.2f h) por rodada | ~%.2f s (%.2f h) em 10 rodadas\n",
                   est_bf_1x, est_bf_1x / 3600.0, est_bf_10x, est_bf_10x / 3600.0);
        } else {
            printf("  - Bellman-Ford todos os pares: nao aplicavel (ciclo negativo detectado na amostra).\n");
        }

        fprintf(arq, "Estimativa de tempo por extrapolacao empirica nesta maquina:\n");
        fprintf(arq, "AVISO: Os valores abaixo sao extrapolacoes matematicas calibradas nesta maquina, e NAO medicoes diretas do grafo completo.\n\n");
        fprintf(arq, "1. Floyd-Warshall (extrapolacao cubica Theta(V^3)):\n");
        descrever_calibracao(arq);
        fprintf(arq, "   - Tempo estimado (1 rodada): %.2f s (%.2f min / %.2f h)\n", est_fw_1x, est_fw_1x / 60.0, est_fw_1x / 3600.0);
        fprintf(arq, "   - Tempo estimado (10 rodadas): %.2f s (%.2f min / %.2f h)\n\n", est_fw_10x, est_fw_10x / 60.0, est_fw_10x / 3600.0);

        fprintf(arq, "2. Bellman-Ford para todos os pares (extrapolacao V * t_medio_1_origem a partir de 10 origens reais no grafo completo):\n");
        if (bf_amostra_ok) {
            fprintf(arq, "   - Tempo medio real (1 origem no grafo completo): %.6f s (sem ciclo negativo)\n", t_bf_medio_1_origem);
            fprintf(arq, "   - Tempo estimado (1 rodada = V execucoes): %.2f s (%.2f min / %.2f h)\n",
                    est_bf_1x, est_bf_1x / 60.0, est_bf_1x / 3600.0);
            fprintf(arq, "   - Tempo estimado (10 rodadas): %.2f s (%.2f min / %.2f h)\n",
                    est_bf_10x, est_bf_10x / 60.0, est_bf_10x / 3600.0);
        } else {
            fprintf(arq, "   - NAO APLICAVEL: ha ciclo negativo alcancavel; as distancias de todos os pares nao estao definidas.\n");
            fprintf(arq, "     (o tempo medido por origem, %.6f s, e o de DETECCAO do ciclo e nao serve de base para estimar V execucoes completas)\n",
                    t_bf_medio_1_origem);
        }

        fclose(arq);
        printf("Resultados salvos em: %s\n", caminho_saida);
        return;
    }

    /* 1. Bellman-Ford executado a partir de todos os V vertices (10 rodadas completas) */
    float *dist_bf = alocar_matriz_distancias_1d(V);
    int *pred_bf = alocar_matriz_predecessores_1d(V);
    if (!dist_bf || !pred_bf) {
        printf("Erro: Memoria insuficiente para matrizes %d x %d (%.2f GB necessarios).\n", V, V, gb_pico);
        fprintf(arq, "Status: Nao executado (memoria insuficiente para matrizes %d x %d = %.2f GB).\n", V, V, gb_pico);
        free(dist_bf);
        free(pred_bf);
        fclose(arq);
        return;
    }

    double soma_bf = 0.0;
    bool bf_ok = true;
    for (int r = 0; r < NUM_EXECUCOES_BENCHMARK; r++) {
        double t0 = obter_tempo_segundos();
        bf_ok = bellman_ford_todos_pares(grafo, dist_bf, pred_bf);
        soma_bf += (obter_tempo_segundos() - t0);
    }
    double media_bf = soma_bf / NUM_EXECUCOES_BENCHMARK;

    /* Libera pred_bf antes de alocar Floyd-Warshall, reduzindo o pico de RAM de 16*V^2 para 12*V^2 bytes */
    free(pred_bf);
    pred_bf = NULL;

    /* 2. Floyd-Warshall executado 10 vezes */
    float *dist_fw = alocar_matriz_distancias_1d(V);
    int *pred_fw = alocar_matriz_predecessores_1d(V);
    if (!dist_fw || !pred_fw) {
        printf("Erro: Memoria insuficiente para matrizes do Floyd-Warshall %d x %d (%.2f GB de pico).\n", V, V, gb_pico);
        fprintf(arq, "Status: Nao executado (memoria insuficiente para Floyd-Warshall %d x %d = %.2f GB).\n", V, V, gb_pico);
        free(dist_bf);
        free(dist_fw);
        free(pred_fw);
        fclose(arq);
        return;
    }

    double soma_fw = 0.0;
    bool fw_ok = true;
    for (int r = 0; r < NUM_EXECUCOES_BENCHMARK; r++) {
        double t0 = obter_tempo_segundos();
        fw_ok = floyd_warshall(grafo, dist_fw, pred_fw);
        soma_fw += (obter_tempo_segundos() - t0);
    }
    double media_fw = soma_fw / NUM_EXECUCOES_BENCHMARK;

    printf("Bellman-Ford (V execucoes x 10 rodadas): Total = %.6f s | Media = %.6f s [%s]\n",
           soma_bf, media_bf, bf_ok ? "Sem ciclo negativo" : "Ciclo negativo");
    printf("Floyd-Warshall (10 rodadas):             Total = %.6f s | Media = %.6f s [%s]\n",
           soma_fw, media_fw, fw_ok ? "Sem ciclo negativo" : "Ciclo negativo");

    fprintf(arq, "1. Bellman-Ford (executado V = %d vezes por rodada; inclui a montagem do CSR em cada rodada):\n", V);
    fprintf(arq, "   - Status: %s\n", bf_ok ? "Sem ciclo negativo" : "Ciclo negativo detectado (tempo = tempo de deteccao)");
    fprintf(arq, "   - Tempo total (10 rodadas): %.6f s\n", soma_bf);
    fprintf(arq, "   - Tempo medio por rodada: %.6f s\n\n", media_bf);

    fprintf(arq, "2. Floyd-Warshall:\n");
    fprintf(arq, "   - Status: %s\n", fw_ok ? "Sem ciclo negativo" : "Ciclo negativo detectado (tempo = tempo de deteccao)");
    fprintf(arq, "   - Tempo total (10 rodadas): %.6f s\n", soma_fw);
    fprintf(arq, "   - Tempo medio por rodada: %.6f s\n\n", media_fw);

    if (bf_ok && fw_ok) {
        int divergencias = 0;
        size_t total_pares = (size_t)V * (size_t)V;
        for (size_t idx = 0; idx < total_pares; idx++) {
            float d1 = dist_bf[idx];
            float d2 = dist_fw[idx];
            if (d1 == FLT_MAX && d2 == FLT_MAX) continue;
            float diff = (d1 > d2) ? (d1 - d2) : (d2 - d1);
            if (diff > 1e-3f) divergencias++;
        }
        printf("Verificacao de consistencia entre algoritmos: %d divergencias em %zu pares.\n", divergencias, total_pares);
        fprintf(arq, "Verificacao de consistencia (Bellman-Ford vs Floyd-Warshall): %d divergencias em %zu pares.\n",
                divergencias, total_pares);
    } else if (bf_ok != fw_ok) {
        printf("ATENCAO: os algoritmos discordam sobre a existencia de ciclo negativo (BF: %s | FW: %s).\n",
               bf_ok ? "nao" : "sim", fw_ok ? "nao" : "sim");
        fprintf(arq, "ATENCAO: os algoritmos discordam sobre a existencia de ciclo negativo (BF: %s | FW: %s).\n",
                bf_ok ? "nao" : "sim", fw_ok ? "nao" : "sim");
    }

    free(dist_bf);
    free(dist_fw);
    free(pred_fw);
    fclose(arq);
    printf("Resultados salvos em: %s\n", caminho_saida);
}

int main(void) {
    int opcao_rep, direcionado;
    char nome_arquivo[256];
    char nome_base[256];

    printf("Escolha a representacao do grafo (1 - Lista de Adjacencia, 2 - Matriz de Adjacencia): ");
    if (scanf("%d", &opcao_rep) != 1) return 1;

    RepresentacaoGrafo tipo_rep = (opcao_rep == 1) ? LISTA_ADJACENCIA : MATRIZ_ADJACENCIA;

    printf("O grafo e direcionado? (1 - Sim, 0 - Nao): ");
    if (scanf("%d", &direcionado) != 1) return 1;

    printf("Digite o nome do arquivo do grafo: ");
    if (scanf("%255s", nome_arquivo) != 1) return 1;

    Grafo *grafo = ler_grafo_do_arquivo(nome_arquivo, tipo_rep, direcionado);
    if (!grafo) {
        printf("Erro ao carregar o grafo!\n");
        return 1;
    }

    if (!direcionado && possui_arestas_negativas(grafo)) {
        printf("AVISO: o grafo foi carregado como NAO-direcionado e tem aresta de peso negativo; nesse caso\n"
               "       cada aresta negativa u-v forma o ciclo negativo u->v->u. Para os estudos de caso do TP3\n"
               "       (grafos direcionados) responda 1 em 'O grafo e direcionado?'.\n");
    }

    extrair_nome_base(nome_arquivo, nome_base, sizeof(nome_base));
    garantir_diretorio_resultados();

    int opcao;
    do {
        mostrar_menu();
        if (scanf("%d", &opcao) != 1) break;

        switch (opcao) {
            case 1: {
                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_informacoes_grafo.txt", PASTA_RESULTADOS, nome_base);
                salvar_informacoes_grafo(grafo, arquivo_saida);
                printf("Informacoes basicas salvas em '%s'.\n", arquivo_saida);
                break;
            }
            case 2: {
                int vertice_inicial;
                printf("Digite o vertice inicial para BFS: ");
                if (scanf("%d", &vertice_inicial) != 1) {
                    printf("Entrada invalida.\n");
                    break;
                }

                int *pais = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                int *niveis = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                busca_em_largura(grafo, vertice_inicial, pais, niveis);

                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_bfs_resultado.txt", PASTA_RESULTADOS, nome_base);
                FILE *arquivo = fopen(arquivo_saida, "w");
                if (arquivo) {
                    fprintf(arquivo, "Resultado da BFS a partir do vertice %d:\n", vertice_inicial);
                    for (int i = 0; i < grafo->num_vertices; i++) {
                        fprintf(arquivo, "Vertice %d: Pai = %d, Nivel = %d\n", i + 1, pais[i] != -1 ? pais[i] + 1 : -1, niveis[i]);
                    }
                    fclose(arquivo);
                }

                free(pais);
                free(niveis);
                printf("Resultado da BFS salvo em '%s'.\n", arquivo_saida);
                break;
            }
            case 3: {
                int vertice_inicial;
                printf("Digite o vertice inicial para DFS: ");
                if (scanf("%d", &vertice_inicial) != 1) {
                    printf("Entrada invalida.\n");
                    break;
                }

                int *pais = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                int *niveis = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                busca_em_profundidade(grafo, vertice_inicial, pais, niveis);

                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_dfs_resultado.txt", PASTA_RESULTADOS, nome_base);
                FILE *arquivo = fopen(arquivo_saida, "w");
                if (arquivo) {
                    fprintf(arquivo, "Resultado da DFS a partir do vertice %d:\n", vertice_inicial);
                    for (int i = 0; i < grafo->num_vertices; i++) {
                        fprintf(arquivo, "Vertice %d: Pai = %d, Nivel = %d\n", i + 1, pais[i] != -1 ? pais[i] + 1 : -1, niveis[i]);
                    }
                    fclose(arquivo);
                }

                free(pais);
                free(niveis);
                printf("Resultado da DFS salvo em '%s'.\n", arquivo_saida);
                break;
            }
            case 4: {
                int origem, destino;
                printf("Digite os vertices de origem e destino: ");
                if (scanf("%d %d", &origem, &destino) != 2) {
                    printf("Entrada invalida.\n");
                    break;
                }
                int distancia = calcular_distancia(grafo, origem, destino);
                if (distancia != -1) {
                    printf("A distancia entre %d e %d e: %d\n", origem, destino, distancia);
                } else {
                    printf("Nao ha caminho entre %d e %d.\n", origem, destino);
                }
                break;
            }
            case 5: {
                int diametro = calcular_diametro(grafo);
                printf("O diametro do grafo e: %d\n", diametro);
                break;
            }
            case 6: {
                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_componentes_conexas.txt", PASTA_RESULTADOS, nome_base);
                encontrar_componentes_conexas(grafo, arquivo_saida);
                printf("Componentes conexas salvas em '%s'.\n", arquivo_saida);
                break;
            }
            case 7: {
                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_memoria_grafo.txt", PASTA_RESULTADOS, nome_base);
                salvar_memoria_grafo(grafo, direcionado, arquivo_saida);
                printf("Memoria utilizada pela representacao salva em '%s'.\n", arquivo_saida);
                break;
            }
            case 8: {
                int *pais = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                int *niveis = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                double t0 = obter_tempo_segundos();
                srand((unsigned int)time(NULL));
                for (int i = 0; i < 100; i++) {
                    int vertice_inicial = (rand() % grafo->num_vertices) + 1;
                    busca_em_largura(grafo, vertice_inicial, pais, niveis);
                }
                double tempo_total = obter_tempo_segundos() - t0;
                printf("Tempo medio de 100 execucoes de BFS: %f segundos\n", tempo_total / 100.0);
                free(pais);
                free(niveis);
                break;
            }
            case 9: {
                int *pais = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                int *niveis = (int *)malloc((size_t)grafo->num_vertices * sizeof(int));
                double t0 = obter_tempo_segundos();
                srand((unsigned int)time(NULL));
                for (int i = 0; i < 100; i++) {
                    int vertice_inicial = (rand() % grafo->num_vertices) + 1;
                    busca_em_profundidade(grafo, vertice_inicial, pais, niveis);
                }
                double tempo_total = obter_tempo_segundos() - t0;
                printf("Tempo medio de 100 execucoes de DFS: %f segundos\n", tempo_total / 100.0);
                free(pais);
                free(niveis);
                break;
            }
            case 10: {
                if (possui_arestas_negativas(grafo)) {
                    printf("Erro: O grafo possui arestas com pesos negativos. Dijkstra nao e aplicavel.\n");
                    break;
                }
                int vertice_inicial;
                printf("Digite o vertice de origem (1 a %d): ", grafo->num_vertices);
                if (scanf("%d", &vertice_inicial) != 1) {
                    printf("Entrada invalida.\n");
                    break;
                }

                if (vertice_inicial < 1 || vertice_inicial > grafo->num_vertices) {
                    printf("Vertice invalido.\n");
                    break;
                }

                int V = grafo->num_vertices;
                float *distancias = (float *)malloc((size_t)V * sizeof(float));
                int *pais = (int *)malloc((size_t)V * sizeof(int));

                dijkstra(grafo, vertice_inicial, distancias, pais);

                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_dijkstra_todos.txt", PASTA_RESULTADOS, nome_base);
                FILE *arquivo = fopen(arquivo_saida, "w");
                if (arquivo) {
                    fprintf(arquivo, "Resultados de Dijkstra a partir do vertice %d:\n\n", vertice_inicial);
                    for (int i = 0; i < V; i++) {
                        fprintf(arquivo, "Destino %d: ", i + 1);
                        if (distancias[i] == FLT_MAX) {
                            fprintf(arquivo, "Distancia = INFINITO | Caminho = Nenhum\n");
                        } else {
                            fprintf(arquivo, "Distancia = %.2f | Caminho = ", distancias[i]);
                            salvar_caminho_minimo(grafo, pais, vertice_inicial - 1, i, V, arquivo);
                        }
                    }
                    fclose(arquivo);
                    printf("Resultados salvos em '%s'.\n", arquivo_saida);
                }
                free(distancias);
                free(pais);
                break;
            }
            case 11: {
                if (possui_arestas_negativas(grafo)) {
                    printf("Erro: O grafo possui arestas com pesos negativos. Dijkstra nao e aplicavel.\n");
                    break;
                }
                int origem, destino;
                printf("Digite o vertice de origem (1 a %d): ", grafo->num_vertices);
                if (scanf("%d", &origem) != 1) {
                    printf("Entrada invalida.\n");
                    break;
                }
                printf("Digite o vertice de destino (1 a %d): ", grafo->num_vertices);
                if (scanf("%d", &destino) != 1) {
                    printf("Entrada invalida.\n");
                    break;
                }

                if (origem < 1 || origem > grafo->num_vertices || destino < 1 || destino > grafo->num_vertices) {
                    printf("Vertices invalidos.\n");
                    break;
                }

                int V = grafo->num_vertices;
                float *distancias = (float *)malloc((size_t)V * sizeof(float));
                int *pais = (int *)malloc((size_t)V * sizeof(int));

                dijkstra(grafo, origem, distancias, pais);

                int dest_idx = destino - 1;
                if (distancias[dest_idx] == FLT_MAX) {
                    printf("Nao existe caminho entre %d e %d.\n", origem, destino);
                } else {
                    printf("\nDistancia minima de %d para %d: %.2f\n", origem, destino, distancias[dest_idx]);
                    printf("Caminho: ");
                    salvar_caminho_minimo(grafo, pais, origem - 1, dest_idx, V, stdout);
                }
                free(distancias);
                free(pais);
                break;
            }
            case 12: {
                if (possui_arestas_negativas(grafo)) {
                    printf("Erro: O grafo possui arestas com pesos negativos. Dijkstra nao e aplicavel.\n");
                    break;
                }

                int V = grafo->num_vertices;
                int origem = 1;
                int destinos[] = {10, 20, 30, 40, 50};
                int num_destinos = 5;

                float *distancias = (float *)malloc((size_t)V * sizeof(float));
                int *pais = (int *)malloc((size_t)V * sizeof(int));

                dijkstra(grafo, origem, distancias, pais);

                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_dijkstra_alvos.txt", PASTA_RESULTADOS, nome_base);
                FILE *arquivo = fopen(arquivo_saida, "w");
                if (arquivo) {
                    fprintf(arquivo, "Caminhos minimos a partir do vertice %d (COM HEAP):\n\n", origem);
                    for (int i = 0; i < num_destinos; i++) {
                        int d = destinos[i];
                        if (d > V) continue;
                        int d_idx = d - 1;
                        fprintf(arquivo, "--- Destino %d ---\n", d);
                        if (distancias[d_idx] == FLT_MAX) {
                            fprintf(arquivo, "Distancia: INFINITO\nCaminho: Inalcancavel\n\n");
                        } else {
                            fprintf(arquivo, "Distancia: %.2f\nCaminho: ", distancias[d_idx]);
                            salvar_caminho_minimo(grafo, pais, origem - 1, d_idx, V, arquivo);
                            fprintf(arquivo, "\n");
                        }
                    }
                    fclose(arquivo);
                    printf("Resultados salvos em '%s'.\n", arquivo_saida);
                }
                free(distancias);
                free(pais);
                break;
            }
            case 13: {
                if (possui_arestas_negativas(grafo)) {
                    printf("Erro: O grafo possui arestas com pesos negativos. Dijkstra nao e aplicavel.\n");
                    break;
                }

                int V = grafo->num_vertices;
                int origem = 1;
                int destinos[] = {10, 20, 30, 40, 50};
                int num_destinos = 5;

                float *distancias = (float *)malloc((size_t)V * sizeof(float));
                int *pais = (int *)malloc((size_t)V * sizeof(int));

                dijkstra_sem_heap(grafo, origem, distancias, pais);

                char arquivo_saida[512];
                snprintf(arquivo_saida, sizeof(arquivo_saida), "%s/%s_dijkstra_sem_heap_alvos.txt", PASTA_RESULTADOS, nome_base);
                FILE *arquivo = fopen(arquivo_saida, "w");
                if (arquivo) {
                    fprintf(arquivo, "Caminhos minimos a partir do vertice %d (SEM HEAP):\n\n", origem);
                    for (int i = 0; i < num_destinos; i++) {
                        int d = destinos[i];
                        if (d > V) continue;
                        int d_idx = d - 1;
                        fprintf(arquivo, "--- Destino %d ---\n", d);
                        if (distancias[d_idx] == FLT_MAX) {
                            fprintf(arquivo, "Distancia: INFINITO\nCaminho: Inalcancavel\n\n");
                        } else {
                            fprintf(arquivo, "Distancia: %.2f\nCaminho: ", distancias[d_idx]);
                            salvar_caminho_minimo(grafo, pais, origem - 1, d_idx, V, arquivo);
                            fprintf(arquivo, "\n");
                        }
                    }
                    fclose(arquivo);
                    printf("Resultados salvos em '%s'.\n", arquivo_saida);
                }
                free(distancias);
                free(pais);
                break;
            }
            case 14: {
                if (possui_arestas_negativas(grafo)) {
                    printf("Erro: O grafo possui arestas com pesos negativos.\n");
                    break;
                }

                int k;
                printf("Digite o numero de vertices aleatorios (k) para o benchmark: ");
                if (scanf("%d", &k) != 1 || k <= 0) {
                    printf("Numero invalido.\n");
                    break;
                }

                int V = grafo->num_vertices;
                int *amostra = (int *)malloc((size_t)k * sizeof(int));
                srand((unsigned int)time(NULL));
                for (int i = 0; i < k; i++) {
                    amostra[i] = (rand() % V) + 1;
                }

                float *distancias = (float *)malloc((size_t)V * sizeof(float));
                int *pais = (int *)malloc((size_t)V * sizeof(int));

                double t0 = obter_tempo_segundos();
                for (int i = 0; i < k; i++) {
                    dijkstra(grafo, amostra[i], distancias, pais);
                }
                double tempo_com_heap = obter_tempo_segundos() - t0;

                t0 = obter_tempo_segundos();
                for (int i = 0; i < k; i++) {
                    dijkstra_sem_heap(grafo, amostra[i], distancias, pais);
                }
                double tempo_sem_heap = obter_tempo_segundos() - t0;

                printf("\n=== RESULTADO DO BENCHMARK (%d execucoes) ===\n", k);
                printf("Com Heap (MinHeap): Total = %.4fs | Medio = %.6fs\n", tempo_com_heap, tempo_com_heap / k);
                printf("Sem Heap (Vetor):   Total = %.4fs | Medio = %.6fs\n", tempo_sem_heap, tempo_sem_heap / k);
                if (tempo_com_heap > 0) {
                    printf("Speedup (Sem Heap / Com Heap): %.2fx\n", tempo_sem_heap / tempo_com_heap);
                }

                free(amostra);
                free(distancias);
                free(pais);
                break;
            }
            case 15:
                executar_tp3_caso1(grafo, nome_base);
                break;
            case 16:
                executar_tp3_caso2(grafo, nome_base);
                break;
            case 17:
                executar_tp3_caso3(grafo, nome_base);
                break;
            case 18:
                executar_tp3_caso4(grafo, nome_base);
                break;
            case 19:
                executar_tp3_caso1(grafo, nome_base);
                executar_tp3_caso2(grafo, nome_base);
                executar_tp3_caso3(grafo, nome_base);
                executar_tp3_caso4(grafo, nome_base);
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opcao invalida!\n");
                break;
        }
    } while (opcao != 0);

    liberar_grafo(grafo);
    return 0;
}
