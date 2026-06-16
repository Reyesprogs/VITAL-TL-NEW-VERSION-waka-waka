#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "compras.h"
#include "historial.h"
#include "semaforo.h" 

void mostrar_tienda(Paciente *p, Analisis *datos) {
    int opcion = 0;
    int ejecutando = 1;
    
    char catalogo[20][100]; 
    int total_estudios = 0;

    
    char carrito[20][100];
    int items_carrito = 0;

    
    bloquear(&datos->candado); 
    FILE *fc = fopen("catalogo.txt", "r");
    if (fc != NULL) {
        char linea[100];
        while (fgets(linea, sizeof(linea), fc) && total_estudios < 20) {
            linea[strcspn(linea, "\n")] = 0; 
            strcpy(catalogo[total_estudios], linea);
            total_estudios++;
        }
        fclose(fc);
    }
    desbloquear(&datos->candado);

    if (total_estudios == 0) {
        clear();
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(10, 10, "El administrador aun no ha agregado estudios al catalogo.");
        attroff(COLOR_PAIR(3) | A_BOLD);
        mvprintw(12, 10, "Presiona cualquier tecla para regresar...");
        getch();
        return;
    }

    while (ejecutando) {
        clear();
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(2, 5, "--- ADQUISICION DE ANALISIS CLINICOS ---");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        
        mvprintw(4, 5, "Carrito actual: [ %d ] articulos listos para pagar.", items_carrito);

        for (int i = 0; i < total_estudios; i++) {
            if (i == opcion) attron(A_REVERSE);
            mvprintw(6 + (i * 2), 8, "[ %s ]", catalogo[i]);
            if (i == opcion) attroff(A_REVERSE);
        }
        
        
        mvprintw(LINES - 4, 5, "Flechas: Navegar  |  ENTER: Agregar al carrito");
        mvprintw(LINES - 3, 5, "    'P': Pagar    |    'Q': Volver al panel");
        refresh();

        int tecla = getch();
        if (tecla == KEY_UP && opcion > 0) opcion--;
        else if (tecla == KEY_DOWN && opcion < total_estudios - 1) opcion++;
        else if (tecla == 'q' || tecla == 'Q') ejecutando = 0;
        
        
        else if (tecla == '\n' || tecla == '\r') {
            if (items_carrito < 20) {
                strcpy(carrito[items_carrito], catalogo[opcion]);
                items_carrito++;
                attron(COLOR_PAIR(1));
                mvprintw(LINES - 6, 5, ">>> %s agregado a la canasta <<<", catalogo[opcion]);
                attroff(COLOR_PAIR(1));
                refresh();
                napms(800);
            } else {
                mvprintw(LINES - 6, 5, "¡El carrito esta lleno!");
                refresh();
                napms(800);
            }
        }
        
        
        else if (tecla == 'p' || tecla == 'P') {
            if (items_carrito == 0) {
                attron(COLOR_PAIR(3));
                mvprintw(LINES - 6, 5, "¡Tu carrito esta vacio! Agrega algo primero.");
                attroff(COLOR_PAIR(3));
                refresh();
                napms(1000);
            } else {
                
                int win_h = 6 + items_carrito;
                if (win_h > LINES - 4) win_h = LINES - 4;
                int win_w = COLS - 20;
                if (win_w < 40) win_w = 40;
                int start_y = (LINES - win_h) / 2;
                int start_x = (COLS - win_w) / 2;

                WINDOW *w = newwin(win_h, win_w, start_y, start_x);
                box(w, 0, 0);
                keypad(w, TRUE);

                wattron(w, A_BOLD | COLOR_PAIR(1));
                mvwprintw(w, 1, 2, "--- CONFIRMAR PAGO ---");
                wattroff(w, A_BOLD | COLOR_PAIR(1));

                
                int list_start = 2;
                for (int j = 0; j < items_carrito && j < win_h - 6; j++) {
                    mvwprintw(w, list_start + j, 4, "- %s", carrito[j]);
                }

                
                const char *opts[2] = {"CONFIRMAR", "CANCELAR"};
                int opt_sel = 0;

                
                while (1) {
                    
                    mvwprintw(w, win_h - 3, 4, "%*s", win_w - 8, " ");
                    int opt_x = 4;
                    for (int k = 0; k < 2; k++) {
                        if (k == opt_sel) wattron(w, A_REVERSE | A_BOLD | COLOR_PAIR(2));
                        else wattron(w, COLOR_PAIR(3));
                        mvwprintw(w, win_h - 3, opt_x, "[ %s ]", opts[k]);
                        if (k == opt_sel) wattroff(w, A_REVERSE | A_BOLD | COLOR_PAIR(2));
                        else wattroff(w, COLOR_PAIR(3));
                        opt_x += strlen(opts[k]) + 6;
                    }
                    mvwprintw(w, win_h - 2, 4, "ENTER: seleccionar | Flechas: mover | ESC: cancelar");
                    wrefresh(w);

                    int c = wgetch(w);
                    if (c == KEY_LEFT && opt_sel > 0) opt_sel--;
                    else if (c == KEY_RIGHT && opt_sel < 1) opt_sel++;
                    else if (c == '\n' || c == '\r') {
                        if (opt_sel == 0) {
                            
                            
                            for(int j = 0; j < items_carrito; j++) {
                                guardar_compra(p->usuario, carrito[j]); 
                            }
                            
                            cargar_historial(p->usuario, datos);

                            
                            delwin(w);
                            clear();
                            attron(COLOR_PAIR(1) | A_BOLD);
                            mvprintw(5, 10, "--- TICKET DE COMPRA ---");
                            attroff(COLOR_PAIR(1) | A_BOLD);
                            for(int j = 0; j < items_carrito; j++) {
                                mvprintw(7 + j, 10, "- %s", carrito[j]);
                            }
                            attron(COLOR_PAIR(1) | A_BOLD);
                            mvprintw(9 + items_carrito, 10, "¡Pago exitoso! El administrador evaluara tus resultados.");
                            attroff(COLOR_PAIR(1) | A_BOLD);
                            mvprintw(11 + items_carrito, 10, "Presiona una tecla para continuar...");
                            refresh();
                            getch();

                            items_carrito = 0; 
                            break;
                        } else {
                            
                            delwin(w);
                            break;
                        }
                    } else if (c == 27) { 
                        delwin(w);
                        break;
                    }
                }
            }
        }
    }
}  