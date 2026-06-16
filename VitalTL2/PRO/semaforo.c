#include <stdio.h>
#include <stdlib.h>
#include "semaforo.h"

void iniciar_semaforo(sem_t *sem, int valor) {
    if (sem_init(sem, 1, valor) != 0) {
        perror("Error en semaforo");
        exit(1);
    }
}

void bloquear(sem_t *sem) {
    sem_wait(sem);
}

void desbloquear(sem_t *sem) {
    sem_post(sem);
}

void borrar_semaforo(sem_t *sem) {
    sem_destroy(sem);
}
