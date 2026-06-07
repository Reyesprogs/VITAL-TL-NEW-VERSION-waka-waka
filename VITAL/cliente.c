#include <ncurses.h>
#include <string.h>
#include "inter.h"
#include "validaciones.h"

// --- 1. CONFIGURACIÓN VISUAL ---
void configurar_colores() {
    start_color();
    if (can_change_color()) {
        init_color(10, 850, 850, 850); // Gris claro base
        init_color(11, 250, 250, 250); // Gris oscuro texto
        init_color(12, 100, 600, 900); // Azul
        init_color(13, 350, 350, 350); // Gris oscuro para botones/errores
        init_color(14, 800, 800, 800); // Gris claro para letras de botones

        init_pair(1, 11, 10); // Texto normal oscuro sobre fondo claro
        init_pair(2, 10, 12); // Resaltado azul
        init_pair(3, 12, 10); // Títulos azules
        init_pair(4, 14, 13); // Botones: Letra clara en fondo oscuro
        init_pair(5, 14, 13); // Fondos de error (Gris oscuro)
    } else {
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_CYAN);
        init_pair(3, COLOR_CYAN, COLOR_WHITE);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_RED, COLOR_WHITE);
    }
}

void dibujar_tarjeta(int start_y, int start_x, int alto, int ancho) {
    attron(COLOR_PAIR(1));
    for(int i = 0; i < alto; i++) {
        mvhline(start_y + i, start_x, ' ', ancho);
    }
    mvhline(start_y, start_x + 1, ACS_HLINE, ancho - 2);         
    mvhline(start_y + alto - 1, start_x + 1, ACS_HLINE, ancho - 2); 
    mvvline(start_y + 1, start_x, ACS_VLINE, alto - 2);         
    mvvline(start_y + 1, start_x + ancho - 1, ACS_VLINE, alto - 2); 
    mvaddch(start_y, start_x, ACS_ULCORNER);
    mvaddch(start_y, start_x + ancho - 1, ACS_URCORNER);
    mvaddch(start_y + alto - 1, start_x, ACS_LLCORNER);
    mvaddch(start_y + alto - 1, start_x + ancho - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(1));
}

// --- 2. PANTALLAS DE NOTIFICACIÓN Y ERRORES ---
void mostrar_error_servidor() {
    clear();
    int start_y = (LINES - 10) / 2, start_x = (COLS - 50) / 2;
    dibujar_tarjeta(start_y, start_x, 10, 50);
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(start_y + 3, start_x + 5, "Error: hay un ploblemita con el sevidor");
    attron(COLOR_PAIR(4));
    mvprintw(start_y + 6, start_x + 19, "   Salir   ");
    attroff(COLOR_PAIR(4) | COLOR_PAIR(5) | A_BOLD);
    refresh();
    while (getch() != '\n') {}
    endwin(); exit(1); 
}

void mostrar_notificacion(const char *mensaje, int es_exito) {
    clear();
    int alto = 10, ancho = 55;
    int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
    dibujar_tarjeta(sy, sx, alto, ancho);

    if (es_exito) attron(COLOR_PAIR(2) | A_BOLD); 
    else attron(COLOR_PAIR(5) | A_BOLD);

    int len_msg = strlen(mensaje);
    mvprintw(sy + 3, sx + (ancho - len_msg) / 2, "%s", mensaje);
    
    if (es_exito) attroff(COLOR_PAIR(2) | A_BOLD);
    else attroff(COLOR_PAIR(5) | A_BOLD);

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(sy + 6, sx + 20, " [ Continuar ] ");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    refresh();
    while (getch() != '\n' && getch() != '\r') {} 
}

void mostrar_errores_formulario(char errores[7][100], int n_errores) {
    int alto = n_errores + 6, ancho = 60;
    int start_y = (LINES - alto) / 2, start_x = (COLS - ancho) / 2;

    attron(COLOR_PAIR(5));
    for(int i=0; i<alto; i++) mvhline(start_y+i, start_x, ' ', ancho);
    
    attron(A_BOLD);
    mvprintw(start_y + 1, start_x + 15, "--- CORRIGE ESTOS DATOS ---");
    attroff(A_BOLD);

    for(int i = 0; i < n_errores; i++) {
        mvprintw(start_y + 3 + i, start_x + 2, "- %s", errores[i]);
    }

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(start_y + alto - 2, start_x + (ancho-16)/2, "  [ Regresar ]  ");
    attroff(COLOR_PAIR(1) | A_BOLD | COLOR_PAIR(5));
    refresh();

    while (getch() != '\n' && getch() != '\r') {} 
}

