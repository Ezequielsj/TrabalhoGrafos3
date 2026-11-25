CC      = gcc
CFLAGS  = -Wall -Wextra -O3 -march=native -g -MMD -MP
LDFLAGS = -lm

SRC    = main.c biblioteca_grafos.c fila.c minheap.c
OBJ    = $(SRC:.c=.o)
DEPS   = $(SRC:.c=.d)
TARGET = programa

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Testes automaticos: compara Bellman-Ford, Floyd-Warshall e Dijkstra contra uma referencia exata
# (inteiros) em milhares de grafos aleatorios, com AddressSanitizer + UBSan.
test: tests/teste_referencia.c biblioteca_grafos.c fila.c minheap.c
	$(CC) -O2 -g -fsanitize=address,undefined -I. -o tests/teste_referencia $^ $(LDFLAGS)
	./tests/teste_referencia 3000

clean:
	rm -f $(OBJ) $(DEPS) $(TARGET) tests/teste_referencia

-include $(DEPS)

.PHONY: all test clean
