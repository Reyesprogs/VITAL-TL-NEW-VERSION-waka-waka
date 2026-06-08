#include "resultados.h"
#include "ui.h"

void dibujar_barra(int y, int x, const char* etiqueta, int valor, int max_val) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, "%-15s", etiqueta);
    attroff(COLOR_PAIR(1));
    
    int longitud_max = 20;
    int barras = (valor * longitud_max) / max_val;
    if (barras > longitud_max) barras = longitud_max;

    attron(COLOR_PAIR(2) | A_BOLD);
    for(int i = 0; i < barras; i++) mvaddch(y, x + 16 + i, ACS_BLOCK); // Dibuja la barra
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    attron(COLOR_PAIR(1));
    mvprintw(y, x + 17 + longitud_max, "[ %3d ]", valor);
    attroff(COLOR_PAIR(1));
}

void mostrar_graficas(const char *usuario_actual, const char *tipo, int semid, MemoriaCompartida *memoria) {
    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
    snprintf(memoria->mensaje, 512, "HISTORIAL_REQ | Usr: %s | Tipo: %s", usuario_actual, tipo);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
    char respuesta[512];
    strncpy(respuesta, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor();

    clear();
    int alto = 18, ancho = 60;
    int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
    dibujar_tarjeta(sy, sx, alto, ancho);

    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(sy + 2, sx + 15, "=== RESULTADOS: %s ===", tipo);
    attroff(COLOR_PAIR(3) | A_BOLD);

    if (strcmp(respuesta, "VACIO") == 0) {
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(sy + 8, sx + 15, " No hay historial registrado ");
        attroff(COLOR_PAIR(5) | A_BOLD);
    } else {
        // Parsear los datos separados por "|"
        char *token = strtok(respuesta, "|");
        int linea = 0;
        while (token != NULL && linea < 8) {
            char parametro[30]; int valor;
            sscanf(token, "%[^:]:%d", parametro, &valor);
            
            if (strcmp(tipo, "SANGRE") == 0 || strcmp(tipo, "ORINA") == 0) {
                dibujar_barra(sy + 5 + (linea * 2), sx + 5, parametro, valor, 200);
            } else {
                // Para pruebas Si/No (Sin gráfica)
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(sy + 5 + (linea * 2), sx + 10, "> %-20s : ", parametro);
                if (valor == 0) {
                    attron(COLOR_PAIR(2)); mvprintw(sy + 5 + (linea * 2), sx + 35, "NEGATIVO"); attroff(COLOR_PAIR(2));
                } else {
                    attron(COLOR_PAIR(5)); mvprintw(sy + 5 + (linea * 2), sx + 35, "POSITIVO"); attroff(COLOR_PAIR(5));
                }
                attroff(COLOR_PAIR(1) | A_BOLD);
            }
            linea++;
            token = strtok(NULL, "|");
        }
    }

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(sy + 15, sx + 25, " [ Salir ] ");
    attroff(COLOR_PAIR(1) | A_BOLD);
    refresh();
    while(getch() != '\n') {}
}

void abrir_resultados(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    int opcion = 0, ejecutando = 1;
    const char *opciones[5] = {
        "   Ultimos Resultados (Pendientes)   ", 
        "   Historial de Sangre (Graficas)    ", 
        "   Historial de Orina (Graficas)     ",
        "   Pruebas de Infeccion (Si/No)      ",
        "   Regresar                          "
    };

    while (ejecutando) {
        clear();
        int alto = 18, ancho = 55;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 18, "=== MIS RESULTADOS ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < 5; i++) {
            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 6 + (i * 2), sx + 8, "%s", opciones[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 6 + (i * 2), sx + 8, "%s", opciones[i]);
                attroff(COLOR_PAIR(1));
            }
        }
        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_UP: opcion = (opcion - 1 + 5) % 5; break;
            case KEY_DOWN: opcion = (opcion + 1) % 5; break;
            case '\n': case '\r':
                if (opcion == 0) {
                    // Pide los resultados pendientes al servidor
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    snprintf(memoria->mensaje, 512, "RESULTADOS_REQ | Usr: %s", usuario_actual);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    char resp[512]; strncpy(resp, memoria->mensaje, 512);
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor();

                    if(strcmp(resp, "VACIO") == 0) mostrar_notificacion("No tienes analisis pendientes.", 0);
                    else mostrar_notificacion("Analisis Pendientes encontrados. (Revisa el archivo de resultados)", 1);
                }
                else if (opcion == 1) mostrar_graficas(usuario_actual, "SANGRE", semid, memoria);
                else if (opcion == 2) mostrar_graficas(usuario_actual, "ORINA", semid, memoria);
                else if (opcion == 3) mostrar_graficas(usuario_actual, "SINO", semid, memoria);
                else if (opcion == 4) ejecutando = 0;
                break;
        }
    }
}
