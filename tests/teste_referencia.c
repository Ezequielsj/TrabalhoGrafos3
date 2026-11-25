/* Teste de referencia: compara a biblioteca com Bellman-Ford exato em inteiros (centavos). */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "biblioteca_grafos.h"

#define MAXV 64
typedef struct { int u, v; long long w; } E;

/* referencia exata: retorna 1 se ha ciclo negativo alcancavel de s; d[] em centavos, INF = LLONG_MAX/4 */
static const long long INFL = (long long)4e18 / 4;
static int ref_bf(int V, E *e, int m, int s, long long *d) {
  for (int i = 0; i < V; i++) d[i] = INFL;
  d[s] = 0;
  for (int it = 0; it < V; it++) {
    int ch = 0;
    for (int k = 0; k < m; k++)
      if (d[e[k].u] < INFL && d[e[k].u] + e[k].w < d[e[k].v]) { d[e[k].v] = d[e[k].u] + e[k].w; ch = 1; }
    if (!ch) return 0;
    if (it == V - 1) return 1;
  }
  return 0;
}

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 25) { printf("FALHA: "); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static double soma_caminho(Grafo *g, const int *pai, int s, int t) {
  /* soma pesos do caminho s->t usando pai[] (pai[v] = anterior de v) */
  double tot = 0; int cur = t, guard = 0;
  while (cur != s) { int p = pai[cur]; if (p < 0 || guard++ > g->num_vertices) return NAN; tot += get_peso_aresta(g, p, cur); cur = p; }
  return tot;
}

int main(int argc, char **argv) {
  int iters = argc > 1 ? atoi(argv[1]) : 3000;
  srand(12345);
  long n_neg = 0, n_pos = 0, n_zero_sum_graphs = 0;
  for (int it = 0; it < iters; it++) {
    int V = 2 + rand() % 30;
    int modo = it % 4;  /* 0: so positivos, 1: poucos negativos, 2: muitos negativos, 3: pares de soma zero */
    E e[400]; int m = 0;
    int target = rand() % (V * 3 + 1);
    for (int k = 0; k < target && m < 390; k++) {
      int u = rand() % V, v = rand() % V;
      long long w;
      if (modo == 0) w = rand() % 1000;
      else if (modo == 1) w = (rand() % 12 == 0) ? -(rand() % 200) : rand() % 1000;
      else if (modo == 2) w = (rand() % 1000) - 250;
      else { w = (rand() % 1000) - 100; }
      e[m++] = (E){u, v, w};
      if (modo == 3 && m < 390 && rand() % 3 == 0) { e[m++] = (E){v, u, -w}; n_zero_sum_graphs++; } /* ciclo de soma exatamente zero */
    }
    Grafo *g = criar_grafo_lista_adj(V);
    Grafo *gm = criar_grafo_matriz_adj(V);
    int tem_multi = 0;
    for (int k = 0; k < m; k++) {
      adicionar_aresta(g, e[k].u, e[k].v, (float)e[k].w / 100.0f, 1);
      if (!isnan(gm->matriz_adj[e[k].u][e[k].v])) tem_multi = 1;
      /* matriz guarda so a ultima aresta paralela: mantem a menor para ser equivalente */
      float w = (float)e[k].w / 100.0f;
      if (isnan(gm->matriz_adj[e[k].u][e[k].v]) || w < gm->matriz_adj[e[k].u][e[k].v]) gm->matriz_adj[e[k].u][e[k].v] = w;
    }
    (void)tem_multi;

    /* referencia: ha ciclo negativo em algum lugar? */
    int ciclo_global = 0; long long dref[MAXV][MAXV]; int ciclo_de[MAXV];
    for (int s = 0; s < V; s++) { ciclo_de[s] = ref_bf(V, e, m, s, dref[s]); if (ciclo_de[s]) ciclo_global = 1; }
    if (ciclo_global) n_neg++; else n_pos++;

    /* 1) Bellman-Ford 1-para-todos (lista e matriz) */
    for (int s = 0; s < V; s++) {
      float d[MAXV]; int p[MAXV];
      int ok = bellman_ford_otimizado(g, s + 1, d, p);
      CHECK(ok == !ciclo_de[s], "BF lista it=%d s=%d ok=%d ref_ciclo=%d (modo %d)", it, s, ok, ciclo_de[s], modo);
      if (ok && !ciclo_de[s]) {
        for (int v = 0; v < V; v++) {
          if (dref[s][v] >= INFL) { CHECK(d[v] == FLT_MAX, "BF inalcancavel it=%d", it); continue; }
          CHECK(fabs(d[v] - dref[s][v] / 100.0) < 1e-3, "BF dist it=%d s=%d v=%d %.4f vs %.4f", it, s, v, d[v], dref[s][v] / 100.0);
          if (v != s) { double sp = soma_caminho(g, p, s, v);
            if (!(fabs(sp - d[v]) < 1e-3) && fails == 0) {
              int cur = v, st = 0; printf("DETALHE it=%d modo=%d s=%d v=%d dist=%.6f soma=%.6f cadeia:", it, modo, s, v, d[v], sp);
              while (cur != -1 && st++ < 12) { printf(" %d(%.6f)", cur, d[cur]); if (cur == s) break; cur = p[cur]; } printf("\n"); }
            CHECK(fabs(sp - d[v]) < 1e-3, "BF caminho it=%d s=%d v=%d", it, s, v); }
        }
      }
      float d2[MAXV]; int p2[MAXV];
      int ok2 = bellman_ford_otimizado(gm, s + 1, d2, p2);
      CHECK(ok2 == ok, "BF matriz != lista it=%d", it);
    }

    /* 2) todos-para-um via transposto: usa destino t */
    int t = rand() % V;
    {
      float d[MAXV]; int p[MAXV];
      int ok = bellman_ford_para_destino(g, t + 1, d, p);
      /* ha ciclo negativo que alcanca t ? = existe s com ciclo e dist(s->t) finito...
         aproximacao exata: ciclo negativo em algum vertice x com x alcanca t */
      int esperado_ciclo = 0;
      for (int x = 0; x < V; x++) if (ciclo_de[x] && dref[x][t] < INFL) esperado_ciclo = 1;
      /* ciclo_de[x]==1 so diz que x alcanca algum ciclo; precisamos de ciclo que alcance t:
         se x alcanca ciclo C e x alcanca t isso nao implica C alcanca t. Refinamento: */
      esperado_ciclo = 0;
      for (int c = 0; c < V && !esperado_ciclo; c++) {
        /* c esta em ciclo negativo? d[c][c] < 0 apos V passes: use FW exato */
        long long dc[MAXV]; (void)dc;
      }
      if (!ciclo_global) {
        CHECK(ok == 1, "destino: falso positivo it=%d", it);
        if (ok) for (int s = 0; s < V; s++) {
          if (dref[s][t] >= INFL) CHECK(d[s] == FLT_MAX, "destino inalc it=%d", it);
          else CHECK(fabs(d[s] - dref[s][t] / 100.0) < 1e-3, "destino dist it=%d s=%d t=%d %.4f vs %.4f", it, s, t, d[s], dref[s][t] / 100.0);
        }
      }
    }

    /* 3) Floyd-Warshall e BF todos os pares */
    {
      float *df = alocar_matriz_distancias_1d(V); int *pf = alocar_matriz_predecessores_1d(V);
      int okf = floyd_warshall(g, df, pf);
      CHECK(okf == !ciclo_global, "FW it=%d ok=%d ref_ciclo=%d (modo %d)", it, okf, ciclo_global, modo);
      if (okf && !ciclo_global) {
        for (int s = 0; s < V; s++) for (int v = 0; v < V; v++) {
          float x = df[(size_t)s * V + v];
          if (dref[s][v] >= INFL) CHECK(x == FLT_MAX, "FW inalc it=%d", it);
          else {
            CHECK(fabs(x - dref[s][v] / 100.0) < 1e-3, "FW dist it=%d s=%d v=%d %.4f vs %.4f", it, s, v, x, dref[s][v] / 100.0);
            if (v != s) { double sp = soma_caminho(g, &pf[(size_t)s * V], s, v); CHECK(fabs(sp - x) < 1e-3, "FW caminho it=%d", it); }
          }
        }
      }
      float *db = alocar_matriz_distancias_1d(V); int *pb = alocar_matriz_predecessores_1d(V);
      int okb = bellman_ford_todos_pares(g, db, pb);
      CHECK(okb == !ciclo_global, "BF todos pares it=%d ok=%d ref=%d", it, okb, ciclo_global);
      free(df); free(pf); free(db); free(pb);
      /* FW em matriz */
      float *dm = alocar_matriz_distancias_1d(V); int *pm = alocar_matriz_predecessores_1d(V);
      int okm = floyd_warshall(gm, dm, pm);
      CHECK(okm == okf, "FW matriz != lista it=%d", it);
      free(dm); free(pm);
    }

    /* 4) Dijkstra (so grafos nao negativos) */
    if (modo == 0) {
      for (int s = 0; s < V; s++) {
        float d1[MAXV], d2[MAXV], d3[MAXV]; int p[MAXV];
        dijkstra(g, s + 1, d1, p); dijkstra_sem_heap(g, s + 1, d2, p); dijkstra(gm, s + 1, d3, p);
        for (int v = 0; v < V; v++) {
          if (dref[s][v] >= INFL) { CHECK(d1[v] == FLT_MAX && d2[v] == FLT_MAX && d3[v] == FLT_MAX, "dij inalc it=%d", it); continue; }
          double r = dref[s][v] / 100.0;
          CHECK(fabs(d1[v] - r) < 1e-3 && fabs(d2[v] - r) < 1e-3 && fabs(d3[v] - r) < 1e-3, "dijkstra it=%d s=%d v=%d %.3f %.3f %.3f ref %.3f", it, s, v, d1[v], d2[v], d3[v], r);
        }
      }
    }
    liberar_grafo(g); liberar_grafo(gm);
  }
  printf("grafos com ciclo negativo: %ld | sem: %ld | pares soma-zero gerados: %ld\n", n_neg, n_pos, n_zero_sum_graphs);
  printf(fails ? "RESULTADO: %d FALHAS\n" : "RESULTADO: TODOS OS TESTES PASSARAM (%d falhas)\n", fails);
  return fails ? 1 : 0;
}
