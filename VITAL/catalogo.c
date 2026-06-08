#include "catalogo.h"
#include "ui.h"

void abrir_catalogo(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    // 1. Pedirle el catálogo al servidor
    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
    strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 

    char respuesta_servidor[512];
    strncpy(respuesta_servidor, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

    // 2. Separar los productos recibidos
    char productos[10][100];
    int total_productos = 0;
    char *token = strtok(respuesta_servidor, ",");
    while (token != NULL && total_productos < 10) {
        strcpy(productos[total_productos], token);
        total_productos++;
        token = strtok(NULL, ",");
    }

    // 3. Interfaz Gráfica
    int opcion = 0;
    int ejecutando = 1;

    while (ejecutando) {
        clear();
        int alto = 18, ancho = 60;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 18, "=== CATALOGO VITAL-TL ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 5, "Selecciona un analisis para agregar a tu carrito:");
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);
        attroff(COLOR_PAIR(1));

        // Dibujar la lista de productos dinámica
        for (int i = 0; i < total_productos; i++) {
            char nombre[50], precio[20];
            sscanf(productos[i], "%[^|]|%s", nombre, precio); // Separar nombre y precio

            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 7 + i, sx + 8, " > %-25s $%s ", nombre, precio);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 7 + i, sx + 8, "   %-25s $%s ", nombre, precio);
                attroff(COLOR_PAIR(1));
            }
        }

        // Botón de salir
        if (opcion == total_productos) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 15, sx + 25, " [ Salir ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();
        int ch = getch();
        switch(ch) {
            case KEY_UP: opcion = (opcion - 1 + (total_productos + 1)) % (total_productos + 1); break;
            case KEY_DOWN: opcion = (opcion + 1) % (total_productos + 1); break;
            case '\n': case '\r':
                if (opcion == total_productos) {
                    ejecutando = 0; // Regresar al panel principal
                } else {
                    // --- AGREGAR AL CARRITO ---
                    char nombre_prod[50], precio_prod[20];
                    sscanf(productos[opcion], "%[^|]|%s", nombre_prod, precio_prod);

                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    // Empacamos el comando para el servidor
                    snprintf(memoria->mensaje, 512, "CARRITO_ADD | Usr: %s | Prod: %s | Prc: %s", 
                             usuario_actual, nombre_prod, precio_prod);
                    
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

                    mostrar_notificacion("Analisis agregado al carrito exitosamente.", 1);
                }
                break;
        }
    }
}
