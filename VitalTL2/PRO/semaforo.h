#ifndef SEMAFORO_H
#define SEMAFORO_H

#include <semaphore.h>

void iniciar_semaforo(sem_t *sem, int valor);
void bloquear(sem_t *sem);
void desbloquear(sem_t *sem);
void borrar_semaforo(sem_t *sem);

#endif
