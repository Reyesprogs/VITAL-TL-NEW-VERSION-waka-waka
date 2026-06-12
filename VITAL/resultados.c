#include "inter.h"
#include "resultados.h"
#include "ui.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

void limpiar_espacios_extremos(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
}

void mostrar_ultimos_resultados(char *usuario, int semid, MemoriaCompartida *memoria) {
    int ejecutando = 1, foco = 0;
    char lineas[20][256] = {0};
    int total_lineas = 0;

    if (down(semid, MUTEX) == -1) mostrar_error_servidor();
    snprintf(memoria->mensaje, 512, "USER_RESULTS_DATA_REQ | Usr: %s", usuario);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
    char resp[512]; strncpy(resp, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor();

    if (strcmp(resp, "VACIO") != 0) {
        char *token = strtok(resp, ";");
        while (token && total_lineas < 20) {
            strncpy(lineas[total_lineas++], token, 255);
            token = strtok(NULL, ";");
        }
    }

    while (ejecutando) {
        clear();
        int alto = total_lineas + 12; if (alto < 16) alto = 16;
        int ancho = 78;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 26, "=== RESULTADOS NUEVOS ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        
        if (total_lineas == 0) {
            attron(COLOR_PAIR(1)); mvprintw(sy + 6, sx + 24, "No tienes analisis listos."); attroff(COLOR_PAIR(1));
        } else {
            attron(COLOR_PAIR(1)); mvprintw(sy + 4, sx + 4, "Selecciona para ver el desglose detallado:"); attroff(COLOR_PAIR(1));
            mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);

            for (int i = 0; i < total_lineas; i++) {
                char p_name[50] = {0}, f_str[20] = {0};
                char *ptr_p = strstr(lineas[i], "Prod: ");
                char *ptr_f = strstr(lineas[i], "Fecha: ");
                
                // CORRECCIÓN: Evitamos la fragmentación por espacios usando %[^|]
                if (ptr_p) { sscanf(ptr_p + 6, "%[^|]", p_name); limpiar_espacios_extremos(p_name); }
                if (ptr_f) { sscanf(ptr_f + 7, "%[^|]", f_str); limpiar_espacios_extremos(f_str); }

                if (foco == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                mvprintw(sy + 6 + i, sx + 4, "%s Fecha: %s | %-35s", (foco == i) ? " >" : "  ", f_str, p_name);
                if (foco == i) attroff(COLOR_PAIR(2) | A_BOLD); else attroff(COLOR_PAIR(1));
            }
        }

        if (foco == total_lineas) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + alto - 3, sx + 32, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + total_lineas + 1) % (total_lineas + 1);
        else if (ch == KEY_DOWN) foco = (foco + 1) % (total_lineas + 1);
        else if (ch == '\n' || ch == '\r') {
            if (foco == total_lineas) { ejecutando = 0; }
            else if (total_lineas > 0) {
                int mirando = 1;
                while (mirando) {
                    clear();
                    int pop_alto = 16, pop_ancho = 65;
                    int py = (LINES - pop_alto) / 2, px = (COLS - pop_ancho) / 2;
                    dibujar_tarjeta(py, px, pop_alto, pop_ancho);

                    char p_name[50] = {0}, data_str[256] = {0};
                    char *ptr_p = strstr(lineas[foco], "Prod: ");
                    char *ptr_d = strstr(lineas[foco], "Data: ");
                    if (ptr_p) { sscanf(ptr_p + 6, "%[^|]", p_name); limpiar_espacios_extremos(p_name); }
                    if (ptr_d) sscanf(ptr_d + 6, "%[^\n]", data_str);

                    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(py + 2, px + 18, "=== DETALLE DE DIAGNOSTICO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
                    attron(COLOR_PAIR(1)); mvprintw(py + 4, px + 4, "Analisis: %s", p_name); mvhline(py + 5, px + 2, ACS_HLINE, pop_ancho - 4);

                    if (strchr(data_str, ':')) {
                        char temp[256]; strcpy(temp, data_str);
                        char *sub_tok = strtok(temp, ",");
                        int fila = 0;
                        while (sub_tok) {
                            char c_nom[50] = {0}, c_val[20] = {0};
                            sscanf(sub_tok, "%[^:]:%s", c_nom, c_val);
                            mvprintw(py + 7 + fila, px + 6, "%-25s : %s", c_nom, c_val);
                            fila++; sub_tok = strtok(NULL, ",");
                        }
                    } else {
                        mvprintw(py + 7, px + 6, "Resultado General : ");
                        if (strcmp(data_str, "POSITIVO") == 0) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(2) | A_BOLD);
                        printw("%s", data_str);
                        attroff(COLOR_PAIR(5) | COLOR_PAIR(2) | A_BOLD);
                    }
                    attroff(COLOR_PAIR(1));

                    attron(COLOR_PAIR(2) | A_BOLD); mvprintw(py + pop_alto - 3, px + 24, " [ Ok (Enter) ] "); attroff(COLOR_PAIR(2) | A_BOLD);
                    refresh();
                    int p_ch = getch();
                    if (p_ch == '\n' || p_ch == '\r') mirando = 0;
                }
            }
        }
    }
}

