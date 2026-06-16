#include <stdio.h>
#include <string.h>
#include "historial.h"
#include "semaforo.h"

void guardar_compra(const char* usuario, const char* estudio) {
    FILE *fp = fopen("historial.txt", "a");
    if (fp != NULL) {

        fprintf(fp, "Usuario: %s\nEstudio: %s\nValor: -2\n------------------------\n", usuario, estudio);
        fclose(fp);
    }
}

void cargar_historial(const char* usuario, Analisis *datos) {
    bloquear(&datos->candado);

    datos->hemoglobina_cant = 0;
    datos->glucosa_cant = 0;
    datos->orina_cant = 0;
    datos->colesterol_cant = 0;
    datos->trigliceridos_cant = 0;

    FILE *fp = fopen("historial.txt", "r");
    if (fp == NULL) {
        desbloquear(&datos->candado);
        return;
    }

    char linea[100];
    char t_usr[50] = "", t_est[50] = "";
    int t_val = 0;

    while (fgets(linea, sizeof(linea), fp)) {
        if (strncmp(linea, "Usuario: ", 9) == 0) {
            sscanf(linea, "Usuario: %s", t_usr);
        } else if (strncmp(linea, "Estudio: ", 9) == 0) {
            sscanf(linea, "Estudio: %[^\n]", t_est);
        } else if (strncmp(linea, "Valor: ", 7) == 0) {
            sscanf(linea, "Valor: %d", &t_val);
            
            if (strcmp(t_usr, usuario) == 0) {
                if (strcmp(t_est, "Biometria Hematica") == 0 && datos->hemoglobina_cant < MAX_MUESTRAS) {
                    datos->hemoglobina[datos->hemoglobina_cant++] = t_val;
                } else if (strcmp(t_est, "Glucosa") == 0 && datos->glucosa_cant < MAX_MUESTRAS) {
                    datos->glucosa[datos->glucosa_cant++] = t_val;
                } else if (strcmp(t_est, "Orina") == 0 && datos->orina_cant < MAX_MUESTRAS) {
                    datos->orina[datos->orina_cant++] = t_val;
                } else if (strcmp(t_est, "Colesterol") == 0 && datos->colesterol_cant < MAX_MUESTRAS) {
                    datos->colesterol[datos->colesterol_cant++] = t_val;
                } else if (strcmp(t_est, "Trigliceridos") == 0 && datos->trigliceridos_cant < MAX_MUESTRAS) {
                    datos->trigliceridos[datos->trigliceridos_cant++] = t_val;
                }
            }
        }
    }
    fclose(fp);
    desbloquear(&datos->candado);
}
