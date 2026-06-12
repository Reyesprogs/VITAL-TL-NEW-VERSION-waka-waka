#ifndef ADMIN_H
#define ADMIN_H

#include "inter.h"

void gestionar_usuarios(int semid, MemoriaCompartida *memoria);
void gestionar_analisis(int semid, MemoriaCompartida *memoria);
void capturar_resultados_analisis(char *usuario, char *categoria, char *nombre, char *componentes, int semid, MemoriaCompartida *memoria);
void menu_registrar_resultados(int semid, MemoriaCompartida *memoria);
void abrir_panel_admin(int semid, MemoriaCompartida *memoria);
void mostrar_panel_ventas(int semid, MemoriaCompartida *memoria);

#endif
