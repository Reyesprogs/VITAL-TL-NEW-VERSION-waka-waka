#include "admin.h"
#include "ui.h"
#include <string.h>

void gestionar_catalogo(int semid, MemoriaCompartida *memoria) {
    int ejecutando = 1;
    while(ejecutando) {
        // 1. Pedir catálogo al servidor
        if (down(semid, MUTEX) == -1) mostrar_error_servidor();
        strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();

        char respuesta[512];
        strncpy(respuesta, memoria->mensaje, 512);
        if (up(semid, MUTEX) == -1) mostrar_error_servidor();

        // 2. Parsear catálogo
        char productos[20][50];
        char nombres[20][30];
        int precios[20];
        int total = 0;
        char *token = strtok(respuesta, ",");
        while (token != NULL && total < 20) {
            strcpy(productos[total], token);
            sscanf(token, "%[^|]|%d", nombres[total], &precios[total]);
            total++;
            token = strtok(NULL, ",");
        }

        // 3. UI
        int foco = 0;
        int sub_ejecutando = 1;
        while (sub_ejecutando) {
            clear();
            int alto = 18, ancho = 60;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);
            
            attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 15, "=== GESTIONAR CATALOGO ==="); attroff(COLOR_PAIR(3) | A_BOLD);

            for (int i = 0; i < total; i++) {
                if (foco == i) attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 4 + i, sx + 5, "%-20s $%d  [Del]", nombres[i], precios[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            }

            if (foco == total) attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(sy + 15, sx + 25, " [ Regresar ] ");
            attroff(COLOR_PAIR(2) | A_BOLD);

            refresh();
            int ch = getch();
            if (ch == KEY_UP) foco = (foco - 1 + total + 1) % (total + 1);
            else if (ch == KEY_DOWN) foco = (foco + 1) % (total + 1);
            else if (ch == '\n') {
                if (foco == total) sub_ejecutando = 0;
                else {
                    // Acción: Eliminar
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                    snprintf(memoria->mensaje, 512, "ADMIN_CAT_DEL | Prod: %s", nombres[foco]);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                    mostrar_notificacion("Analisis eliminado.", 1);
                    sub_ejecutando = 0; // Recargar lista
                }
            }
        }
    }
}
