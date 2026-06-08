#include "admin.h"
#include "ui.h"
#include "validaciones.h"
#include <string.h>
// --- UTILIDADES LOCALES ---
void recortar_espacios_admin(char *str) {
    int len = strlen(str);
    while (len > 0 && str[len-1] == ' ') { str[len-1] = '\0'; len--; }
}

void mostrar_errores_admin(char errores[6][100], int n_errores) {
    int alto = n_errores + 6, ancho = 60;
    int start_y = (LINES - alto) / 2, start_x = (COLS - ancho) / 2;
    attron(COLOR_PAIR(5));
    for(int i=0; i<alto; i++) mvhline(start_y+i, start_x, ' ', ancho);
    attron(A_BOLD); mvprintw(start_y + 1, start_x + 15, "--- CORRIGE ESTOS DATOS ---"); attroff(A_BOLD);
    for(int i = 0; i < n_errores; i++) mvprintw(start_y + 3 + i, start_x + 2, "- %s", errores[i]);
    attron(COLOR_PAIR(1) | A_BOLD); mvprintw(start_y + alto - 2, start_x + (ancho-16)/2, "  [ Regresar ]  "); attroff(COLOR_PAIR(1) | A_BOLD | COLOR_PAIR(5));
    refresh();
    while (getch() != '\n' && getch() != '\r') {} 
}

