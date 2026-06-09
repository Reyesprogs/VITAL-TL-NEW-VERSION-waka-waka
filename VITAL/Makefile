CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lncurses

all: servidor cliente

servidor: servidor.c inter.h
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c inter.h validaciones.c validaciones.h ui.c ui.h panel.c panel.h catalogo.c catalogo.h carrito.c carrito.h resultados.c resultados.h perfil.c perfil.h admin.c admin.h
		$(CC) $(CFLAGS) -o cliente cliente.c validaciones.c ui.c panel.c catalogo.c carrito.c resultados.c perfil.c admin.c $(LDFLAGS)

clean:
	rm -f servidor cliente 
