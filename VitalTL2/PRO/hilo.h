#ifndef HILO_H
#define HILO_H

#include <pthread.h>
#include <sys/types.h>
#include "datos.h"

typedef struct {
    pid_t pid_cliente;           
    int shmid_privado;          
    ConexionCliente *datos;      
} InfoClienteHilo;

void iniciar_hilo(pthread_t *hilo, Analisis *datos);

void iniciar_hilo_cliente(pthread_t *hilo, InfoClienteHilo *info_cliente);

#endif
