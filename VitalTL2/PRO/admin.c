#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "admin.h"
#include "datos.h"
#include "semaforo.h"
#include "chamcont.h"
#include <sys/types.h>
#include <unistd.h>


extern void encriptar(char *texto);


typedef struct {
    char nombre_completo[150];
} EstudioCat;


typedef struct {
    char nombre[50];
    char apellido[50];
    char edad[10];
    char sangre[10];
    char correo[50];
    char usuario[50];
    char password[50];
} Usuario;

void iniciar_panel_admin(Analisis *datos) {
    int opcion = 0;
    int admin_activo = 1;
    int heartbeat_anterior = -1;
    int contador_sin_cambio = 0;

    while (admin_activo) {
        
        
        bloquear(&datos->candado);
        int heartbeat_actual = datos->heartbeat_servidor;
        desbloquear(&datos->candado);
        
        if (heartbeat_actual == heartbeat_anterior) {
            contador_sin_cambio++;
            if (contador_sin_cambio > 40) { 
                clear();
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(LINES/2 - 5, COLS/2 - 30, "========== ERROR CRITICO ==========");
                mvprintw(LINES/2 - 3, COLS/2 - 20, "El servidor VITAL-TL no esta disponible");
                mvprintw(LINES/2 - 1, COLS/2 - 25, "Por favor, reinicie el servidor");
                mvprintw(LINES/2 + 1, COLS/2 - 25, "Reintentando en 2 segundos...");
                attroff(COLOR_PAIR(3) | A_BOLD);
                refresh();
                napms(2000);
                contador_sin_cambio = 0;
                continue;
            }
        } else {
            heartbeat_anterior = heartbeat_actual;
            contador_sin_cambio = 0;
        }
        
        
        clear();
        attron(COLOR_PAIR(2) | A_BOLD); 
        mvprintw(2, 5, "--- VITAL-TL : PANEL DE ADMINISTRACION CONTROL ---");
        attroff(COLOR_PAIR(2) | A_BOLD);
        
        const char *opciones[] = {
            "[ 1. GESTIONAR CATALOGO (AGREGAR / EDITAR / ELIMINAR ESTUDIOS) ]",
            "[ 2. CAPTURAR RESULTADOS PENDIENTES DE PACIENTES ]",
            "[ 3. GENERAR REPORTES DE VENTAS CLINICAS ]",
            "[ 4. MODIFICAR USUARIOS ]",
            "[ 5. CERRAR SESION DE ADMINISTRADOR ]"
        };

        for (int i = 0; i < 5; i++) {
            if (i == opcion) attron(A_REVERSE | COLOR_PAIR(2));
            mvprintw(6 + (i * 2), 8, "%s", opciones[i]);
            if (i == opcion) attroff(A_REVERSE | COLOR_PAIR(2));
        }

        refresh();

        int tecla = getch();
        if (tecla == KEY_UP && opcion > 0) opcion--;
        else if (tecla == KEY_DOWN && opcion < 4) opcion++;
        else if (tecla == '\n' || tecla == '\r') {
            
            
            
            
            if (opcion == 0) {
                int sub_activo = 1;
                while (sub_activo) {
                    EstudioCat catalogo[100];
                    int num_estudios = 0;
                    FILE *fc = fopen("catalogo.txt", "r");
                    
                    if (fc != NULL) {
                        char lin_cat[150];
                        while (fgets(lin_cat, sizeof(lin_cat), fc) && num_estudios < 100) {
                            lin_cat[strcspn(lin_cat, "\n")] = 0;
                            if (strlen(lin_cat) > 0) {
                                strcpy(catalogo[num_estudios].nombre_completo, lin_cat);
                                num_estudios++;
                            }
                        }
                        fclose(fc);
                    }

                    int sel_cat = 0;
                    int interactuando_cat = 1;

                    while (interactuando_cat) {
                        clear();
                        attron(COLOR_PAIR(2) | A_BOLD);
                        mvprintw(2, 5, "--- SELECCIONA UN ESTUDIO PARA GESTIONAR ---");
                        attroff(COLOR_PAIR(2) | A_BOLD);

                        for (int i = 0; i < num_estudios; i++) {
                            if (i == sel_cat) attron(A_REVERSE | COLOR_PAIR(2));
                            mvprintw(5 + i, 5, "[%d] %s", i + 1, catalogo[i].nombre_completo);
                            if (i == sel_cat) attroff(A_REVERSE | COLOR_PAIR(2));
                        }

                        int opt_agregar = num_estudios;
                        int opt_reset = num_estudios + 1;
                        int opt_regresar = num_estudios + 2;

                        if (sel_cat == opt_agregar) attron(A_REVERSE | COLOR_PAIR(2) | A_BOLD);
                        mvprintw(6 + num_estudios, 5, "[ + AGREGAR NUEVO ESTUDIO ]");
                        if (sel_cat == opt_agregar) attroff(A_REVERSE | COLOR_PAIR(2) | A_BOLD);

                        if (sel_cat == opt_reset) attron(A_REVERSE | COLOR_PAIR(3) | A_BOLD);
                        mvprintw(7 + num_estudios, 5, "[ ! BORRAR TODO EL CATALOGO (RESET) ]");
                        if (sel_cat == opt_reset) attroff(A_REVERSE | COLOR_PAIR(3) | A_BOLD);

                        if (sel_cat == opt_regresar) attron(A_REVERSE | COLOR_PAIR(2) | A_BOLD);
                        mvprintw(9 + num_estudios, 5, "[ REGRESAR AL MENU PRINCIPAL ]");
                        if (sel_cat == opt_regresar) attroff(A_REVERSE | COLOR_PAIR(2) | A_BOLD);

                        refresh();

                        int sk = getch();
                        if (sk == KEY_UP && sel_cat > 0) sel_cat--;
                        else if (sk == KEY_DOWN && sel_cat < opt_regresar) sel_cat++;
                        else if (sk == '\n' || sk == '\r') {
                            
                            if (sel_cat == opt_regresar) {
                                interactuando_cat = 0;
                                sub_activo = 0;
                            } 
                            else if (sel_cat == opt_reset) {
                                remove("catalogo.txt");
                                clear();
                                mvprintw(4, 5, "Catalogo reseteado por completo.");
                                refresh(); napms(1000);
                                interactuando_cat = 0; 
                            } 
                            else if (sel_cat == opt_agregar) {
                                clear();
                                mvprintw(4, 5, "Escribe el nombre y precio (Ej. Radiografia - $500): ");
                                curs_set(1); echo();
                                char nuevo_estudio[100];
                                getnstr(nuevo_estudio, 90);
                                noecho(); curs_set(0);

                                if (strlen(nuevo_estudio) > 0) {
                                    FILE *fa = fopen("catalogo.txt", "a");
                                    if (fa != NULL) {
                                        fprintf(fa, "%s\n", nuevo_estudio);
                                        fclose(fa);
                                        mvprintw(6, 5, "Estudio agregado exitosamente.");
                                    }
                                }
                                refresh(); napms(1000);
                                interactuando_cat = 0; 
                            } 
                            else {
                                int opc_ed = 0;
                                int interactuando_ed = 1;
                                while (interactuando_ed) {
                                    clear();
                                    attron(COLOR_PAIR(2) | A_BOLD);
                                    mvprintw(2, 5, "--- ACCION PARA: %s ---", catalogo[sel_cat].nombre_completo);
                                    attroff(COLOR_PAIR(2) | A_BOLD);

                                    if (opc_ed == 0) attron(A_REVERSE | COLOR_PAIR(2));
                                    mvprintw(5, 7, "[ 1. EDITAR NOMBRE / PRECIO ]");
                                    if (opc_ed == 0) attroff(A_REVERSE | COLOR_PAIR(2));

                                    if (opc_ed == 1) attron(A_REVERSE | COLOR_PAIR(3) | A_BOLD);
                                    mvprintw(7, 7, "[ 2. ELIMINAR ESTUDIO DEL CATALOGO ]");
                                    if (opc_ed == 1) attroff(A_REVERSE | COLOR_PAIR(3) | A_BOLD);

                                    if (opc_ed == 2) attron(A_REVERSE | COLOR_PAIR(2));
                                    mvprintw(9, 7, "[ 3. CANCELAR ]");
                                    if (opc_ed == 2) attroff(A_REVERSE | COLOR_PAIR(2));

                                    refresh();
                                    int tk = getch();
                                    if (tk == KEY_UP && opc_ed > 0) opc_ed--;
                                    else if (tk == KEY_DOWN && opc_ed < 2) opc_ed++;
                                    else if (tk == '\n' || tk == '\r') {
                                        if (opc_ed == 2) {
                                            interactuando_ed = 0;
                                        } 
                                        else if (opc_ed == 0) { 
                                            clear();
                                            mvprintw(2, 5, "Estudio actual: %s", catalogo[sel_cat].nombre_completo);
                                            mvprintw(4, 5, "Ingrese los nuevos datos: ");
                                            curs_set(1); echo();
                                            char modificado[150];
                                            getnstr(modificado, 140);
                                            noecho(); curs_set(0);

                                            if (strlen(modificado) > 0) {
                                                strcpy(catalogo[sel_cat].nombre_completo, modificado);
                                                FILE *fw = fopen("catalogo.txt", "w");
                                                if (fw != NULL) {
                                                    for (int f = 0; f < num_estudios; f++) {
                                                        fprintf(fw, "%s\n", catalogo[f].nombre_completo);
                                                    }
                                                    fclose(fw);
                                                }
                                                mvprintw(6, 5, "Estudio modificado con exito.");
                                                refresh(); napms(1000);
                                            }
                                            interactuando_ed = 0;
                                            interactuando_cat = 0;
                                        } 
                                        else if (opc_ed == 1) { 
                                            FILE *fw = fopen("catalogo.txt", "w");
                                            if (fw != NULL) {
                                                for (int f = 0; f < num_estudios; f++) {
                                                    if (f != sel_cat) { 
                                                        fprintf(fw, "%s\n", catalogo[f].nombre_completo);
                                                    }
                                                }
                                                fclose(fw);
                                            }
                                            clear();
                                            mvprintw(4, 5, "Estudio eliminado del catalogo.");
                                            refresh(); napms(1000);
                                            interactuando_ed = 0;
                                            interactuando_cat = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } 
            
            
            
            else if (opcion == 1) {
                int opc_inicial = 0; 
                int menu_inicial_activo = 1;

                while (menu_inicial_activo) {
                    clear();
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvprintw(2, 5, "--- CAPTURA DE RESULTADOS PENDIENTES ---");
                    attroff(COLOR_PAIR(2) | A_BOLD);

                    mvprintw(5, 5, "Seleccione una opcion para continuar:");

                    if (opc_inicial == 0) attron(A_REVERSE | COLOR_PAIR(2));
                    mvprintw(7, 8, "[ 1. INICIAR CAPTURA DE RESULTADOS PENDIENTES ]");
                    if (opc_inicial == 0) attroff(A_REVERSE | COLOR_PAIR(2));

                    if (opc_inicial == 1) attron(A_REVERSE | COLOR_PAIR(2));
                    mvprintw(9, 8, "[ 2. REGRESAR AL MENU PRINCIPAL ]");
                    if (opc_inicial == 1) attroff(A_REVERSE | COLOR_PAIR(2));

                    refresh();

                    int t_init = getch();
                    if (t_init == KEY_UP && opc_inicial > 0) opc_inicial--;
                    else if (t_init == KEY_DOWN && opc_inicial < 1) opc_inicial++;
                    else if (t_init == '\n' || t_init == '\r') {
                        if (opc_inicial == 1) {
                            menu_inicial_activo = 0;
                        } else {
                            menu_inicial_activo = 0;

                            clear();
                            attron(COLOR_PAIR(2) | A_BOLD);
                            mvprintw(2, 5, "--- PROCESANDO CAPTURA DE RESULTADOS ---");
                            attroff(COLOR_PAIR(2) | A_BOLD);

                            bloquear(&datos->candado); 
                            FILE *fp = fopen("historial.txt", "r");
                            FILE *ftemp = fopen("temp.txt", "w");

                            if (fp == NULL || ftemp == NULL) {
                                mvprintw(4, 5, "No hay historial de compras aun.");
                                if (fp) fclose(fp);
                                if (ftemp) fclose(ftemp);
                                desbloquear(&datos->candado);
                                
                                int boton_act = 1;
                                while (boton_act) {
                                    attron(A_REVERSE | COLOR_PAIR(2));
                                    mvprintw(6, 5, "[ REGRESAR ]");
                                    attroff(A_REVERSE | COLOR_PAIR(2));
                                    refresh();
                                    int tk = getch();
                                    if (tk == '\n' || tk == '\r') boton_act = 0;
                                }
                                continue;
                            }

                            char linea[256];
                            char t_usr[50] = "", t_est[50] = "";
                            int pendientes = 0;
                            int y_pos = 5;

                            while (fgets(linea, sizeof(linea), fp)) {
                                if (y_pos > LINES - 8) {
                                    clear();
                                    mvprintw(2, 5, "--- CAPTURA DE RESULTADOS (CONT...) ---");
                                    y_pos = 5;
                                }

                                if (strncmp(linea, "Usuario: ", 9) == 0) {
                                    sscanf(linea, "Usuario: %s", t_usr);
                                    fprintf(ftemp, "%s", linea);
                                } else if (strncmp(linea, "Estudio: ", 9) == 0) {
                                    sscanf(linea, "Estudio: %[^\n]", t_est);
                                    fprintf(ftemp, "%s", linea);
                                } else if (strncmp(linea, "Valor: ", 7) == 0) {
                                    int val;
                                    sscanf(linea, "Valor: %d", &val);
                                    
                                    if (val == -2) {
                                        pendientes++;
                                        mvprintw(y_pos, 5, "Paciente: %-15s | Estudio: %-20s", t_usr, t_est);
                                        mvprintw(y_pos + 1, 5, "Ingresa el resultado numerico: ");
                                        refresh();
                                        
                                        curs_set(1); echo();
                                        char buffer_res[20];
                                        getnstr(buffer_res, 10); 
                                        noecho(); curs_set(0);

                                        fprintf(ftemp, "Valor: %s\n", buffer_res);
                                        mvprintw(y_pos + 2, 5, "[ Guardado exitosamente ]");
                                        y_pos += 4; 
                                    } else {
                                        fprintf(ftemp, "%s", linea);
                                    }
                                } else {
                                    fprintf(ftemp, "%s", linea); 
                                }
                            }
                            fclose(fp);
                            fclose(ftemp);
                            
                            remove("historial.txt");
                            rename("temp.txt", "historial.txt");
                            desbloquear(&datos->candado); 

                            if (pendientes == 0) {
                                mvprintw(5, 5, "Todo al dia. No hay estudios pendientes por evaluar.");
                                y_pos = 7;
                            } else {
                                mvprintw(y_pos + 1, 5, "Todos los resultados pendientes fueron actualizados.");
                                y_pos += 3;
                            }
                            
                            int boton_act = 1;
                            while (boton_act) {
                                attron(A_REVERSE | COLOR_PAIR(2));
                                mvprintw(y_pos, 5, "[ REGRESAR ]");
                                attroff(A_REVERSE | COLOR_PAIR(2));
                                refresh();
                                int tk = getch();
                                if (tk == '\n' || tk == '\r') boton_act = 0;
                            }
                        }
                    }
                }
            } 
            
            
            
            else if (opcion == 2) {
                clear();
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(2, 5, "--- REPORTE DE VENTAS Y FINANZAS ---");
                attroff(COLOR_PAIR(2) | A_BOLD);

                bloquear(&datos->candado);
                datos->pid_cliente = getpid();
                strcpy(datos->log_mensaje, "ADMINISTRADOR genero un reporte de resultados");
                datos->tipo_peticion = 0;
                datos->peticion_activa = 1;
                desbloquear(&datos->candado);
                napms(50);

                bloquear(&datos->candado); 
                FILE *fp = fopen("historial.txt", "r");
                int y_pos = 5;
                
                if (fp == NULL) {
                    mvprintw(5, 5, "No hay ventas registradas en el sistema aun.");
                    y_pos = 7;
                } else {
                    char linea[256];
                    int total_ingresos = 0;
                    int total_estudios = 0;
                    char lista_nombres[50][100];
                    int lista_cantidades[50] = {0};
                    int distintos = 0;

                    while (fgets(linea, sizeof(linea), fp)) {
                        if (strncmp(linea, "Estudio: ", 9) == 0) {
                            total_estudios++;
                            char nombre_est[100];
                            sscanf(linea, "Estudio: %[^\n]", nombre_est);
                            
                            int encontrado = -1;
                            for (int i = 0; i < distintos; i++) {
                                if (strcmp(lista_nombres[i], nombre_est) == 0) {
                                    encontrado = i;
                                    break;
                                }
                            }
                            
                            if (encontrado != -1) {
                                lista_cantidades[encontrado]++;
                            } else if (distintos < 50) {
                                strcpy(lista_nombres[distintos], nombre_est);
                                lista_cantidades[distintos] = 1;
                                distintos++;
                            }

                            char *precio_ptr = strstr(linea, "$");
                            if (precio_ptr != NULL) {
                                total_ingresos += atoi(precio_ptr + 1);
                            }
                        }
                    }
                    fclose(fp);
                    
                    mvprintw(4, 5, "Desglose de estudios vendidos:");
                    y_pos = 6;
                    for (int i = 0; i < distintos; i++) {
                        mvprintw(y_pos++, 5, "- %s (Vendidos: %d)", lista_nombres[i], lista_cantidades[i]);
                    }
                    y_pos++;
                    mvprintw(y_pos++, 5, "--------------------------------------------------");
                    mvprintw(y_pos++, 5, "Total de estudios realizados: %d", total_estudios);
                    
                    attron(COLOR_PAIR(1) | A_BOLD); 
                    mvprintw(y_pos++, 5, "INGRESOS TOTALES: $%d", total_ingresos);
                    attroff(COLOR_PAIR(1) | A_BOLD);
                }
                desbloquear(&datos->candado); 

                int boton_act = 1;
                y_pos += 2;
                while (boton_act) {
                    attron(A_REVERSE | COLOR_PAIR(2));
                    mvprintw(y_pos, 5, "[ REGRESAR ]");
                    attroff(A_REVERSE | COLOR_PAIR(2));
                    refresh();
                    int tk = getch();
                    if (tk == '\n' || tk == '\r') boton_act = 0;
                }
            } 
            
            
            
            else if (opcion == 3) {
                clear();
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(2, 5, "--- MODIFICAR USUARIOS ---");
                attroff(COLOR_PAIR(2) | A_BOLD);

                FILE *fu = fopen("usuarios.txt", "r");
                if (fu == NULL) {
                    mvprintw(5, 5, "No hay usuarios registrados.");
                    int boton_act = 1;
                    while (boton_act) {
                        attron(A_REVERSE | COLOR_PAIR(2));
                        mvprintw(7, 5, "[ REGRESAR ]");
                        attroff(A_REVERSE | COLOR_PAIR(2));
                        refresh();
                        int tk = getch();
                        if (tk == '\n' || tk == '\r') boton_act = 0;
                    }
                    continue;
                }

                Usuario usuarios[100];
                int num_usuarios = 0;
                char linea[256];

                while (fgets(linea, sizeof(linea), fu) && num_usuarios < 100) {
                    if (strncmp(linea, "Nombre: ", 8) == 0) {
                        sscanf(linea, "Nombre: %[^\n]", usuarios[num_usuarios].nombre);
                    } else if (strncmp(linea, "Apellido: ", 10) == 0) {
                        sscanf(linea, "Apellido: %[^\n]", usuarios[num_usuarios].apellido);
                    } else if (strncmp(linea, "Edad: ", 6) == 0) {
                        sscanf(linea, "Edad: %[^\n]", usuarios[num_usuarios].edad);
                    } else if (strncmp(linea, "Sangre: ", 8) == 0) {
                        sscanf(linea, "Sangre: %[^\n]", usuarios[num_usuarios].sangre);
                    } else if (strncmp(linea, "Correo: ", 8) == 0) {
                        sscanf(linea, "Correo: %[^\n]", usuarios[num_usuarios].correo);
                    } else if (strncmp(linea, "Usuario: ", 9) == 0) {
                        sscanf(linea, "Usuario: %[^\n]", usuarios[num_usuarios].usuario);
                    } else if (strncmp(linea, "Password: ", 10) == 0) {
                        sscanf(linea, "Password: %[^\n]", usuarios[num_usuarios].password);
                    } else if (strncmp(linea, "------------------------", 24) == 0) {
                        num_usuarios++;
                    }
                }
                fclose(fu);

                if (num_usuarios == 0) {
                    mvprintw(5, 5, "No hay usuarios legibles.");
                    int boton_act = 1;
                    while (boton_act) {
                        attron(A_REVERSE | COLOR_PAIR(2));
                        mvprintw(7, 5, "[ REGRESAR ]");
                        attroff(A_REVERSE | COLOR_PAIR(2));
                        refresh();
                        int tk = getch();
                        if (tk == '\n' || tk == '\r') boton_act = 0;
                    }
                    continue;
                }

                int sel_usuario = 0;
                int seleccionando = 1;

                while (seleccionando) {
                    clear();
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvprintw(2, 5, "--- SELECCIONA UN USUARIO PARA MODIFICAR ---");
                    attroff(COLOR_PAIR(2) | A_BOLD);

                    for (int i = 0; i < num_usuarios; i++) {
                        if (i == sel_usuario) attron(A_REVERSE | COLOR_PAIR(2));
                        mvprintw(4 + i, 5, "[%d] %s %s (Usuario: %s)", i + 1, usuarios[i].nombre, usuarios[i].apellido, usuarios[i].usuario);
                        if (i == sel_usuario) attroff(A_REVERSE | COLOR_PAIR(2));
                    }

                    if (sel_usuario == num_usuarios) attron(A_REVERSE | COLOR_PAIR(2) | A_BOLD);
                    mvprintw(5 + num_usuarios, 5, "[ REGRESAR AL MENU PRINCIPAL ]");
                    if (sel_usuario == num_usuarios) attroff(A_REVERSE | COLOR_PAIR(2) | A_BOLD);
                    refresh();

                    int tecla_usr = getch();
                    if (tecla_usr == KEY_UP && sel_usuario > 0) sel_usuario--;
                    else if (tecla_usr == KEY_DOWN && sel_usuario < num_usuarios) sel_usuario++; 
                    else if (tecla_usr == '\n' || tecla_usr == '\r') {
                        
                        if (sel_usuario == num_usuarios) {
                            seleccionando = 0; 
                        } else {
                            int campo = 0;
                            char campos_temp[7][100];
                            strcpy(campos_temp[0], usuarios[sel_usuario].nombre);
                            strcpy(campos_temp[1], usuarios[sel_usuario].apellido);
                            strcpy(campos_temp[2], usuarios[sel_usuario].edad);
                            strcpy(campos_temp[3], usuarios[sel_usuario].sangre);
                            strcpy(campos_temp[4], usuarios[sel_usuario].correo);
                            strcpy(campos_temp[5], usuarios[sel_usuario].usuario);
                            strcpy(campos_temp[6], usuarios[sel_usuario].password);

                            const char *etiquetas_ed[] = {"Nombre:", "Apellido:", "Edad:", "Sangre:", "Correo:", "Usuario:", "Password:"};
                            int editando_campo = 1;

                            while (editando_campo) {
                                clear();
                                attron(COLOR_PAIR(2) | A_BOLD);
                                mvprintw(2, 5, "--- EDITAR USUARIO: %s %s ---", usuarios[sel_usuario].nombre, usuarios[sel_usuario].apellido);
                                attroff(COLOR_PAIR(2) | A_BOLD);

                                
                                for (int i = 0; i < 7; i++) {
                                    if (i == campo) attron(A_REVERSE | COLOR_PAIR(2));
                                    mvprintw(4 + i, 5, "[%d] %s %s", i + 1, etiquetas_ed[i], campos_temp[i]);
                                    if (i == campo) attroff(A_REVERSE | COLOR_PAIR(2));
                                }

                                
                                if (campo == 7) attron(A_REVERSE | COLOR_PAIR(2) | A_BOLD);
                                mvprintw(13, 5, "[ G. GUARDAR CAMBIOS Y SALIR ]");
                                if (campo == 7) attroff(A_REVERSE | COLOR_PAIR(2) | A_BOLD);

                                
                                if (campo == 8) attron(A_REVERSE | COLOR_PAIR(3) | A_BOLD);
                                mvprintw(14, 5, "[ C. CANCELAR EDICION ]");
                                if (campo == 8) attroff(A_REVERSE | COLOR_PAIR(3) | A_BOLD);

                                refresh();

                                int tecla_ed = getch();
                                if (tecla_ed == KEY_UP && campo > 0) campo--;
                                else if (tecla_ed == KEY_DOWN && campo < 8) campo++;
                                else if (tecla_ed == '\n' || tecla_ed == '\r') {
                                    
                                    
                                    if (campo >= 0 && campo <= 6) {
                                        int entrada_valida = 0;
                                        char nuevo_valor[100];

                                        while (!entrada_valida) {
                                            clear();
                                            mvprintw(2, 5, "Editar %s", etiquetas_ed[campo]);
                                            mvprintw(3, 5, "Valor actual: %s", campos_temp[campo]);
                                            mvprintw(5, 5, "Nuevo valor (No dejar vacio): ");
                                            
                                            curs_set(1); echo();
                                            getnstr(nuevo_valor, 95);
                                            noecho(); curs_set(0);

                                            if (strlen(nuevo_valor) == 0 || nuevo_valor[0] == ' ') {
                                                mvprintw(7, 5, "[ERROR] El campo no puede quedar vacio.");
                                                refresh(); napms(1200);
                                                continue;
                                            }

                                            if (campo == 2) { 
                                                int es_numero = 1;
                                                for(size_t n = 0; n < strlen(nuevo_valor); n++) {
                                                    if(nuevo_valor[n] < '0' || nuevo_valor[n] > '9') es_numero = 0;
                                                }
                                                if(!es_numero || atoi(nuevo_valor) <= 0 || atoi(nuevo_valor) > 120) {
                                                    mvprintw(7, 5, "[ERROR] Edad invalida. Ingrese un numero entre 1 y 120.");
                                                    refresh(); napms(1500);
                                                    continue;
                                                }
                                            }

                                            if (campo == 4) { 
                                                if (strchr(nuevo_valor, '@') == NULL || strchr(nuevo_valor, '.') == NULL) {
                                                    mvprintw(7, 5, "[ERROR] Formato de correo invalido (Falta '@' o '.').");
                                                    refresh(); napms(1500);
                                                    continue;
                                                }
                                            }

                                            entrada_valida = 1;
                                        }
                                        strcpy(campos_temp[campo], nuevo_valor);
                                    }
                                    
                                    else if (campo == 7) {
                                        strcpy(usuarios[sel_usuario].nombre, campos_temp[0]);
                                        strcpy(usuarios[sel_usuario].apellido, campos_temp[1]);
                                        strcpy(usuarios[sel_usuario].edad, campos_temp[2]);
                                        strcpy(usuarios[sel_usuario].sangre, campos_temp[3]);
                                        strcpy(usuarios[sel_usuario].correo, campos_temp[4]);
                                        strcpy(usuarios[sel_usuario].usuario, campos_temp[5]);
                                        
                                        char pass_encriptada[100];
                                        strcpy(pass_encriptada, campos_temp[6]);
                                        encriptar(pass_encriptada);
                                        strcpy(usuarios[sel_usuario].password, pass_encriptada);

                                        FILE *fw = fopen("usuarios.txt", "w");
                                        if (fw != NULL) {
                                            for (int i = 0; i < num_usuarios; i++) {
                                                fprintf(fw, "Nombre: %s\nApellido: %s\nEdad: %s\nSangre: %s\nCorreo: %s\nUsuario: %s\nPassword: %s\n------------------------\n",
                                                        usuarios[i].nombre, usuarios[i].apellido, usuarios[i].edad, usuarios[i].sangre, usuarios[i].correo, usuarios[i].usuario, usuarios[i].password);
                                            }
                                            fclose(fw);
                                            clear();
                                            mvprintw(2, 5, "Usuario guardado y actualizado exitosamente.");
                                        } else {
                                            clear();
                                            mvprintw(2, 5, "Error fatal al escribir en usuarios.txt.");
                                        }
                                        
                                        int boton_act = 1;
                                        while (boton_act) {
                                            attron(A_REVERSE | COLOR_PAIR(2));
                                            mvprintw(4, 5, "[ REGRESAR ]");
                                            attroff(A_REVERSE | COLOR_PAIR(2));
                                            refresh();
                                            int tk = getch();
                                            if (tk == '\n' || tk == '\r') boton_act = 0;
                                        }
                                        editando_campo = 0;
                                        seleccionando = 0;
                                    }
                                    
                                    else if (campo == 8) {
                                        editando_campo = 0;
                                        seleccionando = 0;
                                    }
                                }
                                
                                else if (tecla_ed == 'G' || tecla_ed == 'g') {
                                    campo = 7;
                                    ungetch('\n'); 
                                } else if (tecla_ed == 'C' || tecla_ed == 'c') {
                                    campo = 8;
                                    ungetch('\n');
                                }
                            }
                        }
                    }
                }
            } 
            
            else if (opcion == 4) {
                admin_activo = 0; 
            }
        }
    }
}