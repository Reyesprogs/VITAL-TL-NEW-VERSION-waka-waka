#include "carrito.h"
#include "ui.h"

void abrir_carrito(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    int ejecutando = 1;
    int foco = 0; 

    while (ejecutando) {
        // 1. Pedirle el Carrito Actual al servidor
        if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
        snprintf(memoria->mensaje, 512, "CARRITO_REQ | Usr: %s", usuario_actual);
        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 

        char respuesta[512];
        strncpy(respuesta, memoria->mensaje, 512);
        if (up(semid, MUTEX) == -1) mostrar_error_servidor();

        // 2. Parsear y sumar precios
        char productos[20][50];
        int precios[20];
        int total_items = 0, total_pagar = 0;

        if (strcmp(respuesta, "VACIO") != 0 && strlen(respuesta) > 0) {
            char *token = strtok(respuesta, ",");
            while (token != NULL && strlen(token) > 2 && total_items < 20) {
                char nombre[50], prc_str[20];
                sscanf(token, "%[^|]|%s", nombre, prc_str);
                strcpy(productos[total_items], nombre);
                precios[total_items] = atoi(prc_str);
                total_pagar += precios[total_items];
                total_items++;
                token = strtok(NULL, ",");
            }
        }

        int opciones_totales = total_items + 2; 
        if (foco >= opciones_totales) foco = 0;

        int recargar = 0;
        // 3. Interfaz Visual
        while (!recargar && ejecutando) {
            clear();
            int alto = 22, ancho = 64;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);

            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(sy + 2, sx + 22, "=== MI CARRITO ===");
            attroff(COLOR_PAIR(3) | A_BOLD);

            if (total_items == 0) {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(sy + 5, sx + 18, " Tu carrito esta vacio ");
                attroff(COLOR_PAIR(5) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 4, sx + 4, "Presiona ENTER sobre un analisis para ELIMINARLO:");
                mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);
                attroff(COLOR_PAIR(1));

                for (int i = 0; i < total_items; i++) {
                    if (i == foco) {
                        attron(COLOR_PAIR(5) | A_BOLD); 
                        mvprintw(sy + 6 + i, sx + 6, " [ BORRAR ] %-25s $%d ", productos[i], precios[i]);
                        attroff(COLOR_PAIR(5) | A_BOLD);
                    } else {
                        attron(COLOR_PAIR(1));
                        mvprintw(sy + 6 + i, sx + 6, "            %-25s $%d ", productos[i], precios[i]);
                        attroff(COLOR_PAIR(1));
                    }
                }
            }

            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(sy + 16, sx + 35, "TOTAL: $%d", total_pagar);
            attroff(COLOR_PAIR(2) | A_BOLD);

            if (foco == total_items) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 18, sx + 15, " [ Pagar Ahora ] ");
            attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            if (foco == total_items + 1) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 18, sx + 35, " [ Regresar ] ");
            attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            refresh();

            int ch = getch();
            switch (ch) {
                case KEY_UP: foco = (foco - 1 + opciones_totales) % opciones_totales; break;
                case KEY_DOWN: foco = (foco + 1) % opciones_totales; break;
                case '\n': case '\r':
                    if (foco < total_items) {
                        // --- ENVIAR ORDEN DE BORRADO ---
                        if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                        snprintf(memoria->mensaje, 512, "CARRITO_DEL | Usr: %s | Prod: %s", usuario_actual, productos[foco]);
                        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                        if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 
                        
                        mostrar_notificacion("Analisis eliminado de tu carrito.", 1);
                        recargar = 1; // Recarga la lista desde el servidor
                    } else if (foco == total_items) {
                        if (total_items == 0) {
                            mostrar_notificacion("No tienes analisis para pagar", 0);
                        } else {
                            // --- ENVIAR ORDEN DE PAGO ---
                            if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                            snprintf(memoria->mensaje, 512, "CARRITO_PAY | Usr: %s", usuario_actual);
                            if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                            if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                            if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 
                            
                            mostrar_notificacion("Pago Exitoso. Analisis mandados al Doctor (En Espera)", 1);
                            ejecutando = 0; // Regresa al menú principal
                            recargar = 1;
                        }
                    } else if (foco == total_items + 1) {
                        ejecutando = 0; 
                        recargar = 1;
                    }
                    break;
            }
        }
    }
}
