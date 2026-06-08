#include "inter.h"
#include <signal.h>
#include <sys/syscall.h> 

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
    pid_t id_hilo_real = syscall(SYS_gettid);

    // --- CONEXIONES BASICAS ---
    if (strcmp(variable_interna, "NUEVA_CONEXION") == 0) {
        printf("\n[Servidor] >>> ALERTA: Un nuevo usuario abrio la aplicacion cliente.\n");
        up(semid, SERVIDOR_LISTO); pthread_exit(NULL);
    }
    if (strcmp(variable_interna, "SALIR") == 0) {
        printf("\n[Hilo OS: %d] El cliente salio y se desconecto.\n", id_hilo_real);
        up(semid, SERVIDOR_LISTO); pthread_exit(NULL);
    }

    // --- LOGIN ---
    if (strncmp(variable_interna, "LOGIN | ", 8) == 0) {
        char *cadena_busqueda = variable_interna + 8; 
        int encontrado = 0;
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("usuarios.txt", "r");
        if (archivo != NULL) {
            char linea_arch[1024];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                if (strstr(linea_arch, cadena_busqueda) != NULL) { encontrado = 1; break; }
            }
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);

        if (encontrado) {
            printf("\n[Hilo OS: %d] Acceso PERMITIDO.\n", id_hilo_real);
            strncpy(memoria->mensaje, "LOGIN_EXITO", 512);
        } else {
            printf("\n[Hilo OS: %d] Acceso DENEGADO.\n", id_hilo_real);
            strncpy(memoria->mensaje, "LOGIN_ERROR", 512);
        }
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- ENVIAR CATALOGO ---
    if (strcmp(variable_interna, "CATALOGO_REQ") == 0) {
        char buffer_catalogo[512] = "";
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("analisis.txt", "r");
        if (archivo != NULL) {
            char linea_arch[100];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                linea_arch[strcspn(linea_arch, "\n")] = 0; 
                strcat(buffer_catalogo, linea_arch); strcat(buffer_catalogo, ","); 
            }
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, buffer_catalogo, 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- AGREGAR AL CARRITO ---
    if (strncmp(variable_interna, "CARRITO_ADD | ", 14) == 0) {
        char usuario[50] = {0}; sscanf(variable_interna, "CARRITO_ADD | Usr: %s", usuario);
        printf("\n[Hilo OS: %d] El usuario '%s' agrego un analisis a su carrito.\n", id_hilo_real, usuario);
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "a");
        if (archivo != NULL) { fprintf(archivo, "%s\n", variable_interna); fclose(archivo); }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "AGREGADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- LEER CARRITO DEL USUARIO ---
    if (strncmp(variable_interna, "CARRITO_REQ | ", 14) == 0) {
        char usuario_req[50] = {0}; sscanf(variable_interna, "CARRITO_REQ | Usr: %s", usuario_req);
        char buffer_carrito[512] = ""; 
        char busqueda[200]; // Aumentado a 200 para evitar warning
        snprintf(busqueda, 200, "Usr: %s | Prod: ", usuario_req);

        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "r");
        if (archivo != NULL) {
            char linea_arch[200];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                if (strstr(linea_arch, busqueda) != NULL) {
                    char prod[50], prc[20];
                    char *ptr_prod = strstr(linea_arch, "Prod: ") + 6;
                    char *ptr_prc = strstr(linea_arch, "Prc: ") + 5;
                    sscanf(ptr_prod, "%[^ |]", prod); sscanf(ptr_prc, "%s", prc);
                    char temp[100]; snprintf(temp, 100, "%s|%s,", prod, prc);
                    strcat(buffer_carrito, temp);
                }
            }
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);
        
        if(strlen(buffer_carrito) == 0) strcpy(buffer_carrito, "VACIO");
        
        strncpy(memoria->mensaje, buffer_carrito, 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- ELIMINAR DEL CARRITO ---
    if (strncmp(variable_interna, "CARRITO_DEL | ", 14) == 0) {
        char usuario_del[50], prod_del[50];
        sscanf(variable_interna, "CARRITO_DEL | Usr: %[^ |] | Prod: %s", usuario_del, prod_del);
        usuario_del[strcspn(usuario_del, " ")] = '\0';
        
        char busqueda[200]; // Aumentado a 200 para evitar warning
        snprintf(busqueda, 200, "Usr: %s | Prod: %s", usuario_del, prod_del);

        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "r");
        FILE *temp = fopen("temp.txt", "w");
        int borrado = 0; 
        if (archivo != NULL && temp != NULL) {
            char linea_arch[200];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                if (!borrado && strstr(linea_arch, busqueda) != NULL) {
                    borrado = 1; // Solo borramos la primera coincidencia
                } else {
                    fprintf(temp, "%s", linea_arch);
                }
            }
            fclose(archivo); fclose(temp);
            remove("carritos.txt"); rename("temp.txt", "carritos.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        
        strncpy(memoria->mensaje, "BORRADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- PAGAR Y MOVER A RESULTADOS PENDIENTES ---
    if (strncmp(variable_interna, "CARRITO_PAY | ", 14) == 0) {
        char usuario_pay[50]; sscanf(variable_interna, "CARRITO_PAY | Usr: %s", usuario_pay);
        printf("\n[Hilo OS: %d] El usuario '%s' HA PAGADO SU CARRITO.\n", id_hilo_real, usuario_pay);
        
        char busqueda[200]; // Aumentado a 200 para evitar warning
        snprintf(busqueda, 200, "Usr: %s | Prod: ", usuario_pay);

        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "r");
        FILE *temp = fopen("temp.txt", "w");
        FILE *resultados = fopen("resultados.txt", "a");
        
        if (archivo != NULL && temp != NULL && resultados != NULL) {
            char linea_arch[200];
            while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                if (strstr(linea_arch, busqueda) != NULL) {
                    // Si es del usuario, lo movemos a la nueva base de datos "resultados.txt"
                    char prod[50];
                    char *ptr_prod = strstr(linea_arch, "Prod: ") + 6;
                    sscanf(ptr_prod, "%[^ |]", prod);
                    fprintf(resultados, "RESULTADO | Usr: %s | Prod: %s | Estado: PENDIENTE\n", usuario_pay, prod);
                } else {
                    // Si no, lo dejamos en el carrito
                    fprintf(temp, "%s", linea_arch);
                }
            }
            fclose(archivo); fclose(temp); fclose(resultados);
            remove("carritos.txt"); rename("temp.txt", "carritos.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        
        strncpy(memoria->mensaje, "PAGO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- ATENDER REGISTRO NUEVO ---
    // Si no fue ninguno de los comandos de arriba, asumimos que es el formulario de registro
    printf("\n[Hilo OS: %d] Atendiendo registro nuevo...\n", id_hilo_real);
    pthread_mutex_lock(&mutex_archivo);
    FILE *archivo2 = fopen("usuarios.txt", "a");
    if (archivo2 != NULL) { fprintf(archivo2, "%s\n", variable_interna); fclose(archivo2); }
    pthread_mutex_unlock(&mutex_archivo);
    
    up(semid, HILO_LECTOR); up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
