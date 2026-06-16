#include "chamcont.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>  
#include "chamcont.h" 

bool validar_credenciales(char *user, char *pass) {
    FILE *fp = fopen("usuarios.txt", "r");
    if (fp == NULL) return false;

    char linea[256], userA[50], passA[50];
    bool encontrado = false;
    
    char passE[50];
    strcpy(passE, pass);
    encriptar(passE); 

    while (fgets(linea, sizeof(linea), fp)) {
        if (strstr(linea, "Usuario: ")) {
            sscanf(linea, "Usuario: %s", userA);
            
            if (strcmp(userA, user) == 0) {
                if (fgets(linea, sizeof(linea), fp)) {
                    sscanf(linea, "Password: %s", passA);
                    if (strcmp(passA, passE) == 0) {
                        encontrado = true;
                        break;
                    }
                }
            }
        }
    }
    fclose(fp);
    return encontrado;
}
