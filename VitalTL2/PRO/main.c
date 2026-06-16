#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "chamcont.h" 
#include "datos.h" 
#include "semaforo.h" 
#include "pantalla.h"
#include "panel.h"
#include "admin.h"
#include "memoria.h"

#define FIFO_CONEXION "/tmp/vital_conexion_fifo"

int conectar_con_servidor(ConexionCliente **datos_cliente) {
    pid_t pid_actual = getpid();
    printf("[CLIENTE] Intentando conectar al servidor (PID=%d)...\n", pid_actual);
    
    int fd = open(FIFO_CONEXION, O_WRONLY);
    if (fd < 0) {
        printf("[ERROR] No se puede conectar al servidor.\n");
        printf("Asegúrate de que el servidor está corriendo con: ./servidor\n");
        return -1;
    }
    
    if (write(fd, &pid_actual, sizeof(pid_t)) != sizeof(pid_t)) {
        printf("[ERROR] No se pudo enviar PID al servidor\n");
        close(fd);
        return -1;
    }
    close(fd);
    printf("[CLIENTE] PID enviado al servidor...\n");
    
    int intentos = 0;
    const int MAX_INTENTOS = 50; 
    int shmid = -1;
    
    while (intentos < MAX_INTENTOS) {
        shmid = obtener_memoria_privada(pid_actual, sizeof(ConexionCliente));
        if (shmid >= 0) break;
        usleep(50000); 
        intentos++;
    }
    
    if (shmid < 0) {
        printf("[ERROR] Timeout: No se recibió memoria privada del servidor\n");
        return -1;
    }
    
    *datos_cliente = (ConexionCliente*) shmat(shmid, NULL, 0);
    if (*datos_cliente == (void*)-1) {
        printf("[ERROR] No se pudo adjuntar memoria privada\n");
        return -1;
    }
    
    printf("[CLIENTE] Conectado exitosamente al servidor\n\n");
    return shmid;
}

void verificar_servidor_disponible(ConexionCliente *datos) {
    int heartbeat_anterior = -1;
    int intentos_sin_cambio = 0;
    const int MAX_INTENTOS_SIN_CAMBIO = 20; 
    
    while (1) {
        bloquear(&datos->candado);
        int heartbeat_actual = datos->heartbeat_servidor;
        desbloquear(&datos->candado);
        
        if (heartbeat_actual != heartbeat_anterior) {
            intentos_sin_cambio = 0;
            heartbeat_anterior = heartbeat_actual;
            return;
        } else {
            intentos_sin_cambio++;
        }
        
        if (intentos_sin_cambio >= MAX_INTENTOS_SIN_CAMBIO) {
            clear();
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(LINES/2 - 5, COLS/2 - 30, "========== ERROR CRITICO ==========");
            mvprintw(LINES/2 - 3, COLS/2 - 20, "El servidor VITAL-TL no esta disponible");
            mvprintw(LINES/2 - 1, COLS/2 - 25, "Por favor, inicie el servidor en otra terminal");
            mvprintw(LINES/2 + 1, COLS/2 - 25, "con: ./servidor");
            attroff(COLOR_PAIR(3) | A_BOLD);
            
            attron(COLOR_PAIR(2));
            mvprintw(LINES/2 + 4, COLS/2 - 35, "Reintentando conexion en 2 segundos...");
            attroff(COLOR_PAIR(2));
            
            refresh();
            napms(2000);
            intentos_sin_cambio = 0; 
        }
        napms(50);
    }
}

