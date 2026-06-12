#include "inter.h"
#include <time.h>
#include <signal.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <unistd.h>

// --- ARQUITECTURA DE SESIONES PERSISTENTES ---
typedef struct {
    char usr[50];
    char rol[20];         // "ADMIN" o "USUARIO"
    sem_t sem_req;        // Semáforo para despertar este hilo
    char peticion[512];   // Buffer interno
    int activo;
} SesionUsr;

SesionUsr sesiones[50];
int total_sesiones = 0;
pthread_mutex_t mutex_sesiones = PTHREAD_MUTEX_INITIALIZER;

// Comunicación para el Hilo exclusivo de Login/Registro
sem_t sem_auth;
char peticion_auth[512];

void *hilo_trabajador_usuario(void *arg);

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

void *hilo_autenticacion(void *arg) {
    while(1) {
        sem_wait(&sem_auth); 
        
        char variable_interna[512];
        strncpy(variable_interna, peticion_auth, sizeof(variable_interna));

        if (strncmp(variable_interna, "LOGIN | ", 8) == 0) {
            char usr_log[50] = {0}, pass_log[50] = {0};
            sscanf(variable_interna, "LOGIN | Usr: %[^ |] | PassXOR: %s", usr_log, pass_log);
            
            char admin_pass_xor[50];
            char admin_raw[] = "Admin12@";
            int i;
            for(i = 0; admin_raw[i] != '\0'; i++) admin_pass_xor[i] = admin_raw[i] ^ 0x0B;
            admin_pass_xor[i] = '\0';

            int login_exito = 0;
            int es_admin = 0;

            if (strcmp(usr_log, "admin") == 0 && strcmp(pass_log, admin_pass_xor) == 0) {
                login_exito = 1; es_admin = 1;
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
                
                if (encontrado) {
                    login_exito = 1;
                    strncpy(memoria->mensaje, "LOGIN_EXITO", 512);
                } else {
                    strncpy(memoria->mensaje, "LOGIN_ERROR", 512);
                }
            }

            // Crear hilo dedicado si el login es correcto
            if (login_exito) {
                pthread_mutex_lock(&mutex_sesiones);
                int idx = -1;
                for(int k = 0; k < total_sesiones; k++) {
                    if (sesiones[k].activo && strcmp(sesiones[k].usr, usr_log) == 0) {
                        idx = k; break;
                    }
                }
                
                if (idx == -1 && total_sesiones < 50) {
                    idx = total_sesiones++;
                    strcpy(sesiones[idx].usr, usr_log);
                    strcpy(sesiones[idx].rol, es_admin ? "ADMIN" : "USUARIO");
                    sem_init(&sesiones[idx].sem_req, 0, 0);
                    sesiones[idx].activo = 1;

                    pthread_t th_usr;
                    int *p_idx = malloc(sizeof(int));
                    *p_idx = idx;
                    pthread_create(&th_usr, NULL, hilo_trabajador_usuario, p_idx);
                    pthread_detach(th_usr);
                }
                pthread_mutex_unlock(&mutex_sesiones);
            }
            up(semid_global, SERVIDOR_LISTO);
        }
        else if (strncmp(variable_interna, "Apellido Paterno", 16) == 0) {
            pthread_mutex_lock(&mutex_archivo);
            FILE *archivo2 = fopen("usuarios.txt", "a");
            if (archivo2 != NULL) { fprintf(archivo2, "%s\n", variable_interna); fclose(archivo2); }
            pthread_mutex_unlock(&mutex_archivo);
            up(semid_global, HILO_LECTOR); 
            up(semid_global, SERVIDOR_LISTO);
        }
    }
    return NULL;
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

void *hilo_trabajador_usuario(void *arg) {
    int idx = *(int*)arg;
    free(arg); // Limpiamos el puntero para evitar fugas
    SesionUsr *mi_sesion = &sesiones[idx];

    // Obtenemos el ID real del hilo en el SO (el numerote)
    pid_t tid = syscall(SYS_gettid);

    printf("\n[Hilo OS: %d] Acceso %s PERMITIDO.\n", tid, mi_sesion->rol);

    while(1) {
        sem_wait(&mi_sesion->sem_req); // Espera petición del main
        if (!mi_sesion->activo) break;

        char variable_interna[512];
        strncpy(variable_interna, mi_sesion->peticion, sizeof(variable_interna));

        // --- ACCIONES DEL ADMINISTRADOR ---
        if (strncmp(variable_interna, "ADMIN_VENTAS_REQ", 16) == 0) {
            float v_dia = 0, v_sem = 0, v_mes = 0, v_total = 0;
            time_t now = time(NULL);
            struct tm *tm_now = localtime(&now);
            int c_d = tm_now->tm_mday, c_m = tm_now->tm_mon + 1, c_y = tm_now->tm_year + 1900;

            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("resultados.txt", "r");
            if (arch) {
                char lin[512];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, "RESULTADO |")) {
                        float precio = 0.0;
                        char *p_prc = strstr(lin, "Prc: ");
                        if (p_prc) sscanf(p_prc + 5, "%f", &precio);
                        else precio = 350.0; // Respaldo
                        
                        v_total += precio;
                        
                        char *pF = strstr(lin, "Fecha: ");
                        if (pF) {
                            int d, m, y;
                            if (sscanf(pF + 7, "%d/%d/%d", &d, &m, &y) == 3) {
                                if (y == c_y && m == c_m) {
                                    v_mes += precio;
                                    if (d == c_d) v_dia += precio;
                                }
                                struct tm tm_lin;
                                memset(&tm_lin, 0, sizeof(struct tm));
                                tm_lin.tm_mday = d; tm_lin.tm_mon = m - 1; tm_lin.tm_year = y - 1900;
                                time_t t_lin = mktime(&tm_lin);
                                double diff = difftime(now, t_lin);
                                if (diff >= 0 && diff <= 7 * 24 * 3600) {
                                    v_sem += precio;
                                }
                            }
                        }
                    }
                }
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            
            snprintf(memoria->mensaje, 512, "DIA:%.2f|SEM:%.2f|MES:%.2f|TOT:%.2f", v_dia, v_sem, v_mes, v_total);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: ELIMINAR USUARIO ---
        if (strncmp(variable_interna, "ADMIN_USR_DEL|", 14) == 0) {
            char usr_target[50];
            sscanf(variable_interna, "ADMIN_USR_DEL|%[^\n]", usr_target);
            char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_target);
            
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("usuarios.txt", "r");
            FILE *tmp = fopen("temp_usr.txt", "w");
            if (arch && tmp) {
                char lin[512];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, busq) == NULL) fprintf(tmp, "%s", lin);
                }
                fclose(arch); fclose(tmp);
                remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            strncpy(memoria->mensaje, "USUARIO_ELIMINADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: ACTUALIZAR USUARIO ---
        if (strncmp(variable_interna, "ADMIN_USR_UPD|", 14) == 0) {
            char old_usr[50], pat[50], mat[50], nom[50], san[10], cor[50], new_usr[50], pass[50];
            sscanf(variable_interna, "ADMIN_USR_UPD|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", 
                   old_usr, pat, mat, nom, san, cor, new_usr, pass);
                   
            char busq[100]; snprintf(busq, 100, "| Usr: %s |", old_usr);
            
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("usuarios.txt", "r"); 
            FILE *tmp = fopen("temp_usr.txt", "w");
            if (arch && tmp) {
                char lin[512];
                while (fgets(lin, sizeof(lin), arch) != NULL) {
                    if (strstr(lin, busq) != NULL) {
                        fprintf(tmp, "Paterno: %s | Materno: %s | Nombre: %s | Sangre: %s | Correo: %s | Usr: %s | PassXOR: %s\n", 
                                pat, mat, nom, san, cor, new_usr, pass);
                    } else {
                        fprintf(tmp, "%s", lin);
                    }
                }
                fclose(arch); fclose(tmp); 
                remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            strncpy(memoria->mensaje, "USUARIO_ACTUALIZADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: ELIMINAR ANALISIS ---
        if (strncmp(variable_interna, "ADMIN_CAT_DEL|", 14) == 0) {
            char prod[50];
            sscanf(variable_interna, "ADMIN_CAT_DEL|%[^\n]", prod);
            char busq[100]; snprintf(busq, 100, "|%s|", prod);
            
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("analisis.txt", "r");
            FILE *tmp = fopen("temp_cat.txt", "w");
            if (arch && tmp) {
                char lin[200];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, busq) == NULL) {
                        fprintf(tmp, "%s", lin);
                    } else {
                        printf("\n[Monitor Admin - Hilo OS: %d] Analisis ELIMINADO de la base de datos:\n  -> %s", tid, lin);
                    }
                }
                fclose(arch); fclose(tmp);
                remove("analisis.txt"); rename("temp_cat.txt", "analisis.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: ACTUALIZAR ANALISIS ---
        if (strncmp(variable_interna, "ADMIN_CAT_UPD|", 14) == 0) {
            char old_p[50], cat[50], sub[50], new_p[50], new_prc[20], comp[150];
            sscanf(variable_interna, "ADMIN_CAT_UPD|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", 
                   old_p, cat, sub, new_p, new_prc, comp);
                   
            char busq[100]; snprintf(busq, 100, "|%s|", old_p);
            
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("analisis.txt", "r"); 
            FILE *tmp = fopen("temp_cat.txt", "w");
            if (arch && tmp) {
                char lin[200];
                while (fgets(lin, sizeof(lin), arch) != NULL) {
                    if (strstr(lin, busq) != NULL) {
                        fprintf(tmp, "%s|%s|%s|%s|%s\n", cat, sub, new_p, new_prc, comp);
                        printf("\n[Monitor Admin - Hilo OS: %d] Analisis ACTUALIZADO:\n  -> Nuevo Registro: %s|%s|%s|$%s|%s\n", tid, cat, sub, new_p, new_prc, comp);
                    } else {
                        fprintf(tmp, "%s", lin);
                    }
                }
                fclose(arch); fclose(tmp); 
                remove("analisis.txt"); rename("temp_cat.txt", "analisis.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: AGREGAR NUEVO ANALISIS ---
        if (strncmp(variable_interna, "ADMIN_CAT_ADD|", 14) == 0) {
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("analisis.txt", "a"); 
            if (arch) {
                fprintf(arch, "%s\n", variable_interna + 14);
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            
            char c_cat[50], c_sub[50], c_nom[50], c_prc[20], c_comp[150];
            sscanf(variable_interna + 14, "%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", c_cat, c_sub, c_nom, c_prc, c_comp);
            printf("\n[Catalogo Monitor - Hilo OS: %d] NUEVO ANALISIS CREADO:\n  -> Categoria: %s [%s]\n  -> Analisis: %s | Costo: $%s\n", tid, c_cat, c_sub, c_nom, c_prc);
            
            strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: SOLICITAR RESULTADOS PENDIENTES ---
        if (strncmp(variable_interna, "ADMIN_PENDIENTES_REQ", 20) == 0) {
            char buffer_pendientes[512] = "";
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("resultados.txt", "r");
            if (arch) {
                char lin[256];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, "Estado: PENDIENTE")) {
                        char usr[50] = {0}, prod[50] = {0};
                        char *p_usr = strstr(lin, "Usr: ");
                        char *p_prod = strstr(lin, "Prod: ");
                        if (p_usr && p_prod) {
                            sscanf(p_usr + 5, "%[^ |]", usr);
                            sscanf(p_prod + 6, "%[^|]", prod);
                            int l = strlen(prod);
                            while(l > 0 && prod[l-1] == ' ') { prod[l-1] = '\0'; l--; }
                            
                            char temp[150];
                            snprintf(temp, sizeof(temp), "%s,%s;", usr, prod);
                            
                            if (strlen(buffer_pendientes) + strlen(temp) < 490) {
                                strcat(buffer_pendientes, temp);
                            }
                        }
                    }
                }
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            
            if (strlen(buffer_pendientes) == 0) strcpy(buffer_pendientes, "VACIO");
            strncpy(memoria->mensaje, buffer_pendientes, 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ADMIN: GUARDAR RESULTADO DEL PACIENTE ---
        if (strncmp(variable_interna, "ADMIN_RES_SAVE|", 15) == 0) {
            char u[50], p[50], data[200];
            sscanf(variable_interna, "ADMIN_RES_SAVE|%[^|]|%[^|]|%[^\n]", u, p, data);
            
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char fecha_str[50];
            snprintf(fecha_str, sizeof(fecha_str), "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

            pthread_mutex_lock(&mutex_archivo);
            FILE *historial = fopen("historial_medico.txt", "a");
            if (historial) {
                fprintf(historial, "PACIENTE: %s | PRUEBA: %s | Fecha: %s | RESULTADOS: %s\n", u, p, fecha_str, data);
                fclose(historial);
            }

            char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: %s | Estado: PENDIENTE", u, p);
            FILE *arch = fopen("resultados.txt", "r");
            FILE *tmp = fopen("temp_res.txt", "w");
            if (arch && tmp) {
                char lin[512];
                int modificado = 0;
                while(fgets(lin, sizeof(lin), arch)) {
                    if (!modificado && strstr(lin, busq)) {
                        char prc[20] = "350";
                        char *ptr_prc = strstr(lin, "Prc: ");
                        if(ptr_prc) sscanf(ptr_prc + 5, "%[^ |]", prc);
                        
                        fprintf(tmp, "RESULTADO | Usr: %s | Prod: %s | Estado: COMPLETADO | Prc: %s | Fecha: %s | Data: %s\n", u, p, prc, fecha_str, data);
                        modificado = 1;
                    } else {
                        fprintf(tmp, "%s", lin);
                    }
                }
                fclose(arch); fclose(tmp);
                remove("resultados.txt"); rename("temp_res.txt", "resultados.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            
            strncpy(memoria->mensaje, "RESULTADO_GUARDADO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- CLIENTE: SOLICITAR CADENAS DE RESULTADOS ---
        if (strncmp(variable_interna, "USER_RESULTS_DATA_REQ | Usr: ", 29) == 0) {
            char u[50] = {0}; 
            sscanf(variable_interna, "USER_RESULTS_DATA_REQ | Usr: %s", u);
            char buffer_res[512] = "";
            char busq[100]; snprintf(busq, 100, "Usr: %s |", u);
            
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("resultados.txt", "r");
            if (arch) {
                char lin[256];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, "Estado: COMPLETADO") && strstr(lin, busq)) {
                        char prod[50] = {0}, fecha[20] = {0}, data[256] = {0};
                        char *p_prod = strstr(lin, "Prod: ");
                        char *p_fecha = strstr(lin, "Fecha: ");
                        char *p_data = strstr(lin, "Data: ");
                        
                        if (p_prod && p_fecha && p_data) {
                            sscanf(p_prod + 6, "%[^|]", prod);
                            sscanf(p_fecha + 7, "%[^|]", fecha);
                            sscanf(p_data + 6, "%[^\n]", data);
                            
                            int l = strlen(prod); while(l > 0 && prod[l-1] == ' ') { prod[l-1] = '\0'; l--; }
                            l = strlen(fecha); while(l > 0 && fecha[l-1] == ' ') { fecha[l-1] = '\0'; l--; }
                            
                            char temp[350];
                            snprintf(temp, sizeof(temp), "%s|%s|%s;", prod, fecha, data);
                            
                            if (strlen(buffer_res) + strlen(temp) < 500) {
                                strcat(buffer_res, temp);
                            }
                        }
                    }
                }
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            
            if (strlen(buffer_res) == 0) strcpy(buffer_res, "VACIO");
            strncpy(memoria->mensaje, buffer_res, 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- CARRITO: AGREGAR ---
        if (strncmp(variable_interna, "CARRITO_ADD", 11) == 0) {
            pthread_mutex_lock(&mutex_archivo);
            FILE *archivo = fopen("carritos.txt", "a");
            if (archivo != NULL) { fprintf(archivo, "%s\n", variable_interna); fclose(archivo); }
            pthread_mutex_unlock(&mutex_archivo);
            
            char usr_add[50], prod_add[50], prc_add[20];
            sscanf(variable_interna, "CARRITO_ADD | Usr: %[^ |] | Prod: %[^ |] | Prc: %s", usr_add, prod_add, prc_add);
            printf("\n[Carrito Monitor - Hilo OS: %d] ITEM AGREGADO:\n  -> Usr: %s | Analisis: %s | Costo: $%s\n", tid, usr_add, prod_add, prc_add);

            strncpy(memoria->mensaje, "AGREGADO_EXITO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- CARRITO: PEDIR ---
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
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- CARRITO: BORRAR ---
        if (strncmp(variable_interna, "CARRITO_DEL", 11) == 0) {
            char u[50], p[50]; sscanf(variable_interna, "CARRITO_DEL | Usr: %[^ |] | Prod: %[^\n]", u, p);
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
            
            printf("\n[Carrito Monitor - Hilo OS: %d] ITEM ELIMINADO:\n  -> Usr: %s quito '%s' de su carrito.\n", tid, u, p);

            strncpy(memoria->mensaje, "BORRADO_EXITO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- CARRITO: PAGAR ---
        if (strncmp(variable_interna, "CARRITO_PAY", 11) == 0) {
            char u[50]; sscanf(variable_interna, "CARRITO_PAY | Usr: %s", u);
            char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: ", u);
            
            int cantidad_pagada = 0; 
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char f_str[50]; snprintf(f_str, sizeof(f_str), "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("carritos.txt", "r"); FILE *tmp = fopen("temp.txt", "w"); FILE *res = fopen("resultados.txt", "a");
            if (arch && tmp && res) {
                char lin[200];
                while (fgets(lin, sizeof(lin), arch) != NULL) {
                    if (strstr(lin, busq)) {
                        char prod[50]; sscanf(strstr(lin, "Prod: ")+6, "%[^ |]", prod);
                        
                        char prc[20] = "350";
                        char *ptr_prc = strstr(lin, "Prc: ");
                        if(ptr_prc) sscanf(ptr_prc + 5, "%s", prc);

                        fprintf(res, "RESULTADO | Usr: %s | Prod: %s | Estado: PENDIENTE | Prc: %s | Fecha: %s\n", u, prod, prc, f_str);
                        cantidad_pagada++;
                    } else fprintf(tmp, "%s", lin);
                }
                fclose(arch); fclose(tmp); fclose(res); remove("carritos.txt"); rename("temp.txt", "carritos.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);

            printf("\n[Transaccion Monitor - Hilo OS: %d] PAGO EXITOSO:\n  -> '%s' liquido %d analisis.\n", tid, u, cantidad_pagada);

            strncpy(memoria->mensaje, "PAGO_EXITO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- RESULTADOS REQ ---
        if (strncmp(variable_interna, "RESULTADOS_REQ", 14) == 0) {
            char u[50]; sscanf(variable_interna, "RESULTADOS_REQ | Usr: %s", u);
            char busq[100]; snprintf(busq, 100, "RESULTADO | Usr: %s |", u);
            int encontrados = 0;
            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("resultados.txt", "r");
            if (arch) {
                char lin[200];
                while(fgets(lin, sizeof(lin), arch)) { if(strstr(lin, busq)) encontrados = 1; }
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            if(encontrados) strncpy(memoria->mensaje, "HAY_DATOS", 512); else strncpy(memoria->mensaje, "VACIO", 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- PEDIR DATOS DEL PERFIL ---
        if (strncmp(variable_interna, "PERFIL_REQ", 10) == 0) {
            char usr_req[50]; sscanf(variable_interna, "PERFIL_REQ | Usr: %s", usr_req);
            char busq[100]; snprintf(busq, 100, "| Usr: %s |", usr_req);
            char respuesta[512] = "VACIO";

            pthread_mutex_lock(&mutex_archivo);
            FILE *arch = fopen("usuarios.txt", "r");
            if (arch) {
                char lin[512];
                while (fgets(lin, sizeof(lin), arch)) {
                    if (strstr(lin, busq)) {
                        lin[strcspn(lin, "\n")] = '\0'; 
                        strcpy(respuesta, lin);
                        break;
                    }
                }
                fclose(arch);
            }
            pthread_mutex_unlock(&mutex_archivo);
            strncpy(memoria->mensaje, respuesta, 512);
            up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // --- ACTUALIZAR PERFIL ---
        if (strncmp(variable_interna, "PERFIL_UPDATE", 13) == 0) {
            char usr_upd[50], pat[50], mat[50], nom[50], sangre[10], correo[50];
            sscanf(variable_interna, "PERFIL_UPDATE | Usr: %[^ |] | Paterno: %[^|]| Materno: %[^|]| Nombre: %[^|]| Sangre: %[^|]| Correo: %s", 
                   usr_upd, pat, mat, nom, sangre, correo);
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
                    } else {
                        fprintf(tmp, "%s", lin);
                    }
                }
                fclose(arch); fclose(tmp); remove("usuarios.txt"); rename("temp_usr.txt", "usuarios.txt");
            }
            pthread_mutex_unlock(&mutex_archivo);
            up(semid_global, HILO_LECTOR); up(semid_global, SERVIDOR_LISTO); sleep(1); continue;
        }

        // Por defecto, si entra algo que no es ningún IF, libera el semáforo para no trabar el servidor
        up(semid_global, SERVIDOR_LISTO);
    }
    return NULL;
}

int main() {
    pthread_t hilo_lector_id, th_auth;
    key_t llave = ftok("servidor.c", 'K');
    FILE *f = fopen("usuarios.txt", "a"); if(f) fclose(f);
    
    shmid_global = shmget(llave, sizeof(MemoriaCompartida), IPC_CREAT | PERMISOS);
    memoria = (MemoriaCompartida *)shmat(shmid_global, 0, 0);
    semid_global = semget(llave, 4, IPC_CREAT | PERMISOS);
    
    union semun arg; unsigned short vals[4] = {1, 0, 0, 0}; arg.array = vals;
    semctl(semid_global, 0, SETALL, arg);
    signal(SIGINT, manejar_cierre);

    // Arrancamos el Hilo 1 (El de Auth)
    sem_init(&sem_auth, 0, 0);
    pthread_create(&th_auth, NULL, hilo_autenticacion, NULL);
    pthread_detach(th_auth);

    // Arrancamos el monitor Lector
    pthread_create(&hilo_lector_id, NULL, leer_archivo, (void*)&semid_global);
    pthread_detach(hilo_lector_id);
    
    printf("=== SERVIDOR VITAL-TL INICIADO ===\nEsperando clientes...\n");

    while(1) {
        if(down(semid_global, CLIENTE_LISTO) == -1) break; 
        
        char req[512];
        strncpy(req, memoria->mensaje, sizeof(req));

        // 1. Desconectar o conectar al vuelo
        if (strcmp(req, "NUEVA_CONEXION") == 0 || strcmp(req, "SALIR") == 0) {
            up(semid_global, SERVIDOR_LISTO);
            continue;
        }

        // 2. Logins y Registros van al Hilo Auth
        if (strncmp(req, "LOGIN", 5) == 0 || strncmp(req, "Apellido Paterno", 16) == 0) {
            pthread_mutex_lock(&mutex_archivo);
            strncpy(peticion_auth, req, 512);
            pthread_mutex_unlock(&mutex_archivo);
            sem_post(&sem_auth); // Despertamos al Hilo Auth
            continue;
        }

        // 3. Revisar a qué Hilo de usuario le toca el trabajo
        int idx_destino = -1;
        char usr_busqueda[50] = {0};
        char *ptr = strstr(req, "Usr: ");
        
        if (ptr) {
            sscanf(ptr + 5, "%[^ |]", usr_busqueda);
            pthread_mutex_lock(&mutex_sesiones);
            for(int i = 0; i < total_sesiones; i++) {
                if(sesiones[i].activo && strcmp(sesiones[i].usr, usr_busqueda) == 0) {
                    idx_destino = i; break;
                }
            }
            pthread_mutex_unlock(&mutex_sesiones);
        } else if (strncmp(req, "ADMIN_", 6) == 0) {
            // Peticiones del admin sin tag explícito
            pthread_mutex_lock(&mutex_sesiones);
            for(int i = 0; i < total_sesiones; i++) {
                if(sesiones[i].activo && strcmp(sesiones[i].rol, "ADMIN") == 0) {
                    idx_destino = i; break;
                }
            }
            pthread_mutex_unlock(&mutex_sesiones);
        }

        // 4. Si encontramos el hilo, le inyectamos la petición
        if (idx_destino != -1) {
            pthread_mutex_lock(&mutex_sesiones);
            strncpy(sesiones[idx_destino].peticion, req, 512);
            pthread_mutex_unlock(&mutex_sesiones);
            sem_post(&sesiones[idx_destino].sem_req); 
        } else {
            // 5. Peticiones Públicas o sin sesión (Historial estático y Catálogo)
            if (strcmp(req, "CATALOGO_REQ") == 0) {
                char buffer_catalogo[512] = "";
                pthread_mutex_lock(&mutex_archivo);
                FILE *archivo = fopen("analisis.txt", "r");
                if (archivo != NULL) {
                    char linea_arch[150]; 
                    while (fgets(linea_arch, sizeof(linea_arch), archivo) != NULL) {
                        linea_arch[strcspn(linea_arch, "\n")] = 0; 
                        strcat(buffer_catalogo, linea_arch); 
                        strcat(buffer_catalogo, ";"); 
                    }
                    fclose(archivo);
                }
                pthread_mutex_unlock(&mutex_archivo);
                strncpy(memoria->mensaje, buffer_catalogo, 512);
            } 
            else if (strncmp(req, "HISTORIAL_REQ", 13) == 0) {
                char tipo[20]; sscanf(strstr(req, "Tipo: ")+6, "%s", tipo);
                if (strcmp(tipo, "SANGRE") == 0) strcpy(memoria->mensaje, "Glucosa:95|Colesterol:160|Trigliceridos:140|Acido Urico:4|");
                else if (strcmp(tipo, "ORINA") == 0) strcpy(memoria->mensaje, "PH (Acidez):6|Densidad:120|Proteinas:10|Leucocitos:5|");
                else if (strcmp(tipo, "SINO") == 0) strcpy(memoria->mensaje, "Sifilis (VDRL):0|VIH 1 y 2:0|Hepatitis C:0|Antidoping:0|"); 
            }
            else {
                strncpy(memoria->mensaje, "ERROR_SIN_SESION", 512);
            }
            up(semid_global, SERVIDOR_LISTO);
        }
    }
    return 0;
}
