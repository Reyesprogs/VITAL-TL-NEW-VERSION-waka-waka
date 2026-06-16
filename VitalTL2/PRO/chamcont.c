#include "chamcont.h"
#include <string.h> 
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h> 


void encriptar(char *password) {
    int llave = 50; 
    int i; 
    int longitud = (int)strlen(password); 

    for (i = 0; i < longitud; i++) {
        password[i] = password[i] ^ llave; 
    }
}

bool correoV(char *correo) {
    if (correo == NULL || strlen(correo) == 0) return false;

    char *arroba = strchr(correo, '@');
    if (arroba == NULL || arroba == correo || *(arroba + 1) == '\0') {
        return false;
    }

    char *punto = strchr(arroba, '.');
    if (punto == NULL || punto == (arroba + 1) || *(punto + 1) == '\0') {
        return false;
    }
    return true; 
}

bool es_edad_valida(char *edad) {
    if (edad == NULL || strlen(edad) == 0) {
        return false;
    }
    int largo = (int)strlen(edad); 
    for (int i = 0; i < largo; i++) {
        if (!isdigit((unsigned char)edad[i])) {
            return false; 
        }
    }
    return true;
}

bool es_sangre_valida(char *sangre) {
    int len = strlen(sangre);
    if (len < 2 || len > 3) return false;

    for (int i = 0; i < len - 1; i++) {
        if (!isupper(sangre[i])) return false;
    }
    char ultimo = sangre[len - 1];
    if (ultimo != '+' && ultimo != '-') return false;

    return true;
}
