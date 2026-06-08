#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

void configurar_colores();
void dibujar_tarjeta(int start_y, int start_x, int alto, int ancho);
void mostrar_error_servidor();
void mostrar_notificacion(const char *mensaje, int es_exito);

#endif
