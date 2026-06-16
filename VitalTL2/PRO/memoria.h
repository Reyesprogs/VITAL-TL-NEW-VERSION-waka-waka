#ifndef MEMORIA_H
#define MEMORIA_H

#include <sys/types.h>

int crear_memoria(int tamano);
void *unir_memoria(int id);
void soltar_memoria(void *dir);
void borrar_memoria(int id);

key_t generar_clave_privada(pid_t pid_cliente);

int crear_memoria_privada(pid_t pid_cliente, int tamano);

int obtener_memoria_privada(pid_t pid_cliente, int tamano);

void borrar_memoria_privada(pid_t pid_cliente);

#endif
