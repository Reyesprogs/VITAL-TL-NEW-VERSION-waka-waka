#ifndef PANTALLA_H
#define PANTALLA_H

#include "datos.h"

void iniciar_pantalla();
void dibujar_barra(int y, int x, int valor, int maximo, const char* texto, int normal, int riesgo);
void actualizar(Analisis *datos);
void cerrar_pantalla();

#endif
