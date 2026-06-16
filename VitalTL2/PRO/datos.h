#ifndef DATOS_H
#define DATOS_H

#include <semaphore.h>
#include <sys/types.h> 
#include <unistd.h>    

typedef struct {
    char nombre[50];
    char apellido[50];
    char edad[10];
    char sangre[10];
    char correo[50];
    char usuario[50];
} Paciente;

#define MAX_MUESTRAS 5

typedef struct {
    int hemoglobina[MAX_MUESTRAS];
    int hemoglobina_cant;

    int glucosa[MAX_MUESTRAS];
    int glucosa_cant;

    int orina[MAX_MUESTRAS];
    int orina_cant;

    int colesterol[MAX_MUESTRAS];
    int colesterol_cant;

    int trigliceridos[MAX_MUESTRAS];
    int trigliceridos_cant;

    int peticion_activa;    
    int tipo_peticion;      
    
    char req_usuario[50];
    char req_pass[50];
    char req_nombre[50];
    char req_apellido[50];
    char req_edad[10];
    char req_sangre[10];
    char req_correo[50];

    int respuesta_servidor; 

    
    pid_t pid_cliente;         
    char log_mensaje[100];     
    
    int heartbeat_servidor;    

    sem_t candado; 
    int cargando;
} ConexionCliente;


typedef struct {
    int heartbeat_global;      
    int clientes_activos;      
    sem_t candado_global;      
} MaestroServidor;


typedef ConexionCliente Analisis;

#endif
