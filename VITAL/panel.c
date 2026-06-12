#include "panel.h"
#include "ui.h"
#include "inter.h"
#include "catalogo.h"
#include "carrito.h"
#include "perfil.h"

// Declaración externa de la nueva función en resultados.c para no tener que modificar cabeceras
extern void abrir_seccion_resultados(char *usuario, int semid, MemoriaCompartida *memoria);

void abrir_panel_principal(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    int opcion = 0;
    int ejecutando_panel = 1;
    
    // Agregamos "Mis Resultados" haciendo que ahora sean 5 opciones
    const char *opciones[5] = {
        "   Catalogo de Analisis   ", 
        "    Carrito de Compras    ", 
        "    Mis Resultados        ", // <--- NUEVA OPCION ENLAZADA
        "    Mi Perfil             ",
        "    Cerrar Sesion         "
    };

    while(ejecutando_panel) {
        clear();
        int alto = 20, ancho = 50; // Se ajustó a 20 para que la nueva opción no quede muy pegada al borde
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(sy + 2, sx + 14, "=== PANEL PRINCIPAL ===");
        attroff(COLOR_PAIR(3) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 5, "Bienvenido: %s", usuario_actual);
        mvhline(sy + 6, sx + 2, ACS_HLINE, ancho - 4);
        attroff(COLOR_PAIR(1));

        for (int i = 0; i < 5; i++) {
            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 8 + (i * 2), sx + 12, "%s", opciones[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 8 + (i * 2), sx + 12, "%s", opciones[i]);
                attroff(COLOR_PAIR(1));
            }
        }
        refresh();

        int ch = getch();
        switch(ch) {
            case KEY_UP: opcion = (opcion - 1 + 5) % 5; break;
            case KEY_DOWN: opcion = (opcion + 1) % 5; break;
            case '\n': case '\r':
                if (opcion == 0) {
                    abrir_catalogo(usuario_actual, semid, memoria);
                } else if (opcion == 1) {
                    abrir_carrito(usuario_actual, semid, memoria);
                } else if (opcion == 2) {
                    // Llamamos a resultados haciendo un cast (char *) para evitar warnings de GCC por el "const"
                    abrir_seccion_resultados((char *)usuario_actual, semid, memoria);
                } else if (opcion == 3) {
                    abrir_perfil(usuario_actual, semid, memoria);
                } else if (opcion == 4) {
                    ejecutando_panel = 0;
                }
                break;
        }
    }
}
