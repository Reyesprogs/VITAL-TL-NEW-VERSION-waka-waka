#ifndef MEMORIA_GLOBAL_H
#define MEMORIA_GLOBAL_H

#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>

typedef struct
{

    int peticion_activa;
    pid_t pid_cliente;
    char log_mensaje[100];
    int respuesta_servidor;

    int hemoglobina_cant;
    int glucosa_cant;
    int orina_cant;
    int colesterol_cant;
    int trigliceridos_cant;

} analisis;

#endif