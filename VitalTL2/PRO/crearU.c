#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "chamcont.h"
#include "datos.h"
#include "semaforo.h"

void pantalla_registro(Analisis *datos) {
    clear();
    refresh();

    init_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_pair(6, COLOR_BLUE, COLOR_WHITE);

    int alto = 20;
    int ancho = 70;
    int start_y = (LINES - alto) / 2;
    int start_x = (COLS - ancho) / 2;

    WINDOW *win = newwin(alto, ancho, start_y, start_x);
    wbkgd(win, COLOR_PAIR(5));
    keypad(win, TRUE);

    char buffers[7][50] = {0};
    int lengths[7] = {0};
    int max_len[7] = {45, 45, 4, 4, 45, 45, 45};
    
    const char *etiquetas[7] = {
        "Nombre:", "Apellido:", "Edad:", "Tipo de sangre:",
        "Correo:", "Usuario:", "Contrasena:"
    };
    int form_y = 4;
    int foco_actual = 0;
    bool ejecutando = true;
    int heartbeat_anterior = -1;
    int contador_sin_cambio = 0;

    while(ejecutando) {
        
        bloquear(&datos->candado);
        int heartbeat_actual = datos->heartbeat_servidor;
        desbloquear(&datos->candado);
        
        if (heartbeat_actual == heartbeat_anterior) {
            contador_sin_cambio++;
            if (contador_sin_cambio > 40) { 
                
                delwin(win);
                clear();
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(LINES/2 - 5, COLS/2 - 30, "========== ERROR CRITICO ==========");
                mvprintw(LINES/2 - 3, COLS/2 - 20, "El servidor VITAL-TL no esta disponible");
                mvprintw(LINES/2 - 1, COLS/2 - 25, "Por favor, reinicie el servidor");
                mvprintw(LINES/2 + 1, COLS/2 - 25, "Reintentando en 2 segundos...");
                attroff(COLOR_PAIR(3) | A_BOLD);
                refresh();
                napms(2000);
                contador_sin_cambio = 0;
                clear();
                
                
                win = newwin(alto, ancho, start_y, start_x);
                wbkgd(win, COLOR_PAIR(5));
                keypad(win, TRUE);
                continue;
            }
        } else {
            heartbeat_anterior = heartbeat_actual;
            contador_sin_cambio = 0;
        }
        
        werase(win);
        wattron(win, A_BOLD);
        mvwprintw(win, 2, 21, "--- REGISTRO DE USUARIO ---");
        wattroff(win, A_BOLD);
	        
        for(int i = 0; i < 7; i++) {
            mvwprintw(win, form_y + i, 5, "%s", etiquetas[i]);
            wmove(win, form_y + i, 22);
            
            if (i == 6) {
                for(int p = 0; p < lengths[i]; p++) wprintw(win, "*");
            } else {
                wprintw(win, "%s", buffers[i]);
            }
        }

        if (foco_actual == 7) wattron(win, COLOR_PAIR(6));
        else wattron(win, COLOR_PAIR(5));
        mvwprintw(win, 14, 28, "<< GUARDAR");
        
        if (foco_actual == 8) wattron(win, COLOR_PAIR(6));
        else wattron(win, COLOR_PAIR(5));
        mvwprintw(win, 16, 25, "<< REGRESAR");
        wattron(win, COLOR_PAIR(5));

        box(win, 0, 0);

        if (foco_actual < 7) {
            curs_set(1);
            wmove(win, form_y + foco_actual, 22 + lengths[foco_actual]);
        } else {
            curs_set(0);
        }

        wrefresh(win);
        int ch = wgetch(win);

        if (ch == KEY_UP) {
            if (foco_actual > 0) foco_actual--;
            else foco_actual = 8;
        } 
        else if (ch == KEY_DOWN) {
            if (foco_actual < 8) foco_actual++;
            else foco_actual = 0;
        } 
        else if (ch == '\n' || ch == '\r') {
            if (foco_actual < 7) {
                foco_actual++;
            } 
		else if (foco_actual == 7) {

		if (lengths[0] == 0 || lengths[1] == 0 || lengths[5] == 0 || lengths[6] == 0) {
                    wattron(win, A_BOLD);
                    mvwprintw(win, 18, 5, "Error: No puedes dejar campos vacios *le da un sape*");
                    wattroff(win, A_BOLD);
                    wrefresh(win); napms(2000);
                }
 
                else if (!es_edad_valida(buffers[2])) { 
                    wattron(win, A_BOLD);
                    mvwprintw(win, 18, 5, "Error: La edad deben ser NUMEROS *le da un sape* ");
                    wattroff(win, A_BOLD);
                    wrefresh(win); napms(2000);
                    foco_actual = 2;
                }
                else if (!es_sangre_valida(buffers[3])) { 
                    wattron(win, A_BOLD);
                    mvwprintw(win, 18, 5, "Error: Tipo de sangre no valido (ej: O+) *le da un sape*");
                    wattroff(win, A_BOLD);
                    wrefresh(win); napms(2000);
                    foco_actual = 3;
                }
                else if (!correoV(buffers[4])) {
                    wattron(win, A_BOLD);
                    mvwprintw(win, 18, 5, "Error: El formato de tu correo esta rarito *le da un sape*");
                    wattroff(win, A_BOLD);
                    wrefresh(win);
                    napms(2000); 
                    foco_actual = 4; 
                } 
                else {
                    encriptar(buffers[6]);
                    FILE *fp = fopen("usuarios.txt", "a");
                    if (fp != NULL) {
                        fprintf(fp, "Nombre: %s\nApellido: %s\nEdad: %s\nSangre: %s\nCorreo: %s\nUsuario: %s\nPassword: %s\n------------------------\n",
                        buffers[0], buffers[1], buffers[2], buffers[3], buffers[4], buffers[5], buffers[6]);
                        fclose(fp);
                    }
                    werase(win);
                    box(win, 0, 0);
                    wattron(win, A_BOLD | COLOR_PAIR(6));
                    mvwprintw(win, alto / 2 - 1, (ancho - 34) / 2, "==================================");
                    mvwprintw(win, alto / 2,     (ancho - 34) / 2, "  ¡USUARIO REGISTRADO CON EXITO!  ");
                    mvwprintw(win, alto / 2 + 1, (ancho - 34) / 2, "==================================");
                    wattroff(win, A_BOLD | COLOR_PAIR(6));
                    wrefresh(win);
                    napms(2500);
                    
                    ejecutando = false;
                }
			}
            else if (foco_actual == 8) {
                ejecutando = false;
            }
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (foco_actual < 7 && lengths[foco_actual] > 0) {
                lengths[foco_actual]--;
                buffers[foco_actual][lengths[foco_actual]] = '\0';
            }
        } 
        else if (ch >= 32 && ch <= 126) {
            if (foco_actual < 7 && lengths[foco_actual] < max_len[foco_actual]) {
                buffers[foco_actual][lengths[foco_actual]] = ch;
                lengths[foco_actual]++;
                buffers[foco_actual][lengths[foco_actual]] = '\0';
            }
        }
    }

    curs_set(0);
    delwin(win);
    clear();
    refresh();
}
