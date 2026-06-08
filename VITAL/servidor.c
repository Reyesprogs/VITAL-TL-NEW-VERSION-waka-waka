#include "inter.h"
#include <signal.h> 

MemoriaCompartida *memoria;
long pos_archivo = 0; 
pthread_mutex_t mutex_archivo = PTHREAD_MUTEX_INITIALIZER; 

int shmid_global, semid_global;

void manejar_cierre(int sig) {
    printf("\n\n[!] Servidor apagado. Limpiando memoria y semaforos...\n");
    shmctl(shmid_global, IPC_RMID, NULL); 
    semctl(semid_global, 0, IPC_RMID);    
    exit(0);
}

void *leer_archivo(void *arg) {
    int semid = *(int*)arg;
    FILE *archivo;
    char linea[512];

    while(1) {
        if(down(semid, HILO_LECTOR) == -1) break; 

        pthread_mutex_lock(&mutex_archivo);
        archivo = fopen("usuarios.txt", "r");
        if (archivo != NULL) {
            fseek(archivo, pos_archivo, SEEK_SET); 
            while (fgets(linea, sizeof(linea), archivo) != NULL) {
                printf("\n[Monitor] Nuevo Registro Guardado:\n  -> %s", linea);
            }
            pos_archivo = ftell(archivo); 
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);
    }
    pthread_exit(NULL);
}

void *atender_cliente(void *arg) {
    int semid = *(int*)arg;
    char variable_interna[512]; 

    strncpy(variable_interna, memoria->mensaje, sizeof(variable_interna));

    if (strcmp(variable_interna, "NUEVA_CONEXION") == 0) {
        printf("\n[Servidor] >>> ALERTA: Un nuevo usuario abrio la aplicacion cliente.\n");
        up(semid, SERVIDOR_LISTO); 
        pthread_exit(NULL);
    }
    if (strcmp(variable_interna, "SALIR") == 0) {
        printf("\n[Hilo %lu] El cliente salio y se desconecto.\n", pthread_self());
        up(semid, SERVIDOR_LISTO); 
        pthread_exit(NULL);
    }

    // --- NUEVO: Validacion de Inicio de Sesion ---
    if (strncmp(variable_interna, "LOGIN | ", 8) == 0) {
        // La variable trae "LOGIN | Usr: reyes | PassXOR: ***"
        // Saltamos los primeros 8 caracteres para buscar solo las credenciales
        char *cadena_busqueda = variable_interna + 8; 
        int encontrado = 0;

        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("usuarios.txt", "r");
        if (archivo != NULL) {
            char linea_arch[1024];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                // strstr busca si la cadena_busqueda existe adentro de la linea
                if (strstr(linea_arch, cadena_busqueda) != NULL) {
                    encontrado = 1; 
                    break;
                }
            }
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);

        // Respondemos al cliente usando la memoria compartida
        if (encontrado) {
            printf("\n[Servidor] Acceso PERMITIDO. Credenciales correctas.\n");
            strncpy(memoria->mensaje, "LOGIN_EXITO", 512);
        } else {
            printf("\n[Servidor] Acceso DENEGADO. Credenciales incorrectas.\n");
            strncpy(memoria->mensaje, "LOGIN_ERROR", 512);
        }
        up(semid, SERVIDOR_LISTO);
        pthread_exit(NULL);
    }

    // Si no fue LOGIN ni conexion, es un REGISTRO nuevo
    printf("\n[Hilo %lu] Atendiendo registro nuevo...\n", pthread_self());

    pthread_mutex_lock(&mutex_archivo);
    FILE *archivo = fopen("usuarios.txt", "a");
    if (archivo != NULL) {
        fprintf(archivo, "%s\n", variable_interna);
        fclose(archivo);
    }
    pthread_mutex_unlock(&mutex_archivo);

    up(semid, HILO_LECTOR);
    up(semid, SERVIDOR_LISTO);

    pthread_exit(NULL);

    // --- NUEVO: Petición para ver el Catálogo ---
        if (strcmp(variable_interna, "CATALOGO_REQ") == 0) {
            printf("\n[Hilo %lu] Enviando catalogo de analisis...\n", pthread_self());
            
            char buffer_catalogo[512] = "";
            pthread_mutex_lock(&mutex_archivo);
            FILE *archivo = fopen("analisis.txt", "r");
            if (archivo != NULL) {
                char linea_arch[100];
                while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                    // Quitamos el salto de linea
                    linea_arch[strcspn(linea_arch, "\n")] = 0; 
                    strcat(buffer_catalogo, linea_arch);
                    strcat(buffer_catalogo, ","); // Separamos cada producto con una coma
                }
                fclose(archivo);
            }
            pthread_mutex_unlock(&mutex_archivo);
    
            // Enviamos todo el catálogo empaquetado al cliente
            strncpy(memoria->mensaje, buffer_catalogo, 512);
            up(semid, SERVIDOR_LISTO);
            pthread_exit(NULL);
        }
    
        // --- NUEVO: Petición para agregar al carrito ---
        if (strncmp(variable_interna, "CARRITO_ADD | ", 14) == 0) {
            printf("\n[Hilo %lu] Agregando item al carrito...\n", pthread_self());
            
            pthread_mutex_lock(&mutex_archivo);
            // Guardamos todo en un archivo general de carritos
            FILE *archivo = fopen("carritos.txt", "a");
            if (archivo != NULL) {
                fprintf(archivo, "%s\n", variable_interna);
                fclose(archivo);
            }
            pthread_mutex_unlock(&mutex_archivo);
    
            strncpy(memoria->mensaje, "AGREGADO_EXITO", 512);
            up(semid, SERVIDOR_LISTO);
            pthread_exit(NULL);
        }
}

int main() {
    pthread_t hilo_atencion, hilo_lector_id;
    key_t llave = ftok("servidor.c", 'K');

    FILE *f = fopen("usuarios.txt", "a"); 
    if(f) fclose(f);
    
    shmid_global = shmget(llave, sizeof(MemoriaCompartida), IPC_CREAT | PERMISOS);
    memoria = (MemoriaCompartida *)shmat(shmid_global, 0, 0);

    semid_global = semget(llave, 4, IPC_CREAT | PERMISOS);
    union semun arg;
    unsigned short valores_iniciales[4] = {1, 0, 0, 0}; 
    arg.array = valores_iniciales;
    semctl(semid_global, 0, SETALL, arg);

    signal(SIGINT, manejar_cierre);

    pthread_create(&hilo_lector_id, NULL, leer_archivo, (void*)&semid_global);
    pthread_detach(hilo_lector_id);

    printf("=== SERVIDOR VITAL-TL INICIADO ===\nEsperando clientes...\n");

    while(1) {
        if(down(semid_global, CLIENTE_LISTO) == -1) break; 
        pthread_create(&hilo_atencion, NULL, atender_cliente, (void*)&semid_global);
        pthread_detach(hilo_atencion); 
    }
    return 0;
}
