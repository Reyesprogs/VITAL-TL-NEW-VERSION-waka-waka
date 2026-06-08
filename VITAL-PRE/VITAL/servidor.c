#include "inter.h"
#include <signal.h>
#include <sys/syscall.h> 

MemoriaCompartida *memoria;
long pos_archivo = 0; 
pthread_mutex_t mutex_archivo = PTHREAD_MUTEX_INITIALIZER; 
int shmid_global, semid_global;

void manejar_cierre(int sig) {
    printf("\n\n[!] Servidor apagado. Limpiando memoria...\n");
    shmctl(shmid_global, IPC_RMID, NULL); 
    semctl(semid_global, 0, IPC_RMID);    
    exit(0);
}

int obtener_hilo_simulado(const char *mensaje) {
    char usr[50] = {0};
    const char *ptr = strstr(mensaje, "Usr: "); 
    if (ptr) {
        sscanf(ptr + 5, "%[^ |]", usr);
        int hash = 5381;
        for (int i = 0; usr[i] != '\0'; i++) hash = ((hash << 5) + hash) + usr[i];
        return (hash % 8999) + 1000; 
    }
    return syscall(SYS_gettid); 
}

void *leer_archivo(void *arg) {
    int semid = *(int*)arg;
    FILE *archivo; char linea[512];
    while(1) {
        if(down(semid, HILO_LECTOR) == -1) break; 
        pthread_mutex_lock(&mutex_archivo);
        archivo = fopen("usuarios.txt", "r");
        if (archivo != NULL) {
            fseek(archivo, pos_archivo, SEEK_SET); 
            while (fgets(linea, sizeof(linea), archivo) != NULL) {
                printf("\n[Monitor] Sistema Actualizado:\n  -> %s", linea);
            }
            pos_archivo = ftell(archivo); fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);
    }
    pthread_exit(NULL);
}

