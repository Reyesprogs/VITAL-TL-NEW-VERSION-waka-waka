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
cliente: cliente.c inter.h validaciones.c validaciones.h ui.c ui.h panel.c panel.h catalogo.c catalogo.h carrito.c carrito.h
	$(CC) $(CFLAGS) -o cliente cliente.c validaciones.c ui.c panel.c catalogo.c carrito.c $(LDFLAGS)

# --- REGLA DE LIMPIEZA ---
clean:
	rm -f servidor cliente 
