#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "memoria.h"

int crear_memoria(int tamano)
{
    int id = shmget(0x1234, tamano, IPC_CREAT | 0666);
    if (id < 0)
    {
        perror("Error en memoria");
        exit(1);
    }
    return id;
}

void *unir_memoria(int id)
{
    void *dir = shmat(id, NULL, 0);
    if (dir == (void *)-1)
    {
        perror("Error al unir");
        exit(1);
    }
    return dir;
}

void soltar_memoria(void *dir)
{
    shmdt(dir);
}

void borrar_memoria(int id)
{
    shmctl(id, IPC_RMID, NULL);
}

key_t generar_clave_privada(pid_t pid_cliente)
{

    int id_ftok = (pid_cliente % 255) + 100;
    key_t clave = ftok("main.c", id_ftok);

    if (clave == -1)
    {
        perror("Error al generar clave privada con ftok");
        return -1;
    }

    return clave;
}

int crear_memoria_privada(pid_t pid_cliente, int tamano)
{
    key_t clave = generar_clave_privada(pid_cliente);
    if (clave == -1)
        return -1;

    int shmid = shmget(clave, tamano, IPC_CREAT | 0666);

    if (shmid < 0)
    {
        perror("Error al crear memoria privada");
        return -1;
    }

    return shmid;
}

int obtener_memoria_privada(pid_t pid_cliente, int tamano)
{
    key_t clave = generar_clave_privada(pid_cliente);
    if (clave == -1)
        return -1;

    int shmid = shmget(clave, tamano, 0666);

    if (shmid < 0)
    {

        if (errno != ENOENT)
        {
            perror("Error: No se encontró la memoria privada del servidor");
        }
        return -1;
    }

    return shmid;
}

void borrar_memoria_privada(pid_t pid_cliente)
{
    key_t clave = generar_clave_privada(pid_cliente);
    if (clave == -1)
        return;

    int shmid = shmget(clave, 0, 0666);
    if (shmid < 0)
    {

        return;
    }

    shmctl(shmid, IPC_RMID, NULL);
}
