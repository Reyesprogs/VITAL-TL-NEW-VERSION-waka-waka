# --- CONFIGURACIÓN ---
CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lncurses

# --- OBJETIVO PRINCIPAL ---
all: servidor cliente

# --- REGLA DEL SERVIDOR ---
servidor: servidor.c inter.h
	$(CC) $(CFLAGS) -o servidor servidor.c

# --- REGLA DEL CLIENTE ---
cliente: cliente.c inter.h validaciones.c validaciones.h
	$(CC) $(CFLAGS) -o cliente cliente.c validaciones.c $(LDFLAGS)

# --- REGLA DE LIMPIEZA ---
clean:
	rm -f servidor cliente historial.txt