// --- 3. PANTALLA DE LOGIN ---
void pantalla_login(int semid, MemoriaCompartida *memoria) {
    int foco = 0; 
    char campos[2][50] = {0};
    const char *titulos[] = {"Usuario:", "Password:"};

    while(1) {
        clear();
        int alto = 16, ancho = 45;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 11, "=== INICIAR SESION ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        for(int i = 0; i < 2; i++) {
            attron(COLOR_PAIR(1));
            mvprintw(sy + 5 + (i * 2), sx + 4, "%-10s", titulos[i]);
            
            if (foco == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4));
            mvprintw(sy + 5 + (i * 2), sx + 15, "                        ");
            
            if (i == 1) { 
                int len = (int)strlen(campos[i]);
                for(int j = 0; j < len; j++) mvprintw(sy + 5 + (i * 2), sx + 16 + j, "*");
            } else {
                mvprintw(sy + 5 + (i * 2), sx + 16, "%s", campos[i]);
            }
            attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
        }

        if (foco == 2) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 10, sx + 15, " [ Ingresar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (foco == 3) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 12, sx + 16, " [ Salir ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + 4) % 4; 
        else if (ch == KEY_DOWN) foco = (foco + 1) % 4; 
        else if (ch == '\n' || ch == '\r') {
            if (foco == 3) return; 
            else if (foco == 2) {
                if (strlen(campos[0]) == 0 || strlen(campos[1]) == 0) {
                    mostrar_notificacion("Campos vacios no permitidos", 0);
                    continue;
                }

                char pass_encriptada[50];
                encriptar_ascii(campos[1], pass_encriptada);

                if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                
                snprintf(memoria->mensaje, 512, "LOGIN | Usr: %s | PassXOR: %s", campos[0], pass_encriptada);
                
                if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 

                char respuesta_servidor[512];
                strncpy(respuesta_servidor, memoria->mensaje, 512);
                
                if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

                if (strcmp(respuesta_servidor, "LOGIN_EXITO") == 0) {
                    mostrar_notificacion("Acceso CORRECTO. Bienvenido a VITAL-TL.", 1);
                    return; 
                } else {
                    mostrar_notificacion("ERROR: Usuario o contrasena incorrectos.", 0);
                }
            } else {
                foco = (foco + 1) % 4; 
            }
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (foco < 2) { 
                int len = strlen(campos[foco]);
                if (len > 0) campos[foco][len-1] = '\0';
            }
        } 
        else if (ch >= 32 && ch <= 126) {
            if (foco < 2) { 
                int len = strlen(campos[foco]);
                if (len < 22) { 
                    campos[foco][len] = ch;
                    campos[foco][len+1] = '\0';
                }
            }
        }
    }
}

// --- 4. PANTALLA DE REGISTRO ---
void pantalla_registro(int semid, MemoriaCompartida *memoria) {
    int foco = 0; 
    char campos[7][50] = {0};
    const char *titulos[] = {
        "Paterno:", "Materno:", "Nombre(s):", "Sangre (A+):", 
        "Correo:", "Usuario:", "Password:"
    };

    while (1) {
        clear();
        int alto = 22, ancho = 55;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 1, sx + 15, "=== NUEVO PACIENTE ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < 7; i++) {
            attron(COLOR_PAIR(1));
            mvprintw(sy + 3 + (i * 2), sx + 2, "%-12s", titulos[i]);
            
            if (foco == i) attron(COLOR_PAIR(2)); 
            else attron(COLOR_PAIR(4)); 
            
            mvprintw(sy + 3 + (i * 2), sx + 15, "                              "); 
            
            if (i == 6) {
                int longitud = (int)strlen(campos[i]);
                for(int j = 0; j < longitud; j++) {
                    mvprintw(sy + 3 + (i * 2), sx + 16 + j, "*");
                }
            } else {
                mvprintw(sy + 3 + (i * 2), sx + 16, "%s", campos[i]);
            }
            
            attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
        }

        if (foco == 7) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 18, sx + 20, " [ Registrar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (foco == 8) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 20, sx + 22, " [ Salir ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + 9) % 9; 
        else if (ch == KEY_DOWN) foco = (foco + 1) % 9; 
        else if (ch == '\n' || ch == '\r') {
            if (foco == 8) return; 
            else if (foco == 7) {
                char errores[7][100];
                int n_err = 0;

                if (!validar_apellido(campos[0])) strcpy(errores[n_err++], "Paterno: 1 palabra, Inicio Mayuscula");
                if (!validar_apellido(campos[1])) strcpy(errores[n_err++], "Materno: 1 palabra, Inicio Mayuscula");
                if (!validar_nombre(campos[2])) strcpy(errores[n_err++], "Nombre: Max 2 palabras, Inicio Mayuscula");
                if (!validar_sangre(campos[3])) strcpy(errores[n_err++], "Sangre: Solo tipos reales y su signo (Ej: O+)");
                if (!validar_correo(campos[4])) strcpy(errores[n_err++], "Correo: Requiere '@' y '.'");
                if (!validar_usuario(campos[5])) strcpy(errores[n_err++], "Usuario: 5-15 letras (minus), num o '_'");
                if (!validar_password(campos[6])) strcpy(errores[n_err++], "Pass: Min 8, 1 may, 1 min, 1 num, 1 sim");

                if (n_err > 0) {
                    mostrar_errores_formulario(errores, n_err);
                } else {
                    char pass_encriptada[50];
                    encriptar_ascii(campos[6], pass_encriptada);

                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    
                    snprintf(memoria->mensaje, 512, "Nombre: %s %s %s | Sangre: %s | Correo: %s | Usr: %s | PassXOR: %s", 
                             campos[2], campos[0], campos[1], campos[3], campos[4], campos[5], pass_encriptada);
                    
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

                    mostrar_notificacion("Creacion de usuario exitoso", 1);
                    return; 
                }
            } else {
                foco = (foco + 1) % 9; 
            }
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (foco < 7) { 
                int len = strlen(campos[foco]);
                if (len > 0) campos[foco][len-1] = '\0';
            }
        } 
        else if (ch >= 32 && ch <= 126) {
            if (foco < 7) { 
                int len = strlen(campos[foco]);
                if (len < 28) { 
                    campos[foco][len] = ch;
                    campos[foco][len+1] = '\0';
                }
            }
        }
    }
}

