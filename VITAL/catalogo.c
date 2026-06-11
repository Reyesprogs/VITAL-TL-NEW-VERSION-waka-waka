#include "catalogo.h"
#include "ui.h"

void abrir_catalogo(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
    strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 

    char respuesta_servidor[512];
    strncpy(respuesta_servidor, memoria->mensaje, 512);
    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 

    // --- LECTURA CORRECTA DE LOS 5 CAMPOS ---
    char categorias[20][50] = {0}, subcats[20][50] = {0};
    char nombres[20][50] = {0}, precios[20][20] = {0}, componentes[20][150] = {0};
    int total_productos = 0;
    
    char *token = strtok(respuesta_servidor, ";"); // Separador estricto
    while (token != NULL && total_productos < 20) {
        if (strlen(token) > 5) {
            sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%s", 
                   categorias[total_productos], subcats[total_productos], 
                   nombres[total_productos], precios[total_productos], componentes[total_productos]);
            total_productos++;
        }
        token = strtok(NULL, ";");
    }

    int opcion = 0;
    int ejecutando = 1;

    while (ejecutando) {
        clear();
        int alto = total_productos + 12, ancho = 75; 
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 25, "=== CATALOGO VITAL-TL ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        attron(COLOR_PAIR(1)); mvprintw(sy + 4, sx + 5, "Selecciona un analisis para agregar a tu carrito:");
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4); attroff(COLOR_PAIR(1));

        for (int i = 0; i < total_productos; i++) {
            if (i == opcion) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            
            // AQUI OCULTAMOS LOS COMPONENTES, SOLO MOSTRAMOS CATEGORIA Y NOMBRE
            if(strcmp(subcats[i], "Paquete") == 0) {
                mvprintw(sy + 7 + i, sx + 4, "%s [%s] %s (Paquete) $%s", 
                         (i == opcion) ? " >" : "  ", categorias[i], nombres[i], precios[i]);
            } else {
                mvprintw(sy + 7 + i, sx + 4, "%s [%s] %s  $%s", 
                         (i == opcion) ? " >" : "  ", categorias[i], nombres[i], precios[i]);
            }
            if (i == opcion) attroff(COLOR_PAIR(2) | A_BOLD); else attroff(COLOR_PAIR(1));
        }

        if (opcion == total_productos) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 9 + total_productos, sx + 32, " [ Salir ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();
        int ch = getch();
        switch(ch) {
            case KEY_UP: opcion = (opcion - 1 + (total_productos + 1)) % (total_productos + 1); break;
            case KEY_DOWN: opcion = (opcion + 1) % (total_productos + 1); break;
            case '\n': case '\r':
                if (opcion == total_productos) ejecutando = 0; 
                else {
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    snprintf(memoria->mensaje, 512, "CARRITO_ADD | Usr: %s | Prod: %s | Prc: %s", 
                             usuario_actual, nombres[opcion], precios[opcion]);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor(); 
                    mostrar_notificacion("Analisis agregado al carrito exitosamente.", 1);
                }
                break;
        }
    }
}
