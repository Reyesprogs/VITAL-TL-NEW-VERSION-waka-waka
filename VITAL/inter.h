#ifndef INTERM
#define INTERM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

#define PERMISOS 0666
#define MUTEX 0
#define CLIENTE_LISTO 1
#define SERVIDOR_LISTO 2
#define HILO_LECTOR 3 

// La estructura de la memoria
typedef struct {
    char mensaje[512];
} MemoriaCompartida;

// El enchufe universal para los semaforos
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

// Funcion para bloquear (restar 1)
int down(int semid, int sem_num) {
    struct sembuf op_p = {sem_num, -1, 0};
    return semop(semid, &op_p, 1);
}

// Funcion para avanzar (sumar 1)
int up(int semid, int sem_num) {
    struct sembuf op_v = {sem_num, 1, 0};
    return semop(semid, &op_v, 1);
}

#endif