// --- 5. FUNCIÓN PRINCIPAL Y MENÚ ---
int main() {
    int shmid, semid;
    MemoriaCompartida *memoria;
    key_t llave = ftok("servidor.c", 'K');

    // Conexión segura con el servidor
    semid = semget(llave, 4, PERMISOS);
    if(semid == -1) {
        printf("\n[ERROR] El servidor VITAL-TL no esta en ejecucion.\n\n");
        exit(-1);
    }

    shmid = shmget(llave, sizeof(MemoriaCompartida), PERMISOS);
    memoria = (MemoriaCompartida *)shmat(shmid, 0, 0);

    // Handshake
    if (down(semid, MUTEX) != -1) {
        strncpy(memoria->mensaje, "NUEVA_CONEXION", 512);
        up(semid, CLIENTE_LISTO);
        down(semid, SERVIDOR_LISTO);
        up(semid, MUTEX);
    }

    // Inicializar interfaz gráfica
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    if (has_colors()) configurar_colores();
    bkgd(COLOR_PAIR(1));

    int opcion = 0, ejecutando = 1;
    const char *opciones[3] = {"   Iniciar Sesion   ", "    Registrarse     ", "       Salir        "};

    while (ejecutando) {
        clear();
        int alto_tarjeta = 14, ancho_tarjeta = 46;
        int start_y = (LINES - alto_tarjeta) / 2, start_x = (COLS - ancho_tarjeta) / 2;
        dibujar_tarjeta(start_y, start_x, alto_tarjeta, ancho_tarjeta);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(start_y + 2, start_x + 15, "=== VITAL-TL ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        attron(COLOR_PAIR(1)); mvprintw(start_y + 3, start_x + 9, "Sistema Clinico Centralizado");
        mvhline(start_y + 5, start_x + 2, ACS_HLINE, ancho_tarjeta - 4); attroff(COLOR_PAIR(1));

        for (int i = 0; i < 3; i++) {
            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD); mvprintw(start_y + 7 + (i * 2), start_x + 13, "%s", opciones[i]); attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1)); mvprintw(start_y + 7 + (i * 2), start_x + 13, "%s", opciones[i]); attroff(COLOR_PAIR(1));
            }
        }
        refresh();

        int tecla = getch();
        switch (tecla) {
            case KEY_UP: opcion = (opcion - 1 + 3) % 3; break;
            case KEY_DOWN: opcion = (opcion + 1) % 3; break;
            case '\n': case '\r':
                if (opcion == 0) {
                    pantalla_login(semid, memoria);
                } 
                else if (opcion == 1) {
                    pantalla_registro(semid, memoria);
                } 
                else if (opcion == 2) {
                    if (down(semid, MUTEX) != -1) {
                        strncpy(memoria->mensaje, "SALIR", 512);
                        up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                    }
                    ejecutando = 0; 
                }
                break;
        }
    }
    
    endwin(); 
    shmdt(memoria); 
    return 0;
}
