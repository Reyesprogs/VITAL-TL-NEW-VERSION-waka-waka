#include "perfil.h"
#include "ui.h"
#include "validaciones.h"

// Función auxiliar para quitar espacios en blanco al final de los textos
void recortar_espacios(char *str) {
    int len = strlen(str);
    while (len > 0 && str[len-1] == ' ') {
        str[len-1] = '\0';
        len--;
    }
}

// --- NUEVA FUNCION: Mostrar errores específicos del perfil ---
void mostrar_errores_perfil(char errores[5][100], int n_errores) {
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

void abrir_perfil(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    int editando = 0;
    char pat_actual[50] = "---";
    char mat_actual[50] = "---";
    char nom_actual[50] = "---";
    char san_actual[20] = "---";
    char cor_actual[50] = "---";

    // --- 1. PEDIR DATOS ACTUALES AL SERVIDOR ---
    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
    snprintf(memoria->mensaje, 512, "PERFIL_REQ | Usr: %s", usuario_actual);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
    
    char respuesta[512];
    strncpy(respuesta, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor();

    if (strcmp(respuesta, "VACIO") != 0) {
        char *p_pat = strstr(respuesta, "Paterno: ");
        char *p_mat = strstr(respuesta, "Materno: ");
        char *p_nom = strstr(respuesta, "Nombre: ");
        char *p_san = strstr(respuesta, "Sangre: ");
        char *p_cor = strstr(respuesta, "Correo: ");

        if (p_pat) { sscanf(p_pat + 9, "%[^|]", pat_actual); recortar_espacios(pat_actual); }
        if (p_mat) { sscanf(p_mat + 9, "%[^|]", mat_actual); recortar_espacios(mat_actual); }
        if (p_nom) { sscanf(p_nom + 8, "%[^|]", nom_actual); recortar_espacios(nom_actual); }
        if (p_san) { sscanf(p_san + 8, "%[^|]", san_actual); recortar_espacios(san_actual); }
        if (p_cor) { sscanf(p_cor + 8, "%[^|]", cor_actual); recortar_espacios(cor_actual); }
    }

    // --- 2. PANTALLA DE SOLO LECTURA ---
    int opcion_ro = 0;
    while (!editando) {
        clear();
        int alto = 18, ancho = 64;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 24, "=== MI PERFIL ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 6, "Usuario:  %s", usuario_actual);
        mvprintw(sy + 5, sx + 6, "Paterno:  %s", pat_actual);
        mvprintw(sy + 6, sx + 6, "Materno:  %s", mat_actual);
        mvprintw(sy + 7, sx + 6, "Nombre:   %s", nom_actual);
        mvprintw(sy + 8, sx + 6, "Sangre:   %s", san_actual);
        mvprintw(sy + 9, sx + 6, "Correo:   %s", cor_actual);
        attroff(COLOR_PAIR(1));

        if (opcion_ro == 0) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 13, sx + 14, " [ Editar Datos ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (opcion_ro == 1) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 13, sx + 38, " [ Salir ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_RIGHT || ch == KEY_LEFT || ch == KEY_UP || ch == KEY_DOWN) {
            opcion_ro = (opcion_ro + 1) % 2;
        }
        else if (ch == '\n' || ch == '\r') {
            if (opcion_ro == 1) return; 
            else if (opcion_ro == 0) editando = 1; 
        }
    }

    // --- 3. PANTALLA DE EDICION ---
    int foco = 0; 
    char campos[5][50] = {0};
    
    // PRE-LLENADO
    strcpy(campos[0], pat_actual);
    strcpy(campos[1], mat_actual);
    strcpy(campos[2], nom_actual);
    strcpy(campos[3], san_actual);
    strcpy(campos[4], cor_actual);

    const char *titulos[] = {"Paterno:", "Materno:", "Nombre:", "Sangre:", "Correo:"};

    while (1) {
        clear();
        int alto = 20, ancho = 60;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 18, "=== EDITAR MI PERFIL ===");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 5, "Usuario Fijo: %s", usuario_actual);
        attroff(COLOR_PAIR(1));

        for (int i = 0; i < 5; i++) {
            attron(COLOR_PAIR(1));
            mvprintw(sy + 6 + (i * 2), sx + 4, "%-15s", titulos[i]);
            
            if (foco == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
            mvprintw(sy + 6 + (i * 2), sx + 20, "                              "); 
            mvprintw(sy + 6 + (i * 2), sx + 21, "%s", campos[i]);
            attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
        }

        if (foco == 5) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 17, sx + 11, " [ Guardar Cambios ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (foco == 6) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 17, sx + 36, " [ Cancelar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + 7) % 7; 
        else if (ch == KEY_DOWN) foco = (foco + 1) % 7; 
        else if (ch == '\n' || ch == '\r') {
            if (foco == 6) return; // Cancelar
            else if (foco == 5) {
                
                // --- NUEVAS VALIDACIONES ESTRICTAS ---
                char errores[5][100];
                int n_err = 0;

                if (!validar_apellido(campos[0])) strcpy(errores[n_err++], "Paterno: 1 palabra, Inicio Mayuscula");
                if (!validar_apellido(campos[1])) strcpy(errores[n_err++], "Materno: 1 palabra, Inicio Mayuscula");
                if (!validar_nombre(campos[2])) strcpy(errores[n_err++], "Nombre: Max 2 palabras, Inicio Mayuscula");
                if (!validar_sangre(campos[3])) strcpy(errores[n_err++], "Sangre: Solo tipos reales y su signo (Ej: O+)");
                if (!validar_correo(campos[4])) strcpy(errores[n_err++], "Correo: Requiere '@' y '.'");

                if (n_err > 0) {
                    // Si algo falla, mostramos la tarjeta gris de correcciones
                    mostrar_errores_perfil(errores, n_err);
                } else {
                    // Si todo está perfecto, mandamos al servidor
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    
                    snprintf(memoria->mensaje, 512, "PERFIL_UPDATE | Usr: %s | Paterno: %s| Materno: %s| Nombre: %s| Sangre: %s| Correo: %s", 
                             usuario_actual, campos[0], campos[1], campos[2], campos[3], campos[4]);
                    
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

                    mostrar_notificacion("Perfil actualizado exitosamente.", 1);
                    return; 
                }
            } else {
                foco = (foco + 1) % 7; 
            }
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (foco < 5) { 
                int len = strlen(campos[foco]);
                if (len > 0) campos[foco][len-1] = '\0';
            }
        } 
        else if (ch >= 32 && ch <= 126) {
            if (foco < 5) { 
                int len = strlen(campos[foco]);
                if (len < 28) { campos[foco][len] = ch; campos[foco][len+1] = '\0'; }
            }
        }
    }
}