// --- SUBMÓDULO: GESTIÓN DE USUARIOS ---
void gestionar_usuarios(int semid, MemoriaCompartida *memoria) {
    int en_menu = 1;
    while(en_menu) {
        char busqueda_usr[50] = {0};
        int capturando = 1;
        int foco_buscar = 0; 
        
        // --- FASE 1: BUSCADOR (Rediseñado) ---
        while(capturando) {
            clear();
            int alto = 14, ancho = 55;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);
            
            attron(COLOR_PAIR(3) | A_BOLD); 
            mvprintw(sy + 2, sx + 16, "=== BUSCAR USUARIO ==="); 
            attroff(COLOR_PAIR(3) | A_BOLD);

            // Input del Usuario
            attron(COLOR_PAIR(1));
            mvprintw(sy + 5, sx + 8, "Usr a buscar:");
            if (foco_buscar == 0) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4));
            mvprintw(sy + 5, sx + 22, "                    "); 
            mvprintw(sy + 5, sx + 23, "%s", busqueda_usr);
            attroff(COLOR_PAIR(2) | COLOR_PAIR(4) | COLOR_PAIR(1));

            // Botones
            if (foco_buscar == 0) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 9, sx + 15, " [ Buscar ] ");
            attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            if (foco_buscar == 1) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 9, sx + 30, " [ Regresar ] ");
            attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            refresh();
            int ch = getch();

            if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT) {
                foco_buscar = (foco_buscar + 1) % 2;
            } 
            else if (ch == '\n' || ch == '\r') {
                if (foco_buscar == 1) { 
                    en_menu = 0; // Regresa al menú Admin
                    capturando = 0; 
                } else {
                    if (strlen(busqueda_usr) > 0) {
                        capturando = 0; // Procede a buscar
                    } else {
                        mostrar_notificacion("Ingresa un usuario para buscar.", 0);
                    }
                }
            } 
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (foco_buscar == 0) {
                    int len = strlen(busqueda_usr);
                    if (len > 0) busqueda_usr[len-1] = '\0';
                }
            } 
            else if (ch >= 32 && ch <= 126) {
                if (foco_buscar == 0) {
                    int len = strlen(busqueda_usr);
                    if (len < 18) { busqueda_usr[len] = ch; busqueda_usr[len+1] = '\0'; }
                }
            }
        }
        
        if (!en_menu) break; // Si le dio a Regresar, rompe el ciclo principal

        // --- PEDIR DATOS AL SERVIDOR ---
        if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
        snprintf(memoria->mensaje, 512, "PERFIL_REQ | Usr: %s", busqueda_usr);
        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
        char respuesta[512]; strncpy(respuesta, memoria->mensaje, 512);
        if (up(semid, MUTEX) == -1) mostrar_error_servidor();

        if (strcmp(respuesta, "VACIO") == 0) {
            mostrar_notificacion("ERROR: El usuario no existe en la base de datos.", 0);
            continue;
        }

        // --- PARSEAR RESPUESTA Y DESCIFRAR CONTRASENA ---
        char pat[50]="---", mat[50]="---", nom[50]="---", san[20]="---", cor[50]="---", pass_xor[50]="---", pass_raw[50]="";
        char *p_pat = strstr(respuesta, "Paterno: "); char *p_mat = strstr(respuesta, "Materno: ");
        char *p_nom = strstr(respuesta, "Nombre: ");  char *p_san = strstr(respuesta, "Sangre: ");
        char *p_cor = strstr(respuesta, "Correo: ");  char *p_pas = strstr(respuesta, "PassXOR: ");
        
        if (p_pat) { sscanf(p_pat + 9, "%[^|]", pat); recortar_espacios_admin(pat); }
        if (p_mat) { sscanf(p_mat + 9, "%[^|]", mat); recortar_espacios_admin(mat); }
        if (p_nom) { sscanf(p_nom + 8, "%[^|]", nom); recortar_espacios_admin(nom); }
        if (p_san) { sscanf(p_san + 8, "%[^|]", san); recortar_espacios_admin(san); }
        if (p_cor) { sscanf(p_cor + 8, "%[^|]", cor); recortar_espacios_admin(cor); }
        if (p_pas) { sscanf(p_pas + 9, "%s", pass_xor); }
        
        encriptar_ascii(pass_xor, pass_raw); 

        // --- FASE 2: VISTA DE LECTURA Y OPCIONES ---
        int accion = 0; 
        int viendo = 1;
        while(viendo) {
            clear();
            int alto = 20, ancho = 64;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);
            
            attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 20, "=== DATOS DEL USUARIO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
            
            attron(COLOR_PAIR(1));
            mvprintw(sy + 4, sx + 6, "Usuario:    %s", busqueda_usr);
            mvprintw(sy + 5, sx + 6, "Paterno:    %s", pat);
            mvprintw(sy + 6, sx + 6, "Materno:    %s", mat);
            mvprintw(sy + 7, sx + 6, "Nombre:     %s", nom);
            mvprintw(sy + 8, sx + 6, "Sangre:     %s", san);
            mvprintw(sy + 9, sx + 6, "Correo:     %s", cor);
            mvprintw(sy + 10, sx + 6,"Contrasena: %s", pass_raw); 
            attroff(COLOR_PAIR(1));

            if (accion == 0) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 14, sx + 6, " [ Editar ] "); attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1));
            
            if (accion == 1) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
            mvprintw(sy + 14, sx + 25, " [ Borrar ] "); attroff(COLOR_PAIR(5) | A_BOLD | COLOR_PAIR(1));

            if (accion == 2) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 14, sx + 44, " [ Regresar ] "); attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(1));
            
            refresh();
            int ch = getch();
            if (ch == KEY_RIGHT) accion = (accion + 1) % 3;
            else if (ch == KEY_LEFT) accion = (accion - 1 + 3) % 3;
            else if (ch == '\n' || ch == '\r') {
                if (accion == 2) { viendo = 0; } 
                
                else if (accion == 1) { 
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    snprintf(memoria->mensaje, 512, "ADMIN_USER_DEL | Usr: %s", busqueda_usr);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    mostrar_notificacion("Usuario eliminado del sistema correctamente.", 1);
                    viendo = 0; 
                }
                
                else if (accion == 0) { 
                    int editando = 1;
                    int foco = 0;
                    char campos[6][50];
                    strcpy(campos[0], pat); strcpy(campos[1], mat); strcpy(campos[2], nom); 
                    strcpy(campos[3], san); strcpy(campos[4], cor); strcpy(campos[5], pass_raw);
                    const char *t[] = {"Paterno:", "Materno:", "Nombre:", "Sangre:", "Correo:", "Password:"};

                    while(editando) {
                        clear();
                        int e_alto = 22, e_ancho = 64;
                        int ey = (LINES - e_alto) / 2, ex = (COLS - e_ancho) / 2;
                        dibujar_tarjeta(ey, ex, e_alto, e_ancho);
                        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(ey + 2, ex + 20, "=== EDITAR USUARIO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
                        attron(COLOR_PAIR(1)); mvprintw(ey + 4, ex + 5, "Modificando a: %s", busqueda_usr); attroff(COLOR_PAIR(1));

                        for (int i = 0; i < 6; i++) {
                            attron(COLOR_PAIR(1)); mvprintw(ey + 6 + (i * 2), ex + 4, "%-15s", t[i]);
                            if (foco == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
                            mvprintw(ey + 6 + (i * 2), ex + 20, "                              "); 
                            mvprintw(ey + 6 + (i * 2), ex + 21, "%s", campos[i]);
                            attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
                        }
                        
                        if (foco == 6) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                        mvprintw(ey + 19, ex + 13, " [ Guardar Cambios ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

                        if (foco == 7) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                        mvprintw(ey + 19, ex + 38, " [ Cancelar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

                        refresh();
                        int e_ch = getch();
                        if (e_ch == KEY_UP) foco = (foco - 1 + 8) % 8;
                        else if (e_ch == KEY_DOWN) foco = (foco + 1) % 8;
                        else if (e_ch == '\n' || e_ch == '\r') {
                            if (foco == 7) editando = 0; 
                            else if (foco == 6) {
                                char errores[6][100]; int n_err = 0;
                                if (!validar_apellido(campos[0])) strcpy(errores[n_err++], "Paterno: 1 palabra, Inicio Mayuscula");
                                if (!validar_apellido(campos[1])) strcpy(errores[n_err++], "Materno: 1 palabra, Inicio Mayuscula");
                                if (!validar_nombre(campos[2])) strcpy(errores[n_err++], "Nombre: Max 2 palabras, Inicio Mayus");
                                if (!validar_sangre(campos[3])) strcpy(errores[n_err++], "Sangre: Tipos reales (Ej: O+)");
                                if (!validar_correo(campos[4])) strcpy(errores[n_err++], "Correo: Requiere '@' y '.'");
                                if (!validar_password(campos[5])) strcpy(errores[n_err++], "Pass: Min 8, may, min, num, sim");

                                if (n_err > 0) {
                                    mostrar_errores_admin(errores, n_err);
                                } else {
                                    char pass_new_xor[50];
                                    encriptar_ascii(campos[5], pass_new_xor);

                                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                                    snprintf(memoria->mensaje, 512, "ADMIN_USER_UPD | Usr: %s | Paterno: %s| Materno: %s| Nombre: %s| Sangre: %s| Correo: %s| PassXOR: %s", 
                                             busqueda_usr, campos[0], campos[1], campos[2], campos[3], campos[4], pass_new_xor);
                                    
                                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 
                                    
                                    mostrar_notificacion("Usuario actualizado exitosamente.", 1);
                                    editando = 0; viendo = 0; 
                                }
                            } else foco = (foco + 1) % 8;
                        }
                        else if (e_ch == KEY_BACKSPACE || e_ch == 127 || e_ch == '\b') {
                            if (foco < 6) { int len = strlen(campos[foco]); if (len > 0) campos[foco][len-1] = '\0'; }
                        } else if (e_ch >= 32 && e_ch <= 126) {
                            if (foco < 6) { int len = strlen(campos[foco]); if (len < 28) { campos[foco][len] = e_ch; campos[foco][len+1] = '\0'; } }
                        }
                    }
                }
            }
        }
    }
}

// --- MENÚ PRINCIPAL DEL ADMINISTRADOR ---
void abrir_panel_admin(int semid, MemoriaCompartida *memoria) {
    int opcion = 0;
    int ejecutando = 1;
    const char *opciones[4] = {
        "   Gestionar Usuarios del Sistema   ", 
        "   Gestionar Catalogo de Analisis   ", 
        "   Reportes de Ventas (Historial)   ",
        "   Cerrar Sesion                    "
    };

    while(ejecutando) {
        clear();
        int alto = 18, ancho = 55;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 13, "=== PANEL ADMINISTRADOR ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(sy + 4, sx + 22, " MODO DIOS ");
        attroff(COLOR_PAIR(5) | A_BOLD);
        
        mvhline(sy + 6, sx + 2, ACS_HLINE, ancho - 4);

        for (int i = 0; i < 4; i++) {
            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 8 + (i * 2), sx + 9, "%s", opciones[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 8 + (i * 2), sx + 9, "%s", opciones[i]);
                attroff(COLOR_PAIR(1));
            }
        }
        refresh();

        int ch = getch();
        switch(ch) {
            case KEY_UP: opcion = (opcion - 1 + 4) % 4; break;
            case KEY_DOWN: opcion = (opcion + 1) % 4; break;
            case '\n': case '\r':
                if (opcion == 0) {
                    gestionar_usuarios(semid, memoria); 
                } else if (opcion == 1) {
                    mostrar_notificacion("Modulo de Analisis en construccion...", 1);
                } else if (opcion == 2) {
                    mostrar_notificacion("Modulo de Reportes en construccion...", 1);
                } else if (opcion == 3) {
                    ejecutando = 0; 
                }
                break;
        }
    }
}
