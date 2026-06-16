#include <ncurses.h>
#include "panel.h"
#include "compras.h"
#include "perfil.h"
#include "pantalla.h"
#include "historial.h" 
#include "datos.h"
#include "semaforo.h"
#include <sys/types.h>
#include <unistd.h>
#include "string.h"
#include <stdlib.h>

void modificar_mis_datos(Paciente *p);
void encriptar(char *cadena);

void iniciar_panel_paciente(Paciente *p, Analisis *datos) {
    int opcion = 0;
    int sesion_activa = 1;
    int heartbeat_anterior = -1;
    int contador_sin_cambio = 0;
    
    cargar_historial(p->usuario, datos);

    while (sesion_activa) {
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
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(2, 5, "--- VITAL-TL : MENU DEL PACIENTE ---");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        mvprintw(4, 5, "Paciente conectado: %s %s", p->nombre, p->apellido);

        const char *opciones[] = {
            "[ ADQUIRIR ESTUDIOS CLINICOS ]",
            "[ INTERPRETACION SEMAFORICA DE TENDENCIAS ]",
            "[ CONSULTAR EXPEDIENTE MEDICO (PERFIL) ]",
            "[ CONFIGURAR / MODIFICAR MI PERFIL ]",
            "[ FINALIZAR SESION ]"
        };

        for (int i = 0; i < 5; i++) {
            if (i == opcion) attron(A_REVERSE | COLOR_PAIR(4));
            mvprintw(7 + (i * 2), 10, "%s", opciones[i]);
            if (i == opcion) attroff(A_REVERSE | COLOR_PAIR(4));
        }

        refresh();
        int tecla = getch();

        if (tecla == KEY_UP && opcion > 0) opcion--;
        else if (tecla == KEY_DOWN && opcion < 4) opcion++;
        else if (tecla == '\n' || tecla == '\r') {
            if (opcion == 0) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid(); 
                strcpy(datos->log_mensaje, "Paciente entro a: ADQUIRIR ESTUDIOS CLINICOS"); 
                datos->tipo_peticion = 0; 
                datos->peticion_activa = 1; 
                desbloquear(&datos->candado);
                napms(50); 

                mostrar_tienda(p, datos); 
            } else if (opcion == 1) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid(); 
                strcpy(datos->log_mensaje, "Paciente abrio: INTERPRETACION SEMAFORICA DE TENDENCIAS");
                datos->tipo_peticion = 0; 
                datos->peticion_activa = 1; 
                desbloquear(&datos->candado);
                napms(50);

                clear();
                nodelay(stdscr, TRUE); 
                int t_tends;
                while((t_tends = getch()) != 'q') {
                    actualizar(datos);
                    napms(100); 
                }
                nodelay(stdscr, FALSE);
            } else if (opcion == 2) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid(); 
                strcpy(datos->log_mensaje, "Paciente abrio: CONSULTAR EXPEDIENTE MEDICO (PERFIL)");
                datos->tipo_peticion = 0; 
                datos->peticion_activa = 1; 
                desbloquear(&datos->candado);
                napms(50);
                mostrar_perfil(p);
            } else if (opcion == 3) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid(); 
                strcpy(datos->log_mensaje, "Paciente solicito: MODIFICAR SUS PROPIOS DATOS");
                datos->tipo_peticion = 0; 
                datos->peticion_activa = 1; 
                desbloquear(&datos->candado);
                napms(50);

                modificar_mis_datos(p);
            } else if (opcion == 4) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid(); 
                strcpy(datos->log_mensaje, "Paciente solicito: FINALIZAR SESION");
                datos->tipo_peticion = 0; 
                datos->peticion_activa = 1; 
                desbloquear(&datos->candado);
                napms(50);
                sesion_activa = 0;
            }
        }
    }
}

