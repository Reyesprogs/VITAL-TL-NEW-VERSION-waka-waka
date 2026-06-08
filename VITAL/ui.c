#include "ui.h"

void configurar_colores() {
    start_color();
    if (can_change_color()) {
        init_color(10, 850, 850, 850); 
        init_color(11, 250, 250, 250); 
        init_color(12, 100, 600, 900); 
        init_color(13, 350, 350, 350); 
        init_color(14, 800, 800, 800); 

        init_pair(1, 11, 10); 
        init_pair(2, 10, 12); 
        init_pair(3, 12, 10); 
        init_pair(4, 14, 13); 
        init_pair(5, 14, 13); 
    } else {
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_CYAN);
        init_pair(3, COLOR_CYAN, COLOR_WHITE);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_RED, COLOR_WHITE);
    }
}

void dibujar_tarjeta(int start_y, int start_x, int alto, int ancho) {
    attron(COLOR_PAIR(1));
    for(int i = 0; i < alto; i++) {
        mvhline(start_y + i, start_x, ' ', ancho);
    }
    mvhline(start_y, start_x + 1, ACS_HLINE, ancho - 2);         
    mvhline(start_y + alto - 1, start_x + 1, ACS_HLINE, ancho - 2); 
    mvvline(start_y + 1, start_x, ACS_VLINE, alto - 2);         
    mvvline(start_y + 1, start_x + ancho - 1, ACS_VLINE, alto - 2); 
    mvaddch(start_y, start_x, ACS_ULCORNER);
    mvaddch(start_y, start_x + ancho - 1, ACS_URCORNER);
    mvaddch(start_y + alto - 1, start_x, ACS_LLCORNER);
    mvaddch(start_y + alto - 1, start_x + ancho - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(1));
}

void mostrar_error_servidor() {
    clear();
    int sy = (LINES - 10) / 2, sx = (COLS - 50) / 2;
    dibujar_tarjeta(sy, sx, 10, 50);
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(sy + 3, sx + 5, "Error: Se ha perdido la conexion con el servidor"); // Actualizado según rúbrica
    attron(COLOR_PAIR(4));
    mvprintw(sy + 6, sx + 19, "   Salir   ");
    attroff(COLOR_PAIR(4) | COLOR_PAIR(5) | A_BOLD);
    refresh();
    while (getch() != '\n') {}
    endwin(); exit(1); 
}

void mostrar_notificacion(const char *mensaje, int es_exito) {
    clear();
    int alto = 10, ancho = 55;
    int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
    dibujar_tarjeta(sy, sx, alto, ancho);

    if (es_exito) attron(COLOR_PAIR(2) | A_BOLD); 
    else attron(COLOR_PAIR(5) | A_BOLD);

    int len_msg = strlen(mensaje);
    mvprintw(sy + 3, sx + (ancho - len_msg) / 2, "%s", mensaje);
    
    if (es_exito) attroff(COLOR_PAIR(2) | A_BOLD);
    else attroff(COLOR_PAIR(5) | A_BOLD);

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(sy + 6, sx + 20, " [ Continuar ] ");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    refresh();
    while (getch() != '\n' && getch() != '\r') {} 
}
