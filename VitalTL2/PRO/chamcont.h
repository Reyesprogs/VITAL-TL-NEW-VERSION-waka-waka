#ifndef CHAMCONT_H
#define CHAMCONT_H

#include <stdbool.h> 
#include "datos.h"   

bool correoV(char *correo);
bool es_edad_valida(char *edad);
bool es_sangre_valida(char *sangre);
void encriptar(char *pass);
bool validar_credenciales(char *user, char *pass);

int pantalla_login(Paciente *p, Analisis *datos);
void pantalla_registro(Analisis *datos);

#endif