void modificar_mis_datos(Paciente *p) {
    typedef struct {
        char nombre[50];
        char apellido[50];
        char edad[10];
        char sangre[10];
        char correo[50];
        char usuario[50];
        char password[50];
    } RegistroUsuario;

    RegistroUsuario lista_usuarios[100];
    int total_usuarios = 0;
    char linea[256];
    int mi_indice = -1;

    FILE *fu = fopen("usuarios.txt", "r");
    if (fu == NULL) {
        clear();
        mvprintw(5, 5, "Error: No se pudo abrir el archivo de usuarios.");
        getch();
        return;
    }

    memset(lista_usuarios, 0, sizeof(lista_usuarios));

    char r_nom[50] = "", r_ape[50] = "", r_edad[10] = "", r_sangre[10] = "", r_corr[50] = "", r_usr[50] = "", r_pass[50] = "";

    while (fgets(linea, sizeof(linea), fu) && total_usuarios < 100) {
        if (strncmp(linea, "Nombre: ", 8) == 0) {
            sscanf(linea, "Nombre: %[^\n]", r_nom);
        } else if (strncmp(linea, "Apellido: ", 10) == 0) {
            sscanf(linea, "Apellido: %[^\n]", r_ape);
        } else if (strncmp(linea, "Edad: ", 6) == 0) {
            sscanf(linea, "Edad: %[^\n]", r_edad);
        } else if (strncmp(linea, "Sangre: ", 8) == 0) {
            sscanf(linea, "Sangre: %[^\n]", r_sangre);
        } else if (strncmp(linea, "Correo: ", 8) == 0) {
            sscanf(linea, "Correo: %[^\n]", r_corr);
        } else if (strncmp(linea, "Usuario: ", 9) == 0) {
            sscanf(linea, "Usuario: %[^\n]", r_usr);
        } else if (strncmp(linea, "Password: ", 10) == 0) {
            sscanf(linea, "Password: %[^\n]", r_pass);
        } else if (strncmp(linea, "------------------------", 24) == 0) {
            strcpy(lista_usuarios[total_usuarios].nombre, r_nom);
            strcpy(lista_usuarios[total_usuarios].apellido, r_ape);
            strcpy(lista_usuarios[total_usuarios].edad, r_edad);
            strcpy(lista_usuarios[total_usuarios].sangre, r_sangre);
            strcpy(lista_usuarios[total_usuarios].correo, r_corr);
            strcpy(lista_usuarios[total_usuarios].usuario, r_usr);
            strcpy(lista_usuarios[total_usuarios].password, r_pass);

            if (strcmp(r_usr, p->usuario) == 0) {
                mi_indice = total_usuarios;
            }

            total_usuarios++;
            
            r_nom[0] = '\0'; r_ape[0] = '\0'; r_edad[0] = '\0'; 
            r_sangre[0] = '\0'; r_corr[0] = '\0'; r_usr[0] = '\0'; r_pass[0] = '\0';
        }
    }
    fclose(fu);

    if (mi_indice == -1) {
        clear();
        mvprintw(5, 5, "Error critico: No se encontro tu expediente en usuarios.txt");
        getch();
        return;
    }

    int campo_actual = 0;
    char campos_temp[7][100];
    strcpy(campos_temp[0], lista_usuarios[mi_indice].nombre);
    strcpy(campos_temp[1], lista_usuarios[mi_indice].apellido);
    strcpy(campos_temp[2], lista_usuarios[mi_indice].edad);
    strcpy(campos_temp[3], lista_usuarios[mi_indice].sangre);
    strcpy(campos_temp[4], lista_usuarios[mi_indice].correo);
    strcpy(campos_temp[5], lista_usuarios[mi_indice].usuario);
    strcpy(campos_temp[6], lista_usuarios[mi_indice].password);

    const char *etiquetas[] = {"Nombre:", "Apellido:", "Edad:", "Sangre:", "Correo:", "Usuario:", "Password:"};
    int editando = 1;

    while (editando) {
        clear();
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(2, 5, "--- MODIFICAR MIS DATOS PERSONALES ---");
        attroff(COLOR_PAIR(4) | A_BOLD);

        
        for (int i = 0; i < 7; i++) {
            if (i == campo_actual) attron(A_REVERSE | COLOR_PAIR(4));
            if (i == 6) {
                mvprintw(5 + i, 5, "[%d] %s ********* (Oculto)", i + 1, etiquetas[i]);
            } else {
                mvprintw(5 + i, 5, "[%d] %s %s", i + 1, etiquetas[i], campos_temp[i]);
            }
            if (i == campo_actual) attroff(A_REVERSE | COLOR_PAIR(4));
        }

        
        if (campo_actual == 7) attron(A_REVERSE | COLOR_PAIR(4) | A_BOLD);
        mvprintw(14, 5, "[ GUARDAR CAMBIOS EN MI PERFIL ]");
        if (campo_actual == 7) attroff(A_REVERSE | COLOR_PAIR(4) | A_BOLD);

        
        if (campo_actual == 8) attron(A_REVERSE | COLOR_PAIR(3) | A_BOLD);
        mvprintw(15, 5, "[ REVERTIR Y CANCELAR ]");
        if (campo_actual == 8) attroff(A_REVERSE | COLOR_PAIR(3) | A_BOLD);

        mvprintw(17, 5, "Navega con flechas ARRIBA/ABAJO. ENTER para modificar o accionar.");
        refresh();

        int tecla_ed = getch();
        if (tecla_ed == KEY_UP && campo_actual > 0) campo_actual--;
        else if (tecla_ed == KEY_DOWN && campo_actual < 8) campo_actual++;
        else if (tecla_ed == '\n' || tecla_ed == '\r') {
            
            
            if (campo_actual < 7) {
                int entrada_valida = 0;
                char buffer_nuevo[100];

                while (!entrada_valida) {
                    clear();
                    mvprintw(2, 5, "Modificando campo: %s", etiquetas[campo_actual]);
                    if (campo_actual != 6) mvprintw(3, 5, "Valor actual: %s", campos_temp[campo_actual]);
                    mvprintw(5, 5, "Ingrese el nuevo valor (No dejar vacio): ");
                    
                    curs_set(1); echo();
                    memset(buffer_nuevo, 0, sizeof(buffer_nuevo));
                    getnstr(buffer_nuevo, 45);
                    noecho(); curs_set(0);

                    
                    if (strlen(buffer_nuevo) == 0 || buffer_nuevo[0] == ' ') {
                        mvprintw(7, 5, "[ERROR] El campo no puede quedar vacio.");
                        refresh(); napms(1200);
                        continue;
                    }

                    
                    if (campo_actual == 2) {
                        int es_numero = 1;
                        for (size_t n = 0; n < strlen(buffer_nuevo); n++) {
                            if (buffer_nuevo[n] < '0' || buffer_nuevo[n] > '9') es_numero = 0;
                        }
                        if (!es_numero || atoi(buffer_nuevo) <= 0 || atoi(buffer_nuevo) > 120) {
                            mvprintw(7, 5, "[ERROR] Edad invalida. Ingrese un numero entre 1 y 120.");
                            refresh(); napms(1500);
                            continue;
                        }
                    }

                    
                    if (campo_actual == 4) {
                        if (strchr(buffer_nuevo, '@') == NULL || strchr(buffer_nuevo, '.') == NULL) {
                            mvprintw(7, 5, "[ERROR] Correo invalido. Debe incluir '@' y un dominio '.'");
                            refresh(); napms(1500);
                            continue;
                        }
                    }

                    entrada_valida = 1;
                }
                
                strcpy(campos_temp[campo_actual], buffer_nuevo);
            } 
            
            else if (campo_actual == 7) {
                strcpy(lista_usuarios[mi_indice].nombre, campos_temp[0]);
                strcpy(lista_usuarios[mi_indice].apellido, campos_temp[1]);
                strcpy(lista_usuarios[mi_indice].edad, campos_temp[2]);
                strcpy(lista_usuarios[mi_indice].sangre, campos_temp[3]);
                strcpy(lista_usuarios[mi_indice].correo, campos_temp[4]);
                strcpy(lista_usuarios[mi_indice].usuario, campos_temp[5]);

                if (strcmp(lista_usuarios[mi_indice].password, campos_temp[6]) != 0) {
                    char pass_nueva[100];
                    strcpy(pass_nueva, campos_temp[6]);
                    encriptar(pass_nueva);
                    strcpy(lista_usuarios[mi_indice].password, pass_nueva);
                }

                FILE *fw = fopen("usuarios.txt", "w");
                if (fw != NULL) {
                    for (int i = 0; i < total_usuarios; i++) {
                        fprintf(fw, "Nombre: %s\nApellido: %s\nEdad: %s\nSangre: %s\nCorreo: %s\nUsuario: %s\nPassword: %s\n------------------------\n",
                                lista_usuarios[i].nombre, lista_usuarios[i].apellido, lista_usuarios[i].edad, lista_usuarios[i].sangre, lista_usuarios[i].correo, lista_usuarios[i].usuario, lista_usuarios[i].password);
                    }
                    fclose(fw);

                    strcpy(p->nombre, lista_usuarios[mi_indice].nombre);
                    strcpy(p->apellido, lista_usuarios[mi_indice].apellido);
                    strcpy(p->edad, lista_usuarios[mi_indice].edad);
                    strcpy(p->sangre, lista_usuarios[mi_indice].sangre);
                    strcpy(p->correo, lista_usuarios[mi_indice].correo);
                    strcpy(p->usuario, lista_usuarios[mi_indice].usuario);

                    clear();
                    mvprintw(4, 5, "¡Tus datos personales se han modificado exitosamente!");
                } else {
                    clear();
                    mvprintw(4, 5, "Error: No se pudieron guardar los cambios en el disco.");
                }
                
                
                int boton_act = 1;
                while (boton_act) {
                    attron(A_REVERSE | COLOR_PAIR(4));
                    mvprintw(6, 5, "[ REGRESAR ]");
                    attroff(A_REVERSE | COLOR_PAIR(4));
                    refresh();
                    int tk = getch();
                    if (tk == '\n' || tk == '\r') boton_act = 0;
                }
                editando = 0;
            } 
            
            else if (campo_actual == 8) {
                editando = 0;
            }
        }
    }
}