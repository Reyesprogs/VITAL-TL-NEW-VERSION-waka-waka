#include "inter.h"
#include <time.h>
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

        if (strcmp(usr_log, "admin") == 0 && strcmp(pass_log, admin_pass_xor) == 0) {
            printf("\n[Hilo OS: %d] Acceso ADMIN PERMITIDO.\n", id_hilo);
            strncpy(memoria->mensaje, "LOGIN_ADMIN", 512);
            up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
        }

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
            printf("\n[Hilo OS: %d] Acceso PERMITIDO.\n", id_hilo);
            strncpy(memoria->mensaje, "LOGIN_EXITO", 512);
        } else {
            printf("\n[Hilo OS: %d] Acceso DENEGADO.\n", id_hilo);
            strncpy(memoria->mensaje, "LOGIN_ERROR", 512);
        }
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- ACCIONES DEL ADMINISTRADOR ---
    if (strncmp(variable_interna, "ADMIN_VENTAS_REQ", 16) == 0) {
        int total_ventas = 0;
        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("resultados.txt", "r");
        if (arch) {
            char lin[200];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if(strstr(lin, "RESULTADO |")) total_ventas += 350; 
            }
            fclose(arch);
        }
        pthread_mutex_unlock(&mutex_archivo);
        snprintf(memoria->mensaje, 512, "Balance de Ventas Totales: $%d.00 MXN", total_ventas);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
                    printf("\n[Monitor Admin] Analisis ELIMINADO de la base de datos:\n  -> %s", lin);
                }
            }
            fclose(arch); fclose(tmp);
            remove("analisis.txt"); rename("temp_cat.txt", "analisis.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
                    printf("\n[Monitor Admin] Analisis ACTUALIZADO:\n  -> Nuevo Registro: %s|%s|%s|$%s|%s\n", cat, sub, new_p, new_prc, comp);
                } else {
                    fprintf(tmp, "%s", lin);
                }
            }
            fclose(arch); fclose(tmp); 
            remove("analisis.txt"); rename("temp_cat.txt", "analisis.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);
        strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
        printf("\n[Catalogo Monitor] NUEVO ANALISIS CREADO:\n  -> Categoria: %s [%s]\n  -> Analisis: %s | Costo: $%s\n", c_cat, c_sub, c_nom, c_prc);
        
        strncpy(memoria->mensaje, "CATALOGO_ACTUALIZADO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

// --- ADMIN: SOLICITAR RESULTADOS PENDIENTES (CORREGIDO PARA ESPACIOS) ---
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
                        // CORRECCIÓN: Leemos hasta el pipe '|' para incluir los espacios del nombre
                        sscanf(p_prod + 6, "%[^|]", prod);
                        
                        // Recortamos los espacios en blanco sobrantes al final del nombre
                        int l = strlen(prod);
                        while(l > 0 && prod[l-1] == ' ') {
                            prod[l-1] = '\0';
                            l--;
                        }
                        
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

// --- ADMIN: GUARDAR RESULTADO DEL PACIENTE (CON FECHA REAL) ---
    if (strncmp(variable_interna, "ADMIN_RES_SAVE|", 15) == 0) {
        char u[50], p[50], data[200];
        sscanf(variable_interna, "ADMIN_RES_SAVE|%[^|]|%[^|]|%[^\n]", u, p, data);
        
        // Obtener fecha del sistema (DD/MM/YYYY)
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
		char fecha_str[50]; // Aumentamos la capacidad a 50 bytes
		snprintf(fecha_str, sizeof(fecha_str), "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

        pthread_mutex_lock(&mutex_archivo);
        FILE *historial = fopen("historial_medico.txt", "a");
        if (historial) {
            fprintf(historial, "PACIENTE: %s | PRUEBA: %s | Fecha: %s | RESULTADOS: %s\n", u, p, fecha_str, data);
            fclose(historial);
            printf("\n[Historial Monitor] Nuevo resultado guardado en historial_medico.txt de '%s'\n", u);
        }

        char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: %s | Estado: PENDIENTE", u, p);
        FILE *arch = fopen("resultados.txt", "r");
        FILE *tmp = fopen("temp_res.txt", "w");
        if (arch && tmp) {
            char lin[512];
            int modificado = 0;
            while(fgets(lin, sizeof(lin), arch)) {
                if (!modificado && strstr(lin, busq)) {
                    // Guardamos la fecha y los datos estructurados en resultados.txt
                    fprintf(tmp, "RESULTADO | Usr: %s | Prod: %s | Estado: COMPLETADO | Fecha: %s | Data: %s\n", u, p, fecha_str, data);
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- CLIENTE: SOLICITAR CADENAS DE RESULTADOS COMPLETADOS ---
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
                    lin[strcspn(lin, "\n")] = 0;
                    // Los empaquetamos separados por punto y coma (;) para el cliente
                    if (strlen(buffer_res) + strlen(lin) + 2 < 512) {
                        strcat(buffer_res, lin);
                        strcat(buffer_res, ";");
                    }
                }
            }
            fclose(arch);
        }
        pthread_mutex_unlock(&mutex_archivo);
        
        if (strlen(buffer_res) == 0) strcpy(buffer_res, "VACIO");
        strncpy(memoria->mensaje, buffer_res, 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- CATALOGO ---
    if (strcmp(variable_interna, "CATALOGO_REQ") == 0) {
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // --- CARRITO ---
    if (strncmp(variable_interna, "CARRITO_ADD", 11) == 0) {
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo = fopen("carritos.txt", "a");
        if (archivo != NULL) { fprintf(archivo, "%s\n", variable_interna); fclose(archivo); }
        pthread_mutex_unlock(&mutex_archivo);
        
        char usr_add[50], prod_add[50], prc_add[20];
        sscanf(variable_interna, "CARRITO_ADD | Usr: %[^ |] | Prod: %[^ |] | Prc: %s", usr_add, prod_add, prc_add);
        printf("\n[Carrito Monitor] ITEM AGREGADO:\n  -> Usr: %s | Analisis: %s | Costo: $%s\n", usr_add, prod_add, prc_add);

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
        
        printf("\n[Carrito Monitor] ITEM ELIMINADO:\n  -> Usr: %s quito '%s' de su carrito.\n", u, p);

        strncpy(memoria->mensaje, "BORRADO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "CARRITO_PAY", 11) == 0) {
        char u[50]; sscanf(variable_interna, "CARRITO_PAY | Usr: %s", u);
        char busq[200]; snprintf(busq, 200, "Usr: %s | Prod: ", u);
        
        int cantidad_pagada = 0; 

        pthread_mutex_lock(&mutex_archivo);
        FILE *arch = fopen("carritos.txt", "r"); FILE *tmp = fopen("temp.txt", "w"); FILE *res = fopen("resultados.txt", "a");
        if (arch && tmp && res) {
            char lin[200];
            while (fgets(lin, sizeof(lin), arch) != NULL) {
                if (strstr(lin, busq)) {
                    char prod[50]; sscanf(strstr(lin, "Prod: ")+6, "%[^ |]", prod);
                    fprintf(res, "RESULTADO | Usr: %s | Prod: %s | Estado: PENDIENTE\n", u, prod);
                    cantidad_pagada++;
                } else fprintf(tmp, "%s", lin);
            }
            fclose(arch); fclose(tmp); fclose(res); remove("carritos.txt"); rename("temp.txt", "carritos.txt");
        }
        pthread_mutex_unlock(&mutex_archivo);

        printf("\n[Transaccion Monitor] PAGO EXITOSO ($):\n  -> El paciente '%s' liquido %d analisis.\n  -> (Enviados a bandeja de PENDIENTES del Admin)\n", u, cantidad_pagada);

        strncpy(memoria->mensaje, "PAGO_EXITO", 512);
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }
    
    // --- RESULTADOS_REQ / HISTORIAL_REQ ---
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    if (strncmp(variable_interna, "HISTORIAL_REQ", 13) == 0) {
        char tipo[20]; sscanf(strstr(variable_interna, "Tipo: ")+6, "%s", tipo);
        if (strcmp(tipo, "SANGRE") == 0) strcpy(memoria->mensaje, "Glucosa:95|Colesterol:160|Trigliceridos:140|Acido Urico:4|");
        else if (strcmp(tipo, "ORINA") == 0) strcpy(memoria->mensaje, "PH (Acidez):6|Densidad:120|Proteinas:10|Leucocitos:5|");
        else if (strcmp(tipo, "SINO") == 0) strcpy(memoria->mensaje, "Sifilis (VDRL):0|VIH 1 y 2:0|Hepatitis C:0|Antidoping:0|"); 
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
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
        up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

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
        up(semid, HILO_LECTOR); up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    // Nuevo registro
    if (strncmp(variable_interna, "Apellido Paterno", 16) == 0) {
        pthread_mutex_lock(&mutex_archivo);
        FILE *archivo2 = fopen("usuarios.txt", "a");
        if (archivo2 != NULL) { fprintf(archivo2, "%s\n", variable_interna); fclose(archivo2); }
        pthread_mutex_unlock(&mutex_archivo);
        up(semid, HILO_LECTOR); up(semid, SERVIDOR_LISTO); sleep(1); pthread_exit(NULL);
    }

    up(semid, SERVIDOR_LISTO); pthread_exit(NULL);
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