void mostrar_historial_medico(char *usuario, int semid, MemoriaCompartida *memoria) {
    char lineas[20][256] = {0}; int total_lineas = 0;

    if (down(semid, MUTEX) == -1) mostrar_error_servidor();
    snprintf(memoria->mensaje, 512, "USER_RESULTS_DATA_REQ | Usr: %s", usuario);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
    char resp[512]; strncpy(resp, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor();

    if (strcmp(resp, "VACIO") == 0) {
        mostrar_notificacion("No cuentas con historial clinico aun.", 0); return;
    }

    char *token = strtok(resp, ";");
    while (token && total_lineas < 20) { strncpy(lineas[total_lineas++], token, 255); token = strtok(NULL, ";"); }

    char anal_unicos[30][50] = {0}; int total_unicos = 0;
    for (int i = 0; i < total_lineas; i++) {
        char data_str[256] = {0}, p_name[50] = {0};
        char *ptr_d = strstr(lineas[i], "Data: ");
        char *ptr_p = strstr(lineas[i], "Prod: ");
        if (ptr_d) sscanf(ptr_d + 6, "%[^\n]", data_str);
        // CORRECCIÓN: Cambiado de %[^ |] a %[^|] para levantar el nombre entero con espacios
        if (ptr_p) { sscanf(ptr_p + 6, "%[^|]", p_name); limpiar_espacios_extremos(p_name); }

        if (strchr(data_str, ':')) {
            char temp[256]; strcpy(temp, data_str);
            char *sub_tok = strtok(temp, ",");
            while (sub_tok) {
                char c_nom[50] = {0}; sscanf(sub_tok, "%[^:]", c_nom);
                char *start = c_nom; while (*start == ' ') start++;
                limpiar_espacios_extremos(start);

                int existe = 0;
                for (int k = 0; k < total_unicos; k++) {
                    if (strcmp(anal_unicos[k], start) == 0) { existe = 1; break; }
                }
                if (!existe && total_unicos < 30 && strlen(start) > 0) {
                    strcpy(anal_unicos[total_unicos++], start);
                }
                sub_tok = strtok(NULL, ",");
            }
        } else {
            limpiar_espacios_extremos(p_name);
            int existe = 0;
            for (int k = 0; k < total_unicos; k++) {
                if (strcmp(anal_unicos[k], p_name) == 0) { existe = 1; break; }
            }
            if (!existe && total_unicos < 30 && strlen(p_name) > 0) {
                strcpy(anal_unicos[total_unicos++], p_name);
            }
        }
    }

    int foco = 0, ejecutando = 1;
    while (ejecutando) {
        clear();
        int alto = total_unicos + 12; if (alto < 16) alto = 16;
        int ancho = 60;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 18, "=== HISTORIAL MEDICO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        attron(COLOR_PAIR(1)); mvprintw(sy + 4, sx + 4, "Selecciona un componente para ver grafico:"); attroff(COLOR_PAIR(1));
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);

        for (int i = 0; i < total_unicos; i++) {
            if (foco == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 6 + i, sx + 4, "%s %s", (foco == i) ? " >" : "  ", anal_unicos[i]);
            if (foco == i) attroff(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        }

        if (foco == total_unicos) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + alto - 3, sx + 23, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + total_unicos + 1) % (total_unicos + 1);
        else if (ch == KEY_DOWN) foco = (foco + 1) % (total_unicos + 1);
        else if (ch == '\n' || ch == '\r') {
            if (foco == total_unicos) { ejecutando = 0; }
            else {
                int viendo_grafica = 1;
                char h_fechas[20][20] = {0}, h_textos[20][20] = {0};
                double h_valores[20] = {0};
                int h_total = 0, es_num = 0; double max_val = 0.0;

                for (int i = 0; i < total_lineas; i++) {
                    char data_str[256] = {0}, p_name[50] = {0}, f_str[20] = {0};
                    char *ptr_d = strstr(lineas[i], "Data: ");
                    char *ptr_p = strstr(lineas[i], "Prod: ");
                    char *ptr_f = strstr(lineas[i], "Fecha: ");
                    if (ptr_d) sscanf(ptr_d + 6, "%[^\n]", data_str);
                    
                    // CORRECCIÓN: Mismo ajuste de lectura completa de nombre para graficación
                    if (ptr_p) { sscanf(ptr_p + 6, "%[^|]", p_name); limpiar_espacios_extremos(p_name); }
                    if (ptr_f) { sscanf(ptr_f + 7, "%[^|]", f_str); limpiar_espacios_extremos(f_str); }

                    if (strchr(data_str, ':')) {
                        char temp[256]; strcpy(temp, data_str);
                        char *sub_tok = strtok(temp, ",");
                        while (sub_tok) {
                            char c_nom[50] = {0}, c_val[20] = {0};
                            sscanf(sub_tok, "%[^:]:%s", c_nom, c_val);
                            char *start = c_nom; while (*start == ' ') start++;
                            limpiar_espacios_extremos(start);

                            if (strcmp(start, anal_unicos[foco]) == 0) {
                                strcpy(h_fechas[h_total], f_str);
                                h_valores[h_total] = atof(c_val);
                                if (h_valores[h_total] > max_val) max_val = h_valores[h_total];
                                h_total++; es_num = 1; break;
                            }
                            sub_tok = strtok(NULL, ",");
                        }
                    } else {
                        limpiar_espacios_extremos(p_name);
                        if (strcmp(p_name, anal_unicos[foco]) == 0) {
                            strcpy(h_fechas[h_total], f_str);
                            limpiar_espacios_extremos(data_str);
                            strcpy(h_textos[h_total], data_str);
                            h_total++; es_num = 0;
                        }
                    }
                }

                while (viendo_grafica) {
                    clear();
                    int g_alto = 16 + h_total, g_ancho = 78;
                    int gy = (LINES - g_alto) / 2, gx = (COLS - g_ancho) / 2;
                    dibujar_tarjeta(gy, gx, g_alto, g_ancho);

                    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(gy + 2, gx + 22, "=== COMPARATIVA DE EVOLUCION ==="); attroff(COLOR_PAIR(3) | A_BOLD);
                    attron(COLOR_PAIR(1)); mvprintw(gy + 4, gx + 4, "Historial de: %s", anal_unicos[foco]);
                    mvhline(gy + 5, gx + 2, ACS_HLINE, g_ancho - 4);

                    if (es_num) {
                        if (max_val == 0.0) max_val = 1.0;
                        for (int i = 0; i < h_total; i++) {
                            mvprintw(gy + 7 + i, gx + 4, "%s | ", h_fechas[i]);
                            int tam_barra = (int)((h_valores[i] / max_val) * 30);
                            attron(COLOR_PAIR(2));
                            for (int b = 0; b < tam_barra; b++) printw("#");
                            attroff(COLOR_PAIR(2));
                            attron(COLOR_PAIR(1) | A_BOLD); printw(" [%.2f]", h_valores[i]); attroff(COLOR_PAIR(1) | A_BOLD);
                        }
                    } else {
                        for (int i = 0; i < h_total; i++) {
                            mvprintw(gy + 7 + i, gx + 4, "%s | Estado Clinico: ", h_fechas[i]);
                            if (strcmp(h_textos[i], "POSITIVO") == 0) {
                                attron(COLOR_PAIR(5) | A_BOLD); printw("[ POSITIVO ]"); attroff(COLOR_PAIR(5) | A_BOLD);
                            } else {
                                attron(COLOR_PAIR(2) | A_BOLD); printw("[ NEGATIVO ]"); attroff(COLOR_PAIR(2) | A_BOLD);
                            }
                        }
                    }
                    attroff(COLOR_PAIR(1));

                    attron(COLOR_PAIR(2) | A_BOLD); mvprintw(gy + g_alto - 3, gx + 31, " [ Volver ] "); attroff(COLOR_PAIR(2) | A_BOLD);
                    refresh();
                    int g_ch = getch(); if (g_ch == '\n' || g_ch == '\r') viendo_grafica = 0;
                }
            }
        }
    }
}

