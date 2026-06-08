#include "validaciones.h"
#include <string.h>
#include <ctype.h>

int validar_apellido(const char *str) {
    int len = strlen(str);
    if (len == 0) return 0;
    if (!isupper(str[0])) return 0; // Debe empezar con mayúscula
    for (int i = 1; i < len; i++) {
        if (!islower(str[i])) return 0; // El resto minúsculas, sin espacios
    }
    return 1;
}

int validar_nombre(const char *str) {
    int len = strlen(str);
    if (len == 0) return 0;
    
    int espacios = 0;
    int inicio_palabra = 1;

    for (int i = 0; i < len; i++) {
        if (str[i] == ' ') {
            espacios++;
            if (espacios > 1) return 0; // Máximo 2 palabras (1 espacio)
            inicio_palabra = 1;
        } else {
            if (inicio_palabra) {
                if (!isupper(str[i])) return 0; // Inicio de palabra en mayúscula
                inicio_palabra = 0;
            } else {
                if (!islower(str[i])) return 0;
            }
        }
    }
    return 1;
}

int validar_sangre(const char *str) {
    // Solo existen estos tipos de sangre válidos
    const char *tipos[] = {"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"};
    for (int i = 0; i < 8; i++) {
        if (strcmp(str, tipos[i]) == 0) return 1;
    }
    return 0;
}

int validar_correo(const char *str) {
    // Debe tener al menos un '@' y un '.'
    if (strchr(str, '@') != NULL && strchr(str, '.') != NULL) return 1;
    return 0;
}

int validar_usuario(const char *str) {
    int len = strlen(str);
    if (len < 5 || len > 15) return 0;
    
    for (int i = 0; i < len; i++) {
        // Solo minúsculas, números o guion bajo
        if (!islower(str[i]) && !isdigit(str[i]) && str[i] != '_') return 0;
    }
    return 1;
}

int validar_password(const char *str) {
    int len = strlen(str);
    if (len < 8) return 0;

    int upper = 0, lower = 0, digit = 0, symbol = 0;
    for (int i = 0; i < len; i++) {
        if (isupper(str[i])) upper++;
        else if (islower(str[i])) lower++;
        else if (isdigit(str[i])) digit++;
        else if (ispunct(str[i])) symbol++; // ispunct detecta simbolos ASCII
    }
    
    if (upper > 0 && lower > 0 && digit > 0 && symbol > 0) return 1;
    return 0;
}

// Encriptación rápida y segura con operador Bitwise XOR
void encriptar_ascii(const char *password, char *salida) {
    char llave = 0x0B; // Llave numérica secreta para la operación XOR
    int i;
    for (i = 0; password[i] != '\0'; i++) {
        salida[i] = password[i] ^ llave; // Ofusca la letra
    }
    salida[i] = '\0';
}
