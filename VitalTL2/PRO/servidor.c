#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "chamcont.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "datos.h"
#include "semaforo.h"
#include "memoria.h"
#include "hilo.h"
#include <pthread.h>

#define FIFO_CONEXION "/tmp/vital_conexion_fifo"
#define MAX_CLIENTES 10

typedef struct
{
    pid_t pid_cliente;
    int shmid_privado;
    pthread_t hilo_id;
    int activo;
} RegistroCliente;

RegistroCliente clientes_activos[MAX_CLIENTES];
int num_clientes = 0;
int servidor_activo = 1;

void inicializar_clientes()
{
    for (int i = 0; i < MAX_CLIENTES; i++)
    {
        clientes_activos[i].pid_cliente = 0;
        clientes_activos[i].shmid_privado = -1;
        clientes_activos[i].activo = 0;
    }
    num_clientes = 0;
}

int agregar_cliente(pid_t pid_cliente, int shmid_privado, pthread_t hilo_id)
{
    if (num_clientes >= MAX_CLIENTES)
    {
        printf("[ERROR] Límite de clientes alcanzado\n");
        return -1;
    }

    clientes_activos[num_clientes].pid_cliente = pid_cliente;
    clientes_activos[num_clientes].shmid_privado = shmid_privado;
    clientes_activos[num_clientes].hilo_id = hilo_id;
    clientes_activos[num_clientes].activo = 1;

    printf("[SERVIDOR] Cliente agregado (PID=%d)\n", pid_cliente);
    num_clientes++;

    return num_clientes - 1;
}

void remover_cliente(int indice)
{
    if (indice < 0 || indice >= num_clientes)
        return;

    if (clientes_activos[indice].activo)
    {
        printf("[SERVIDOR] Removiendo cliente PID=%d\n", clientes_activos[indice].pid_cliente);

        borrar_memoria_privada(clientes_activos[indice].pid_cliente);

        clientes_activos[indice].activo = 0;
    }
}

void procesar_solicitud_conexion(pid_t pid_cliente)
{
    printf("[SERVIDOR] Solicitud de conexión recibida de cliente PID=%d\n", pid_cliente);

    int shmid_privado = crear_memoria_privada(pid_cliente, sizeof(ConexionCliente));
    if (shmid_privado < 0)
    {
        return;
    }

    ConexionCliente *datos_cliente = (ConexionCliente *)shmat(shmid_privado, NULL, 0);
    if (datos_cliente == (void *)-1)
    {
        printf("[ERROR] No se pudo adjuntar memoria privada\n");
        borrar_memoria(shmid_privado);
        return;
    }

    memset(datos_cliente, 0, sizeof(ConexionCliente));
    datos_cliente->pid_cliente = pid_cliente;
    datos_cliente->heartbeat_servidor = 0;
    datos_cliente->peticion_activa = 0;

    iniciar_semaforo(&datos_cliente->candado, 1);

    InfoClienteHilo *info_hilo = malloc(sizeof(InfoClienteHilo));
    info_hilo->pid_cliente = pid_cliente;
    info_hilo->shmid_privado = shmid_privado;
    info_hilo->datos = datos_cliente;

    pthread_t hilo_cliente;
    iniciar_hilo_cliente(&hilo_cliente, info_hilo);

    agregar_cliente(pid_cliente, shmid_privado, hilo_cliente);

    printf("[SERVIDOR] Cliente PID=%d conectado exitosamente\n", pid_cliente);
    fflush(stdout);
}

void *hilo_aceptador_conexiones(void *arg)
{
    (void)arg;

    unlink(FIFO_CONEXION);
    if (mkfifo(FIFO_CONEXION, 0666) < 0)
    {
        perror("Error al crear FIFO");
        return NULL;
    }

    while (servidor_activo)
    {

        int fd = open(FIFO_CONEXION, O_RDONLY);
        if (fd < 0)
        {
            perror("Error al abrir FIFO");
            continue;
        }

        pid_t pid_cliente;
        if (read(fd, &pid_cliente, sizeof(pid_t)) == sizeof(pid_t))
        {
            procesar_solicitud_conexion(pid_cliente);
        }

        close(fd);
    }

    return NULL;
}

int main()
{
    printf("=== SERVIDOR VITAL-TL INICIADO ===\n");
    printf("Esperando conexiones de clientes...\n\n");

    inicializar_clientes();

    pthread_t hilo_aceptador;
    pthread_create(&hilo_aceptador, NULL, hilo_aceptador_conexiones, NULL);

    while (servidor_activo)
    {

        sleep(1);
    }

    printf("=== SERVIDOR VITAL-TL SE APAGÓ CORRECTAMENTE ===\n");

    for (int i = 0; i < num_clientes; i++)
    {
        remover_cliente(i);
    }

    unlink(FIFO_CONEXION);

    return 0;
}