void *atender_cliente(void *arg) {
    int semid = *(int*)arg;
    char variable_interna[512]; 
    strncpy(variable_interna, memoria->mensaje, sizeof(variable_interna));
    int id_hilo = obtener_hilo_simulado(variable_interna);

    if (strcmp(variable_interna, "NUEVA_CONEXION") == 0) { up(semid, SERVIDOR_LISTO); pthread_exit(NULL); }
    if (strcmp(variable_interna, "SALIR") == 0) { up(semid, SERVIDOR_LISTO); pthread_exit(NULL); }

    // --- LOGIN ---
    if (strncmp(variable_interna, "LOGIN | ", 8) == 0) {
        char usr_log[50] = {0}, pass_log[50] = {0};
        sscanf(variable_interna, "LOGIN | Usr: %[^ |] | PassXOR: %s", usr_log, pass_log);
        
        char admin_pass_xor[50];
        char admin_raw[] = "Admin12@";
        int i;
        for(i = 0; admin_raw[i] != '\0'; i++) admin_pass_xor[i] = admin_raw[i] ^ 0x0B;
        admin_pass_xor[i] = '\0';

        if (strcmp(usr_log, "admin_chu") == 0 && strcmp(pass_log, admin_pass_xor) == 0) {
            strncpy(memoria->mensaje, "LOGIN_ADMIN", 512);
        } else {
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
            if (encontrado) strncpy(memoria->mensaje, "LOGIN_EXITO", 512);
            else strncpy(memoria->mensaje, "LOGIN_ERROR", 512);
        }
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- COMANDOS ADMINISTRATIVOS ---
    if (strncmp(variable_interna, "ADMIN_CAT_DEL", 13) == 0) {
        char prod_del[50]; sscanf(variable_interna, "ADMIN_CAT_DEL | Prod: %s", prod_del);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("analisis.txt", "r"); FILE *tmp = fopen("temp_cat.txt", "w");
        if (arch && tmp) {
            char lin[200];
            while (fgets(lin, sizeof(lin), arch)) { if (strstr(lin, prod_del) == NULL) fprintf(tmp, "%s", lin); }
            fclose(arch); fclose(tmp); remove("analisis.txt"); rename("temp_cat.txt", "analisis.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "CAT_BORRADO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "ADMIN_RES_ADD", 13) == 0) {
        pthread_mutex_lock(&mutex_archivo);
        FILE *res = fopen("resultados.txt", "a");
        if (res) { fprintf(res, "%s\n", variable_interna); fclose(res); }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "RES_GUARDADO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- CARRITO Y OTROS ---
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

    if (strncmp(variable_interna, "CARRITO_ADD", 11) == 0) {
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "a");
        if (archivo != NULL) { fprintf(archivo, "%s\n", variable_interna); fclose(archivo); }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "AGREGADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "CARRITO_REQ", 11) == 0) {
        char usr_req[50] = {0}; sscanf(variable_interna, "CARRITO_REQ | Usr: %s", usr_req);
        char buffer[512] = "", busq[200]; snprintf(busq, 200, "Usr: %s | Prod: ", usr_req);
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "r");
        if (archivo != NULL) {
            char linea[200];
            while (fgets(linea, sizeof(linea), archivo) != NULL) {
                if (strstr(linea, busq) != NULL) {
                    char prod[50], prc[20];
                    sscanf(strstr(linea, "Prod: ")+6, "%[^ |]", prod);
                    sscanf(strstr(linea, "Prc: ")+5, "%s", prc);
                    char tmp[100]; snprintf(tmp, 100, "%s|%s,", prod, prc);
                    strcat(buffer, tmp);
                }
            }
            fclose(archivo);
        }
        pthread_mutex_unlock(&mutex_archivo);
        if(strlen(buffer)==0) strcpy(buffer, "VACIO");
        strncpy(memoria->mensaje, buffer, 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "CARRITO_DEL", 11) == 0) {
        char u[50], p[50]; sscanf(variable_interna, "CARRITO_DEL | Usr: %[^ |] | Prod: %s", u, p);
        u[strcspn(u, " ")]='\0'; char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: %s", u, p);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("carritos.txt", "r"); FILE *tmp = fopen("temp.txt", "w");
        int borrado = 0; 
        if (arch && tmp) {
            char lin[200];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (!borrado && strstr(lin, busq)) borrado = 1; else fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); remove("carritos.txt"); rename("temp.txt", "carritos.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "BORRADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "CARRITO_PAY", 11) == 0) {
        char u[50]; sscanf(variable_interna, "CARRITO_PAY | Usr: %s", u);
        char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: ", u);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("carritos.txt", "r"); FILE *tmp = fopen("temp.txt", "w"); FILE *res = fopen("resultados.txt", "a");
        if (arch && tmp && res) {
            char lin[200];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (strstr(lin, busq)) {
                    char prod[50]; sscanf(strstr(lin, "Prod: ")+6, "%[^ |]", prod);
                    fprintf(res, "RESULTADO | Usr: %s | Prod: %s | Estado: PENDIENTE\n", u, prod);
                } else fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); fclose(res); remove("carritos.txt"); rename("temp.txt", "carritos.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "PAGO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- PERFIL Y USUARIOS ---
    if (strncmp(variable_interna, "PERFIL_REQ", 10) == 0) {
        char usr_req[50]; sscanf(variable_interna, "PERFIL_REQ | Usr: %s", usr_req);
        char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_req);
        char respuesta[512] = "VACIO";
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("usuarios.txt", "r");
        if (arch) {
            char lin[512];
            while (fgets(lin, sizeof(lin), arch)) {
                if (strstr(lin, busq)) { lin[strcspn(lin, "\n")] = '\0'; strcpy(respuesta, lin); break; }
            }
            fclose(arch);
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, respuesta, 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "PERFIL_UPDATE", 13) == 0) {
        char usr_upd[50], pat[50], mat[50], nom[50], sangre[10], correo[50];
        sscanf(variable_interna, "PERFIL_UPDATE | Usr: %[^ |] | Paterno: %[^|]| Materno: %[^|]| Nombre: %[^|]| Sangre: %[^|]| Correo: %s", 
               usr_upd, pat, mat, nom, sangre, correo);
        usr_upd[strcspn(usr_upd, " ")] = '\0';
        char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_upd);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("usuarios.txt", "r"); FILE *tmp = fopen("temp_usr.txt", "w");
        if (arch && tmp) {
            char lin[512];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (strstr(lin, busq) != NULL) {
                    char pass[50] = "RESETEADA"; 
                    char *ptr_pass = strstr(lin, "PassXOR: ");
                    if (ptr_pass != NULL) sscanf(ptr_pass + 9, "%s", pass);
                    fprintf(tmp, "Paterno: %s | Materno: %s | Nombre: %s | Sangre: %s | Correo: %s | Usr: %s | PassXOR: %s\n", 
                            pat, mat, nom, sangre, correo, usr_upd, pass);
                } else fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        up(semid, HILO_LECTOR); up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "ADMIN_USER_DEL", 14) == 0) {
        char usr_del[50]; sscanf(variable_interna, "ADMIN_USER_DEL | Usr: %s", usr_del);
        char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_del);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("usuarios.txt", "r"); FILE *tmp = fopen("temp_usr.txt", "w");
        if (arch && tmp) {
            char lin[512];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (strstr(lin, busq) == NULL) fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "BORRADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "ADMIN_USER_UPD", 14) == 0) {
        char usr_upd[50], pat[50], mat[50], nom[50], sangre[10], correo[50], pass_new_xor[50];
        sscanf(variable_interna, "ADMIN_USER_UPD | Usr: %[^ |] | Paterno: %[^|]| Materno: %[^|]| Nombre: %[^|]| Sangre: %[^|]| Correo: %[^|]| PassXOR: %s", 
               usr_upd, pat, mat, nom, sangre, correo, pass_new_xor);
        char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_upd);
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("usuarios.txt", "r"); FILE *tmp = fopen("temp_usr.txt", "w");
        if (arch && tmp) {
            char lin[512];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (strstr(lin, busq) != NULL) {
                    fprintf(tmp, "Paterno: %s | Materno: %s | Nombre: %s | Sangre: %s | Correo: %s | Usr: %s | PassXOR: %s\n", 
                            pat, mat, nom, sangre, correo, usr_upd, pass_new_xor);
                } else fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "UPD_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    printf("\n[Hilo OS: %d] Atendiendo registro nuevo...\n", id_hilo);
    pthread_mutex_lock(&mutex_archivo);
    FILE *archivo2 = fopen("usuarios.txt", "a");
    if (archivo2 != NULL) { fprintf(archivo2, "%s\n", variable_interna); fclose(archivo2); }
    pthread_mutex_unlock(&mutex_archivo);
    up(semid, HILO_LECTOR); up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
}

int main() {
    pthread_t hilo_atencion, hilo_lector_id;
    key_t llave = ftok("servidor.c", 'K');
    FILE *f = fopen("usuarios.txt", "a"); if(f) fclose(f);
    
    shmid_global = shmget(llave, sizeof(MemoriaCompartida), IPC_CREAT | PERMISOS);
    memoria = (MemoriaCompartida *)shmat(shmid_global, 0, 0);
    semid_global = semget(llave, 4, IPC_CREAT | PERMISOS);
    union semun arg; unsigned short vals[4] = {1, 0, 0, 0}; arg.array = vals;
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
