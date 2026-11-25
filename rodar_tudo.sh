#!/bin/bash
# Executa TODOS os estudos de caso (1 a 4) em todos os grafos, um apos o outro.
# Uso:
#   ./rodar_tudo.sh                       # grafo_W_1.txt ... grafo_W_5.txt (na pasta atual)
#   ./rodar_tudo.sh a.txt b.txt           # grafos escolhidos
# Variaveis opcionais:
#   TP3_ORCAMENTO_FW=100000 ./rodar_tudo.sh   # deixa o Floyd-Warshall fazer as 10 rodadas inteiras
cd "$(dirname "$0")" || exit 1
make -s || { echo "Falha ao compilar."; exit 1; }

if [ $# -gt 0 ]; then GRAFOS=("$@"); else GRAFOS=(grafo_W_1.txt grafo_W_2.txt grafo_W_3.txt grafo_W_4.txt grafo_W_5.txt); fi
mkdir -p resultados logs

for g in "${GRAFOS[@]}"; do
  if [ ! -f "$g" ]; then echo ">>> $g nao encontrado, pulando."; continue; fi
  base=$(basename "$g" .txt)
  echo "================ $g ($(date +%H:%M:%S)) ================"
  # 1 = lista de adjacencia | 1 = direcionado | arquivo | 19 = todos os casos | 0 = sair
  printf "1\n1\n%s\n19\n0\n" "$g" | ./programa 2>&1 | tee "logs/${base}.log"
done
echo "Pronto. Tabelas em resultados/ e logs completos em logs/."