int main() {
    ConexionCliente *datos = NULL;
    int shmid = conectar_con_servidor(&datos);
    
    if (shmid < 0) {
        printf("\n[ERROR CRITICO] No se puede conectar al servidor.\n");
        printf("Por favor, inicie el servidor con: ./servidor\n\n");
        exit(1);
    }
    
    bloquear(&datos->candado); 
    datos->pid_cliente = getpid(); 
    strcpy(datos->log_mensaje, "Cliente abrió la pantalla de Acceso Principal");
    datos->peticion_activa = 1; 
    desbloquear(&datos->candado);

    initscr(); start_color(); cbreak(); noecho();
    keypad(stdscr, TRUE); curs_set(0);

    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(3, COLOR_RED, COLOR_BLACK);     
    init_pair(4, COLOR_WHITE, COLOR_BLACK);   
    init_pair(5, COLOR_BLACK, COLOR_CYAN);    
    init_pair(6, COLOR_BLACK, COLOR_YELLOW); 
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK); 

    int opcion = 0;
    int ejecutando = 1;
    Paciente usuario_sesion; 
    
    int heartbeat_anterior_loop = -1;
    int contador_sin_cambio = 0;

    int alto = 18;
    int ancho = 64;

    while (ejecutando) {
        int heartbeat_check;
        bloquear(&datos->candado);
        heartbeat_check = datos->heartbeat_servidor;
        desbloquear(&datos->candado);
        
        if (heartbeat_check == heartbeat_anterior_loop) {
            contador_sin_cambio++;
            if (contador_sin_cambio > 40) { 
                verificar_servidor_disponible(datos);
                contador_sin_cambio = 0;
            }
        } else {
            heartbeat_anterior_loop = heartbeat_check;
            contador_sin_cambio = 0;
        }
        
        bkgd(COLOR_PAIR(4));
        clear();

        int sy = (LINES - alto) / 2;
        int sx = (COLS - ancho) / 2;

        attron(COLOR_PAIR(1));
        for (int i = 0; i < alto; i++) {
            mvaddch(sy + i, sx, ACS_VLINE);
            mvaddch(sy + i, sx + ancho - 1, ACS_VLINE);
        }
        mvhline(sy, sx, ACS_HLINE, ancho);
        mvhline(sy + alto - 1, sx, ACS_HLINE, ancho);
        mvaddch(sy, sx, ACS_ULCORNER);
        mvaddch(sy, sx + ancho - 1, ACS_URCORNER);
        mvaddch(sy + alto - 1, sx, ACS_LLCORNER);
        mvaddch(sy + alto - 1, sx + ancho - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(1));

        attron(COLOR_PAIR(7) | A_BOLD);
        mvprintw(sy + 2, sx + (ancho - 36)/2, " __      _______ _______       _      ");
        mvprintw(sy + 3, sx + (ancho - 36)/2, " \\ \\    / /_   _|__   __|     | |     ");
        mvprintw(sy + 4, sx + (ancho - 36)/2, "  \\ \\  / /  | |    | |  __ _  | |     ");
        mvprintw(sy + 5, sx + (ancho - 36)/2, "   \\ \\/ /   | |    | | / _` | | |     ");
        mvprintw(sy + 6, sx + (ancho - 36)/2, "    \\  /   _| |_   | || (_| | | |____ ");
        mvprintw(sy + 7, sx + (ancho - 36)/2, "     \\/   |_____|  |_| \\__,_| |______|");
        attroff(COLOR_PAIR(7) | A_BOLD);

        int menu_y = sy + 10;
        int menu_x = sx + (ancho - 30) / 2;

        if (opcion == 0) {
            attron(COLOR_PAIR(5) | A_BOLD);
            mvprintw(menu_y, menu_x, "      [ INICIAR SESION ]      ");
            attroff(COLOR_PAIR(5) | A_BOLD);
        } else {
            attron(COLOR_PAIR(1));
            mvprintw(menu_y, menu_x, "        INICIAR SESION        ");
            attroff(COLOR_PAIR(1));
        }

        if (opcion == 1) {
            attron(COLOR_PAIR(6) | A_BOLD);
            mvprintw(menu_y + 2, menu_x, "        [ REGISTRARSE ]       ");
            attroff(COLOR_PAIR(6) | A_BOLD);
        } else {
            attron(COLOR_PAIR(2));
            mvprintw(menu_y + 2, menu_x, "          REGISTRARSE         ");
            attroff(COLOR_PAIR(2));
        }

        if (opcion == 2) {
            attron(COLOR_PAIR(3) | A_REVERSE | A_BOLD);
            mvprintw(menu_y + 4, menu_x, "           [ SALIR ]          ");
            attroff(COLOR_PAIR(3) | A_REVERSE | A_BOLD);
        } else {
            attron(COLOR_PAIR(3));
            mvprintw(menu_y + 4, menu_x, "             SALIR            ");
            attroff(COLOR_PAIR(3));
        }

        refresh();
        int ch = getch();

        if (ch == KEY_UP && opcion > 0) opcion--;
        else if (ch == KEY_DOWN && opcion < 2) opcion++;
        else if (ch == 'q' || ch == 'Q') {
            ejecutando = 0;
        } else if (ch == '\n' || ch == '\r') {
            if (opcion == 0) {
                clear(); refresh(); 
                int tipo_usuario = pantalla_login(&usuario_sesion, datos);
                if (tipo_usuario == 1) {
                    iniciar_panel_paciente(&usuario_sesion, datos); 
                } 
                else if (tipo_usuario == 2) {
                    clear(); refresh();
                    iniciar_panel_admin(datos); 
                }
            }
            else if (opcion == 1) {
                clear(); refresh();
                pantalla_registro(datos);
            } else if (opcion == 2) {
                bloquear(&datos->candado);
                datos->pid_cliente = getpid();
                strcpy(datos->log_mensaje, "Cliente solicito desconectarse del servidor");
                datos->tipo_peticion = 2;
                datos->peticion_activa = 1;
                desbloquear(&datos->candado);

                int esperando = 1;
                while (esperando) {
                    bloquear(&datos->candado);
                    if (datos->peticion_activa == 0) esperando = 0;
                    desbloquear(&datos->candado);
                    if (esperando) napms(50);
                }
                ejecutando = 0;
            }
        }
    }

    endwin();
    shmdt(datos);
    printf("[CLIENTE] Desconectado del servidor.\n");
    return 0;
}