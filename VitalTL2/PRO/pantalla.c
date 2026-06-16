#include <ncurses.h>
#include "pantalla.h"
#include "semaforo.h"

void iniciar_pantalla() {
    initscr(); start_color(); cbreak(); noecho(); curs_set(0);
    nodelay(stdscr, TRUE);
}

void dibujar_bloque_estudio(int *linea_y, int x, int *valores, int cantidad, const char* nombre_estudio, int maximo) {
    mvprintw(*linea_y, x, "%-18s", nombre_estudio);
    
    if (cantidad == 0) {
        attron(COLOR_PAIR(4));
        printw(" [ ------------- ESTUDIO NO ADQUIRIDO ------------- ]");
        attroff(COLOR_PAIR(4));
        (*linea_y) += 2; 
        return;
    }

    (*linea_y)++;
    for (int i = 0; i < cantidad; i++) {
        int val = valores[i];
        mvprintw(*linea_y, x + 4, "Muestra %d: ", i + 1);

        if (val == -2) {
            attron(COLOR_PAIR(2) | A_BOLD);
            printw("[ ADQUIRIDO - PENDIENTE DE PARAMETROS ]");
            attroff(COLOR_PAIR(2) | A_BOLD);
        } else {
            printw("[");

            int color_barra = 1; 
            if (i == 1) color_barra = 4; 
            if (i == 2) color_barra = 2; 

            attron(COLOR_PAIR(color_barra) | A_BOLD);
            int tope = (val * 40) / maximo;
            for(int b = 0; b < 40; b++) {
                if (b < tope) addch(ACS_CKBOARD);
                else addch(' ');
            }
            attroff(COLOR_PAIR(color_barra) | A_BOLD);
            printw("] %3d unidades", val);
        }
        (*linea_y)++;
    }
    (*linea_y)++; 
}

void actualizar(Analisis *datos) {
    erase();
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(1, 5, "--- VITAL-TL : EVOLUCION HISTORICA DE TENDENCIAS ---");
    attroff(COLOR_PAIR(4) | A_BOLD);

    bloquear(&datos->candado);
    
    int dinamico_y = 4;

    dibujar_bloque_estudio(&dinamico_y, 5, datos->hemoglobina, datos->hemoglobina_cant, "Biometria Hem.", 20);
    dibujar_bloque_estudio(&dinamico_y, 5, datos->glucosa, datos->glucosa_cant, "Glucosa", 150);
    dibujar_bloque_estudio(&dinamico_y, 5, datos->orina, datos->orina_cant, "pH Orina", 10);
    dibujar_bloque_estudio(&dinamico_y, 5, datos->colesterol, datos->colesterol_cant, "Colesterol", 260);
    dibujar_bloque_estudio(&dinamico_y, 5, datos->trigliceridos, datos->trigliceridos_cant, "Trigliceridos", 240);
    
    desbloquear(&datos->candado);
    
    mvprintw(dinamico_y, 5, "Presiona 'q' para regresar al menu anterior.");
    refresh();
}

void cerrar_pantalla() {
    endwin();
}