void abrir_seccion_resultados(char *usuario, int semid, MemoriaCompartida *memoria) {
    int opcion = 0, ejecutando = 1;
    const char *opciones[3] = {
        "   Resultados Nuevos                ",
        "   Ver Historial Medico             ",
        "   Regresar al Panel                "
    };

    while (ejecutando) {
        clear();
        int alto = 16, ancho = 52;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 14, "=== MIS RESULTADOS ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);

        for (int i = 0; i < 3; i++) {
            if (i == opcion) { attron(COLOR_PAIR(2) | A_BOLD); mvprintw(sy + 7 + (i * 2), sx + 8, "%s", opciones[i]); attroff(COLOR_PAIR(2) | A_BOLD); } 
            else { attron(COLOR_PAIR(1)); mvprintw(sy + 7 + (i * 2), sx + 8, "%s", opciones[i]); attroff(COLOR_PAIR(1)); }
        }
        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP: opcion = (opcion - 1 + 3) % 3; break;
            case KEY_DOWN: opcion = (opcion + 1) % 3; break;
            case '\n': case '\r':
                if (opcion == 0) mostrar_ultimos_resultados(usuario, semid, memoria);
                else if (opcion == 1) mostrar_historial_medico(usuario, semid, memoria);
                else if (opcion == 2) ejecutando = 0;
                break;
        }
    }
}
