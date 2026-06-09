#ifndef ADMIN_H
#define ADMIN_H

#include "inter.h"

void abrir_panel_admin(int semid, MemoriaCompartida *memoria);
void gestionar_usuarios(int semid, MemoriaCompartida *memoria);
void gestionar_analisis(int semid, MemoriaCompartida *memoria); // <-- NUEVA

#endif
