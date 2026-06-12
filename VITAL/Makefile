CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lncurses

all: servidor cliente

servidor: servidor.c inter.h
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c validaciones.c ui.c panel.c catalogo.c carrito.c perfil.c admin.c resultados.c
	gcc -Wall -pthread -o cliente cliente.c validaciones.c ui.c panel.c catalogo.c carrito.c perfil.c admin.c resultados.c -lncurses

clean:
	rm -f servidor cliente 
