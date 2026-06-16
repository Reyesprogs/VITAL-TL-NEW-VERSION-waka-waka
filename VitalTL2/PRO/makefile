CC = gcc
CFLAGS = -Wall -Wextra -O2


SERV_TARGET = servidor
CLI_TARGET = cliente


SERV_SRCS = servidor.c semaforo.c validar.c chamcont.c memoria.c hilo.c
CLI_SRCS = main.c chamcont.c crearU.c Isesion.c validar.c \
           pantalla.c compras.c perfil.c panel.c historial.c admin.c semaforo.c memoria.c


SERV_OBJS = $(SERV_SRCS:.c=.o)
CLI_OBJS = $(CLI_SRCS:.c=.o)


HEADERS = chamcont.h datos.h pantalla.h semaforo.h \
          compras.h perfil.h panel.h historial.h admin.h memoria.h hilo.h


all: $(SERV_TARGET) $(CLI_TARGET)


$(SERV_TARGET): $(SERV_OBJS)
	$(CC) $(CFLAGS) -o $(SERV_TARGET) $(SERV_OBJS) -lpthread


$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $(CLI_TARGET) $(CLI_OBJS) -lncurses -lpthread


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(SERV_OBJS) $(CLI_OBJS) $(SERV_TARGET) $(CLI_TARGET)


clean-all: clean
	rm -f /tmp/vital_conexion_fifo

.PHONY: all clean clean-all