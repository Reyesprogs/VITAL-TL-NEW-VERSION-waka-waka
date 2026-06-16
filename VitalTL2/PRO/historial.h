#ifndef HISTORIAL_H
#define HISTORIAL_H

#include "datos.h"

void guardar_compra(const char* usuario, const char* estudio);
void cargar_historial(const char* usuario, Analisis *datos);

#endif
