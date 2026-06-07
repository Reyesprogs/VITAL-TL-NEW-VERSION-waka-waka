#ifndef VALIDACIONES_H
#define VALIDACIONES_H

// Prototipos de funciones de validación
int validar_apellido(const char *str);
int validar_nombre(const char *str);
int validar_sangre(const char *str);
int validar_correo(const char *str);
int validar_usuario(const char *str);
int validar_password(const char *str);

// Prototipo de encriptación ASCII
void encriptar_ascii(const char *password, char *salida);

#endif
