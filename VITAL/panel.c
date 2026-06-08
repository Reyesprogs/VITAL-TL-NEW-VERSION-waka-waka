#include "panel.h"
#include "ui.h"
#include "inter.h"
#include "catalogo.h"

void abrir_panel_principal(const char *usuario_actual, int semid, MemoriaCompartida *memoria) {
    int opcion = 0;
// ... (el resto de tu codigo sigue igual)
    int ejecutando_panel = 1;
    const char *opciones[5] = {
        "   Catalogo de Analisis   ", 
        "    Carrito de Compras    ", 
        "    Mis Resultados        ",
        "    Mi Perfil             ",
        "    Cerrar Sesion         "
    };

    while(ejecutando_panel) {
        clear();
        int alto = 18, ancho = 50;
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
                    mostrar_notificacion("Abriendo Carrito...", 1);
                    // abrir_carrito(semid, memoria);
                } else if (opcion == 2) {
                    mostrar_notificacion("Abriendo Resultados y Graficas...", 1);
                    // abrir_resultados(semid, memoria);
                } else if (opcion == 3) {
                    mostrar_notificacion("Abriendo Modificacion de Perfil...", 1);
                    // abrir_perfil(semid, memoria);
                } else if (opcion == 4) {
                    ejecutando_panel = 0; // Rompe el ciclo y regresa al menú de Login
                }
                break;
        }
    }
}
