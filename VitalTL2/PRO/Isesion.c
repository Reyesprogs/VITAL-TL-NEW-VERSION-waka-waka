
#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "chamcont.h"
#include "datos.h"
#include "semaforo.h"

int pantalla_login(Paciente *p, Analisis *datos) { 
    int alto = 14;
    int ancho = 50;
    int start_y = (LINES - alto) / 2;
    int start_x = (COLS - ancho) / 2;

    WINDOW *win = newwin(alto, ancho, start_y, start_x);
    wbkgd(win, COLOR_PAIR(1));
    keypad(win, TRUE);

    char buffers[2][50] = {0}; 
    int lengths[2] = {0};
    int foco_actual = 0; 
    bool ejecutando = true;
    int login_exitoso = 0;
    int heartbeat_anterior = -1;
    int contador_sin_cambio = 0;

    curs_set(1); 

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
                wbkgd(win, COLOR_PAIR(1));
                keypad(win, TRUE);
                continue;
            }
        } else {
            heartbeat_anterior = heartbeat_actual;
            contador_sin_cambio = 0;
        }
        
        werase(win);
        box(win, 0, 0);

        wattron(win, A_BOLD);
        mvwprintw(win, 2, 13, "--- INICIO DE SESION ---");
        wattroff(win, A_BOLD);
        
        mvwprintw(win, 5, 5, "Usuario: [%s]", buffers[0]);
        mvwprintw(win, 7, 5, "Contrasena: [");
        for(int i = 0; i < lengths[1]; i++) waddch(win, '*');
        wprintw(win, "]");

        if (foco_actual == 2) wattron(win, A_REVERSE);
        mvwprintw(win, 10, 10, "[ ENTRAR ]");
        if (foco_actual == 2) wattroff(win, A_REVERSE);

        if (foco_actual == 3) wattron(win, A_REVERSE);
        mvwprintw(win, 10, 28, "[ SALIR ]");
        if (foco_actual == 3) wattroff(win, A_REVERSE);

        if (foco_actual == 0) wmove(win, 5, 15 + lengths[0]);
        else if (foco_actual == 1) wmove(win, 7, 18 + lengths[1]);

        wrefresh(win);
        int ch = wgetch(win);

        if (ch == KEY_UP) {
            if (foco_actual > 0) foco_actual--;
        } 
        else if (ch == KEY_DOWN) {
            if (foco_actual < 3) foco_actual++;
        } 
        else if (ch == '\n' || ch == '\r') {
            if (foco_actual < 2) {
                foco_actual++;
            } else if (foco_actual == 2) {
                
      
                mvwprintw(win, 12, 5, "Conectando al servidor...          ");
                wrefresh(win);

             
                bloquear(&datos->candado);
                strcpy(datos->req_usuario, buffers[0]);
                strcpy(datos->req_pass, buffers[1]);
                datos->tipo_peticion = 1; 
                datos->peticion_activa = 1;
                desbloquear(&datos->candado);

                
                int esperando = 1;
                while (esperando) {
                    bloquear(&datos->candado);
                    if (datos->peticion_activa == 0) esperando = 0; 
                    desbloquear(&datos->candado);
                    if (esperando) napms(50); 
                }

                 bloquear(&datos->candado);
                  if (datos->respuesta_servidor == 1) {
                                  
                     strcpy(p->nombre, datos->req_nombre);
                     strcpy(p->apellido, datos->req_apellido);
                     strcpy(p->edad, datos->req_edad);
                     strcpy(p->sangre, datos->req_sangre);
                     strcpy(p->correo, datos->req_correo);
                     strcpy(p->usuario, buffers[0]);
                
                     mvwprintw(win, 12, 5, "Ingreso exitoso.                   ");
                     wrefresh(win);
                     napms(1000);
                     login_exitoso = 1; 
                     ejecutando = false; 
                                } 
                                else if (datos->respuesta_servidor == 2) {
                                   
                                    mvwprintw(win, 12, 5, "Modo Administrador Activo.          ");
                                    wrefresh(win);
                                    napms(1000);
                                    login_exitoso = 2; 
                                    ejecutando = false;
                                } 
                                else {
                                   
                                    wattron(win, COLOR_PAIR(3));
                                    mvwprintw(win, 12, 5, "Datos incorrectos *le da un sape* ");
                                    wattroff(win, COLOR_PAIR(3));
                                    wrefresh(win);
                                    napms(1500);
                                    foco_actual = 0; 
                                }
                                desbloquear(&datos->candado);
          

            } else if (foco_actual == 3) {
                ejecutando = false;
            }
        } 
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (foco_actual < 2 && lengths[foco_actual] > 0) {
                lengths[foco_actual]--;
                buffers[foco_actual][lengths[foco_actual]] = '\0';
            }
        } 
        else if (ch >= 32 && ch <= 126) {
            if (foco_actual < 2 && lengths[foco_actual] < 45) {
                buffers[foco_actual][lengths[foco_actual]] = ch;
                lengths[foco_actual]++;
            }
        }
    }

    curs_set(0); 
    delwin(win);
    return login_exitoso;
}
