#include <ncurses.h>
#include <stdlib.h>

void configurar_colores() {
    start_color();

  
    if (can_change_color()) {

        init_color(10, 850, 850, 850); 
        init_color(11, 250, 250, 250);
        init_color(12, 100, 600, 900);

       
        init_pair(1, 11, 10);          
        init_pair(2, 10, 12);         
        init_pair(3, 12, 10);         
    } else {
      
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_CYAN);
        init_pair(3, COLOR_CYAN, COLOR_WHITE);
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

int main() {
   
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        configurar_colores();
    }

  
    bkgd(COLOR_PAIR(1));


    int opcion = 0;
    int ejecutando = 1;
    const char *opciones[3] = {
        "   Iniciar Sesion   ", 
        "    Registrarse     ", 
        "       Salir        "
    };

 
    while (ejecutando) {
        clear();

        int alto_tarjeta = 14;
        int ancho_tarjeta = 46;
        int start_y = (LINES - alto_tarjeta) / 2;
        int start_x = (COLS - ancho_tarjeta) / 2;

        dibujar_tarjeta(start_y, start_x, alto_tarjeta, ancho_tarjeta);

        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(start_y + 2, start_x + 15, "=== VITAL-TL ===");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(start_y + 3, start_x + 9, "Sistema Clinico Centralizado");
        
        mvhline(start_y + 5, start_x + 2, ACS_HLINE, ancho_tarjeta - 4);
        attroff(COLOR_PAIR(1));

        for (int i = 0; i < 3; i++) {
            if (i == opcion) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(start_y + 7 + (i * 2), start_x + 13, "%s", opciones[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(start_y + 7 + (i * 2), start_x + 13, "%s", opciones[i]);
                attroff(COLOR_PAIR(1));
            }
        }

        refresh();
        int tecla = getch();

        switch (tecla) {
            case KEY_UP:
                opcion = (opcion - 1 + 3) % 3; 
                break;
                
            case KEY_DOWN:
                opcion = (opcion + 1) % 3;
                break;
                
            case '\n': 
            case '\r':
                if (opcion == 0) {
                    clear();
                    dibujar_tarjeta(start_y, start_x, alto_tarjeta, ancho_tarjeta);
                    attron(COLOR_PAIR(3) | A_BOLD);
                    mvprintw(start_y + (alto_tarjeta/2), start_x + 13, "Pantalla de Login...");
                    attroff(COLOR_PAIR(3) | A_BOLD);
                    refresh();
                    napms(1000); 
                } 
                else if (opcion == 1) {
                    clear();
                    dibujar_tarjeta(start_y, start_x, alto_tarjeta, ancho_tarjeta);
                    attron(COLOR_PAIR(3) | A_BOLD);
                    mvprintw(start_y + (alto_tarjeta/2), start_x + 11, "Pantalla de Registro...");
                    attroff(COLOR_PAIR(3) | A_BOLD);
                    refresh();
                    napms(1000);
                } 
                else if (opcion == 2) {
                    ejecutando = 0;
                }
                break;
        }
    }

    endwin();
    return 0;
}
