#include "inter.h"
#include "admin.h"
#include "ui.h"
#include "validaciones.h"
#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern void mostrar_errores_formulario(char errores[7][100], int n_errores);
extern void mostrar_error_servidor(); // Aseguramos que cargue tu pantalla de error

void aplicar_xor(const char *origen, char *destino) {
    int i;
    for(i = 0; origen[i] != '\0'; i++) destino[i] = origen[i] ^ 0x0B;
    destino[i] = '\0';
}

// --- MODULO: CAPTURA DE RESULTADOS CLINICOS ---
// Se debe llamar cuando el admin selecciona un analisis especifico de un paciente
void capturar_resultados_analisis(char *usuario, char *categoria, char *nombre, char *componentes, int semid, MemoriaCompartida *memoria) {
    int en_captura = 1;
    int foco = 0;
    
    // Bandera lógica para cambiar la interfaz
    int es_rapida = (strcmp(categoria, "Pruebas_Rapidas") == 0);
    int resultado_rapido = 0; // 0 = Negativo, 1 = Positivo

    // Variables para Paquetes / Análisis Normales
    char nombres_comp[10][50] = {0};
    char valores_comp[10][20] = {0};
    int total_comp = 0;

	if (!es_rapida) {
        char temp_comp[256];
        strncpy(temp_comp, componentes, 255);
        char *token = strtok(temp_comp, ",");
        while (token && total_comp < 10) {
            while(*token == ' ') token++; // Limpiar espacios
            
            // Limpiar saltos de linea accidentales
            int len = strlen(token);
            while(len > 0 && (token[len-1] == '\n' || token[len-1] == '\r')) {
                token[len-1] = '\0';
                len--;
            }

            strncpy(nombres_comp[total_comp], token, 49);
            total_comp++;
            token = strtok(NULL, ",");
        }

        // --- NUEVA REGLA: Ocultar "Ninguno" y usar el Nombre del Analisis ---
        if (total_comp == 1 && strncmp(nombres_comp[0], "Ninguno", 7) == 0) {
            strncpy(nombres_comp[0], nombre, 49);
        }
    }

    int total_campos = es_rapida ? 1 : total_comp;

    while (en_captura) {
        clear();
        int alto = 16 + total_campos, ancho = 78;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        if (sx < 0) sx = 0;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); 
        mvprintw(sy + 2, sx + 22, "=== REGISTRO DE RESULTADOS ==="); 
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 4, "Paciente: %s", usuario);
        mvprintw(sy + 5, sx + 4, "Analisis: %s (%s)", nombre, categoria);
        mvhline(sy + 6, sx + 2, ACS_HLINE, ancho - 4);
        attroff(COLOR_PAIR(1));

        if (es_rapida) {
            mvprintw(sy + 8, sx + 4, "Resultado de la prueba:");
            
            if (foco == 0) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(4));
            // Flechas <- -> para alternar
            mvprintw(sy + 8, sx + 30, "< %s >", resultado_rapido ? "POSITIVO" : "NEGATIVO");
            attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(4));
        } else {
            mvprintw(sy + 8, sx + 4, "Ingresa los valores (usa numeros y decimales):");
            for (int i = 0; i < total_comp; i++) {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 10 + i, sx + 4, "%-20.20s :", nombres_comp[i]);
                
                if (foco == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(4));
                mvprintw(sy + 10 + i, sx + 26, "                    "); // Limpiar fondo
                mvprintw(sy + 10 + i, sx + 27, "%s", valores_comp[i]);
                
                if (foco == i) {
                    attron(A_BLINK);
                    mvprintw(sy + 10 + i, sx + 27 + strlen(valores_comp[i]), "_");
                    attroff(A_BLINK);
                }
                attroff(COLOR_PAIR(2) | A_BOLD | COLOR_PAIR(4));
            }
        }

        // Posicionamiento dinámico de botones
        int btn_guardar = total_campos;
        int btn_cancelar = total_campos + 1;

        if (foco == btn_guardar) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + alto - 3, sx + 20, " [ Guardar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (foco == btn_cancelar) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
        mvprintw(sy + alto - 3, sx + 45, " [ Cancelar ] "); attroff(COLOR_PAIR(5) | COLOR_PAIR(1) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + total_campos + 2) % (total_campos + 2);
        else if (ch == KEY_DOWN) foco = (foco + 1) % (total_campos + 2);
        else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            if (es_rapida && foco == 0) {
                resultado_rapido = !resultado_rapido; // Alterna entre 0 y 1
            }
        }
        else if (ch == '\n' || ch == '\r') {
            if (foco == btn_cancelar) return;
            else if (foco == btn_guardar) {
                char cadena_envio[512] = {0};
                if (es_rapida) {
                    snprintf(cadena_envio, 512, "ADMIN_RES_SAVE|%s|%s|%s", usuario, nombre, resultado_rapido ? "POSITIVO" : "NEGATIVO");
                } else {
                    // Armar la cadena estructurada: "Glucosa:90,Urea:20"
                    char res_comp[256] = {0};
                    for(int i = 0; i < total_comp; i++) {
                       char temp[100]; // Lo subimos a 100 para que sobre espacio
                       snprintf(temp, 100, "%s:%s", nombres_comp[i], valores_comp[i]);
                       strcat(res_comp, temp);
                       if (i < total_comp - 1) strcat(res_comp, ",");
                    }
                    snprintf(cadena_envio, 512, "ADMIN_RES_SAVE|%s|%s|%s", usuario, nombre, res_comp);
                }

                // Comunicación IPC
                if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                strncpy(memoria->mensaje, cadena_envio, 512);
                if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                
                mostrar_notificacion("Resultados guardados.", 1);
                en_captura = 0;
            } else {
                foco = (foco + 1) % (total_campos + 2);
            }
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (!es_rapida && foco < total_comp) { 
                int l = strlen(valores_comp[foco]); 
                if(l > 0) valores_comp[foco][l-1] = '\0'; 
            }
        }
        else if ((ch >= '0' && ch <= '9') || ch == '.') { 
            // Validacion estricta: Solo acepta números y punto decimal
            if (!es_rapida && foco < total_comp) { 
                int l = strlen(valores_comp[foco]); 
                if(l < 15) { // Limite de longitud del valor
                    valores_comp[foco][l] = ch; 
                    valores_comp[foco][l+1] = '\0'; 
                } 
            }
        }
    }
}



// --- MODULO: GESTIONAR USUARIOS ---
void gestionar_usuarios(int semid, MemoriaCompartida *memoria) {
    int buscar_nuevo = 1;
    
    while(buscar_nuevo) {
        buscar_nuevo = 0;
        char target_usr[50] = {0};
        int buscando = 1, foco_buscar = 0; 

        while(buscando) {
            clear();
            int alto = 16, ancho = 50;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);

            attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 13, "=== BUSCAR USUARIO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
            attron(COLOR_PAIR(1)); mvprintw(sy + 5, sx + 5, "Usuario:"); attroff(COLOR_PAIR(1));
            
            if (foco_buscar == 0) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4));
            mvprintw(sy + 5, sx + 15, "                        ");
            mvprintw(sy + 5, sx + 16, "%s", target_usr);
            if (foco_buscar == 0 && strlen(target_usr) < 20) { attron(A_BLINK); mvprintw(sy + 5, sx + 16 + strlen(target_usr), "_"); attroff(A_BLINK); }
            attroff(COLOR_PAIR(2) | COLOR_PAIR(4));

            if (foco_buscar == 1) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 9, sx + 18, " [ Buscar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            if (foco_buscar == 2) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 11, sx + 17, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
            refresh();

            int ch = getch();
            if (ch == KEY_UP) foco_buscar = (foco_buscar - 1 + 3) % 3;
            else if (ch == KEY_DOWN) foco_buscar = (foco_buscar + 1) % 3;
            else if (ch == '\n' || ch == '\r') {
                if (foco_buscar == 2) return; 
                else if (foco_buscar == 1) {
                    if (strlen(target_usr) > 0) buscando = 0; else mostrar_notificacion("Ingresa un usuario.", 0);
                } else { foco_buscar = 1; } 
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (foco_buscar == 0) { int len = strlen(target_usr); if (len > 0) target_usr[len-1] = '\0'; }
            }
            else if (ch >= 32 && ch <= 126) {
                if (foco_buscar == 0) { int len = strlen(target_usr); if (len < 20) { target_usr[len] = ch; target_usr[len+1] = '\0'; } }
            }
        }

        // --- PROTECCION DE CONEXION ---
        if (down(semid, MUTEX) == -1) mostrar_error_servidor();
        snprintf(memoria->mensaje, 512, "PERFIL_REQ | Usr: %s", target_usr);
        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
        
        char respuesta[512]; strncpy(respuesta, memoria->mensaje, 512);
        if (up(semid, MUTEX) == -1) mostrar_error_servidor();

        if (strcmp(respuesta, "VACIO") == 0) {
            mostrar_notificacion("ERROR: Usuario inexistente.", 0); buscar_nuevo = 1; continue;
        }

        char campos[7][50] = {0}; char *p;
        p = strstr(respuesta, "Paterno: "); if(p) sscanf(p+9, "%[^ |]", campos[0]);
        p = strstr(respuesta, "Materno: "); if(p) sscanf(p+9, "%[^ |]", campos[1]);
        p = strstr(respuesta, "Nombre: ");  if(p) sscanf(p+8, "%[^|]", campos[2]);
        int ln = strlen(campos[2]); while(ln>0 && campos[2][ln-1]==' ') campos[2][--ln]='\0';
        p = strstr(respuesta, "Sangre: ");  if(p) sscanf(p+8, "%[^ |]", campos[3]);
        p = strstr(respuesta, "Correo: ");  if(p) sscanf(p+8, "%[^ |]", campos[4]);
        p = strstr(respuesta, "Usr: ");     if(p) sscanf(p+5, "%[^ |]", campos[5]);
        
        char pass_encriptada[50] = {0};
        p = strstr(respuesta, "PassXOR: "); if(p) sscanf(p+9, "%s", pass_encriptada);
        aplicar_xor(pass_encriptada, campos[6]);

        int foco_edit = 0, en_edicion = 1;
        const char *titulos[] = {"Paterno:", "Materno:", "Nombre:", "Sangre:", "Correo:", "Usuario:", "Password:"};

        while(en_edicion) {
            clear();
            int alto = 26, ancho = 55;
            int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
            dibujar_tarjeta(sy, sx, alto, ancho);

            attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 1, sx + 13, "=== EDITAR PACIENTE ==="); attroff(COLOR_PAIR(3) | A_BOLD);

            for (int i = 0; i < 7; i++) {
                attron(COLOR_PAIR(1)); mvprintw(sy + 4 + (i * 2), sx + 4, "%-12s", titulos[i]);
                if (foco_edit == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
                mvprintw(sy + 4 + (i * 2), sx + 17, "                              "); 
                mvprintw(sy + 4 + (i * 2), sx + 18, "%s", campos[i]);
                if (foco_edit == i && strlen(campos[i]) < 28) { attron(A_BLINK); mvprintw(sy + 4 + (i * 2), sx + 18 + strlen(campos[i]), "_"); attroff(A_BLINK); }
                attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
            }

            if (foco_edit == 7) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 19, sx + 18, " [ Guardar Cambios ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

            if (foco_edit == 8) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
            mvprintw(sy + 21, sx + 20, " [ Eliminar Usr ] "); attroff(COLOR_PAIR(5) | COLOR_PAIR(1) | A_BOLD);

            if (foco_edit == 9) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 23, sx + 21, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
            refresh();

            int ch = getch();
            if (ch == KEY_UP) foco_edit = (foco_edit - 1 + 10) % 10;
            else if (ch == KEY_DOWN) foco_edit = (foco_edit + 1) % 10;
            else if (ch == '\n' || ch == '\r') {
                if (foco_edit == 9) { en_edicion = 0; buscar_nuevo = 1; } 
                else if (foco_edit == 8) { 
                    // --- PROTECCION DE CONEXION ---
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                    snprintf(memoria->mensaje, 512, "ADMIN_USR_DEL|%s", target_usr);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor();
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                    
                    mostrar_notificacion("Usuario eliminado.", 1);
                    en_edicion = 0; buscar_nuevo = 1;
                }
                else if (foco_edit == 7) { 
                    char errores[7][100]; int n_err = 0;
                    if (!validar_apellido(campos[0])) strcpy(errores[n_err++], "Paterno: Invalido");
                    if (!validar_apellido(campos[1])) strcpy(errores[n_err++], "Materno: Invalido");
                    if (!validar_nombre(campos[2])) strcpy(errores[n_err++], "Nombre: Invalido");
                    if (!validar_sangre(campos[3])) strcpy(errores[n_err++], "Sangre: Invalido");
                    if (!validar_correo(campos[4])) strcpy(errores[n_err++], "Correo: Invalido");
                    if (!validar_usuario(campos[5])) strcpy(errores[n_err++], "Usr: Invalido");
                    if (!validar_password(campos[6])) strcpy(errores[n_err++], "Pass: Invalida");

                    if (n_err > 0) mostrar_errores_formulario(errores, n_err);
                    else {
                        char nueva_pass[50]; aplicar_xor(campos[6], nueva_pass);
                        // --- PROTECCION DE CONEXION ---
                        if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                        snprintf(memoria->mensaje, 512, "ADMIN_USR_UPD|%.45s|%.45s|%.45s|%.45s|%.45s|%.45s|%.45s|%.45s", 
                                 target_usr, campos[0], campos[1], campos[2], campos[3], campos[4], campos[5], nueva_pass);
                        if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                        if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                        if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                        
                        mostrar_notificacion("Datos guardados con exito.", 1);
                        en_edicion = 0; buscar_nuevo = 1; 
                    }
                } else { foco_edit = (foco_edit + 1) % 10; }
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (foco_edit < 7) { int l = strlen(campos[foco_edit]); if(l>0) campos[foco_edit][l-1]='\0'; }
            }
            else if (ch >= 32 && ch <= 126) {
                if (foco_edit < 7) { int l = strlen(campos[foco_edit]); if(l<28) { campos[foco_edit][l]=ch; campos[foco_edit][l+1]='\0'; } }
            }
        }
    }
}

// --- MODULO: GESTIONAR ANALISIS (ESTILO BANDEJA/LISTA) ---
void gestionar_analisis(int semid, MemoriaCompartida *memoria) {
    int recargar = 1;
    char categorias[20][50] = {0}, subcats[20][50] = {0};
    char nombres[20][50] = {0}, precios[20][20] = {0}, componentes[20][150] = {0}; 
    int total = 0, foco_lista = 0;

    while(1) {
        if (recargar) {
            if (down(semid, MUTEX) == -1) mostrar_error_servidor();
            strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
            if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
            if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
            
            char resp[512]; strncpy(resp, memoria->mensaje, 512);
            if (up(semid, MUTEX) == -1) mostrar_error_servidor();

            total = 0; char *token = strtok(resp, ";");
            while(token && total < 20) {
                if(strlen(token) > 5) {
                    // CORRECCION 1: Cambio de %s a %[^\n] para aceptar espacios en los componentes
                    sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", 
                           categorias[total], subcats[total], nombres[total], precios[total], componentes[total]);
                    total++;
                }
                token = strtok(NULL, ";");
            }
            recargar = 0;
            if (foco_lista > total + 1) foco_lista = 0; 
        }

        clear();
        // CORRECCION 2: Reducimos ancho a 78 para que encaje en 80 columnas
        int alto = total + 12, ancho = 78; 
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        if (sx < 0) sx = 0; // Seguro para que nunca sea negativo
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 26, "=== CATALOGO CLINICO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 4, "Selecciona un analisis para EDITAR o presiona Crear Nuevo:");
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);
        attroff(COLOR_PAIR(1));

        for(int i=0; i<total; i++) {
            if (foco_lista == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            // Ajustamos el tamaño de impresión de la tabla
            mvprintw(sy + 6 + i, sx + 4, "%s [%s - %.10s] %-25s $%s ", 
                     (foco_lista == i) ? " >" : "  ", categorias[i], subcats[i], nombres[i], precios[i]);
            if (foco_lista == i) attroff(COLOR_PAIR(2) | A_BOLD); else attroff(COLOR_PAIR(1));
        }

        // Posiciones de los botones ajustadas al nuevo ancho
        if (foco_lista == total) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 8 + total, sx + 15, " [ + Crear Nuevo ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        if (foco_lista == total + 1) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 8 + total, sx + 45, " [ Regresar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco_lista = (foco_lista - 1 + total + 2) % (total + 2);
        else if (ch == KEY_DOWN) foco_lista = (foco_lista + 1) % (total + 2);
        else if (ch == '\n' || ch == '\r') {
            if (foco_lista == total + 1) return; 
            else if (foco_lista == total) {
                int c_foco = 0, c_en_edicion = 1;
                char c_campos[5][100] = {"Sangre", "Simple", "", "", "Ninguno"}; 
                const char *c_titulos[] = {"Categoria:", "Tipo (Sub):", "Nombre:", "Precio:", "Componentes:"};

                while(c_en_edicion) {
                    clear();
                    int e_alto = 24, e_ancho = 78; // Ajustado a 78
                    int ey = (LINES - e_alto) / 2, ex = (COLS - e_ancho) / 2;
                    if (ex < 0) ex = 0;
                    dibujar_tarjeta(ey, ex, e_alto, e_ancho);

                    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(ey + 1, ex + 28, "=== NUEVO ANALISIS ==="); attroff(COLOR_PAIR(3) | A_BOLD);
                    attron(COLOR_PAIR(5) | A_BOLD); mvprintw(ey + 3, ex + 15, "(Usa flechas DER/IZQ para Categoria y Tipo)"); attroff(COLOR_PAIR(5) | A_BOLD);

                    if(strcmp(c_campos[0], "Pruebas_Rapidas") == 0) { strcpy(c_campos[1], "Simple"); strcpy(c_campos[4], "Ninguno"); }

                    for (int i = 0; i < 5; i++) {
                        attron(COLOR_PAIR(1)); mvprintw(ey + 6 + (i * 2), ex + 4, "%-13s", c_titulos[i]);
                        if (c_foco == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
                        
                        // Barra en blanco recortada
                        mvprintw(ey + 6 + (i * 2), ex + 18, "                                                          "); 
                        
                        if ((i == 1 || i == 4) && strcmp(c_campos[0], "Pruebas_Rapidas") == 0) {
                            attron(COLOR_PAIR(1) | A_DIM); mvprintw(ey + 6 + (i * 2), ex + 19, "%s", c_campos[i]); attroff(COLOR_PAIR(1) | A_DIM);
                        } else {
                            if (i == 0 || i == 1) mvprintw(ey + 6 + (i * 2), ex + 19, "< %s >", c_campos[i]);
                            else mvprintw(ey + 6 + (i * 2), ex + 19, "%.55s", c_campos[i]);
                            if (c_foco == i && i > 1 && strlen(c_campos[i]) < 55) { attron(A_BLINK); mvprintw(ey + 6 + (i * 2), ex + 19 + strlen(c_campos[i]), "_"); attroff(A_BLINK); }
                        }
                        attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
                    }

                    if (c_foco == 5) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(ey + 18, ex + 20, " [ Guardar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

                    if (c_foco == 6) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
                    mvprintw(ey + 18, ex + 45, " [ Cancelar ] "); attroff(COLOR_PAIR(5) | COLOR_PAIR(1) | A_BOLD);

                    refresh();
                    int c_e = getch();
                    if (c_e == KEY_UP) c_foco = (c_foco - 1 + 7) % 7;
                    else if (c_e == KEY_DOWN) c_foco = (c_foco + 1) % 7;
                    else if (c_e == KEY_LEFT || c_e == KEY_RIGHT) {
                        if (c_foco == 0) {
                            if(strcmp(c_campos[0], "Sangre") == 0) strcpy(c_campos[0], "Orina");
                            else if(strcmp(c_campos[0], "Orina") == 0) strcpy(c_campos[0], "Pruebas_Rapidas");
                            else strcpy(c_campos[0], "Sangre");
                        } else if (c_foco == 1 && strcmp(c_campos[0], "Pruebas_Rapidas") != 0) {
                            if(strcmp(c_campos[1], "Simple") == 0) strcpy(c_campos[1], "Paquete");
                            else strcpy(c_campos[1], "Simple");
                        }
                    }
                    else if (c_e == '\n' || c_e == '\r') {
                        if (c_foco == 6) { c_en_edicion = 0; }
                        else if (c_foco == 5) { 
                            if(strlen(c_campos[2]) == 0 || strlen(c_campos[3]) == 0) {
                                mostrar_notificacion("Nombre y precio no pueden estar vacios", 0);
                            } else {
                                if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                                snprintf(memoria->mensaje, 512, "ADMIN_CAT_ADD|%.45s|%.45s|%.45s|%.45s|%.100s", 
                                         c_campos[0], c_campos[1], c_campos[2], c_campos[3], c_campos[4]);
                                if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                                if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                                if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                                
                                mostrar_notificacion("Analisis agregado.", 1);
                                c_en_edicion = 0; recargar = 1;
                            }
                        } else { c_foco = (c_foco + 1) % 7; }
                    }
                    else if (c_e == KEY_BACKSPACE || c_e == 127 || c_e == '\b') {
                        if (c_foco > 1 && c_foco < 5) { int l = strlen(c_campos[c_foco]); if(l>0) c_campos[c_foco][l-1]='\0'; }
                    }
                    else if (c_e >= 32 && c_e <= 126) {
                        // Limite de caracteres rebajado para que no exceda visualmente
                        if (c_foco > 1 && c_foco < 5) { int l = strlen(c_campos[c_foco]); if(l<55) { c_campos[c_foco][l]=c_e; c_campos[c_foco][l+1]='\0'; } }
                    }
                }
            }
            else {
                int foco_edit = 0, en_edicion = 1;
                char campos[5][100]; 
                strcpy(campos[0], categorias[foco_lista]); strcpy(campos[1], subcats[foco_lista]);
                strcpy(campos[2], nombres[foco_lista]); strcpy(campos[3], precios[foco_lista]);
                strcpy(campos[4], componentes[foco_lista]);
                const char *titulos[] = {"Categoria:", "Tipo (Sub):", "Nombre:", "Precio:", "Componentes:"};

                while(en_edicion) {
                    clear();
                    int e_alto = 24, e_ancho = 78; // Ajustado a 78
                    int ey = (LINES - e_alto) / 2, ex = (COLS - e_ancho) / 2;
                    if (ex < 0) ex = 0;
                    dibujar_tarjeta(ey, ex, e_alto, e_ancho);

                    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(ey + 1, ex + 28, "=== EDITAR ANALISIS ==="); attroff(COLOR_PAIR(3) | A_BOLD);

                    for (int i = 0; i < 5; i++) {
                        attron(COLOR_PAIR(1)); mvprintw(ey + 4 + (i * 2), ex + 4, "%-13s", titulos[i]);
                        if (foco_edit == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
                        
                        mvprintw(ey + 4 + (i * 2), ex + 18, "                                                          "); 
                        mvprintw(ey + 4 + (i * 2), ex + 19, "%.55s", campos[i]);
                        if (foco_edit == i && strlen(campos[i]) < 55) { attron(A_BLINK); mvprintw(ey + 4 + (i * 2), ex + 19 + strlen(campos[i]), "_"); attroff(A_BLINK); }
                        attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
                    }

                    if (foco_edit == 5) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(ey + 15, ex + 15, " [ Guardar Cambios ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

                    if (foco_edit == 6) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
                    mvprintw(ey + 17, ex + 32, " [ Eliminar ] "); attroff(COLOR_PAIR(5) | COLOR_PAIR(1) | A_BOLD);

                    if (foco_edit == 7) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(ey + 19, ex + 32, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
                    refresh();

                    int c_e = getch();
                    if (c_e == KEY_UP) foco_edit = (foco_edit - 1 + 8) % 8;
                    else if (c_e == KEY_DOWN) foco_edit = (foco_edit + 1) % 8;
                    else if (c_e == '\n' || c_e == '\r') {
                        if (foco_edit == 7) { en_edicion = 0; }
                        else if (foco_edit == 6) { 
                            if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                            snprintf(memoria->mensaje, 512, "ADMIN_CAT_DEL|%s", nombres[foco_lista]);
                            if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                            if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                            if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                            
                            mostrar_notificacion("Analisis eliminado.", 1);
                            en_edicion = 0; recargar = 1; foco_lista = 0;
                        }
                        else if (foco_edit == 5) { 
                            if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                            snprintf(memoria->mensaje, 512, "ADMIN_CAT_UPD|%.45s|%.45s|%.45s|%.45s|%.45s|%.100s", 
                                     nombres[foco_lista], campos[0], campos[1], campos[2], campos[3], campos[4]);
                            if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                            if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                            if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                            
                            mostrar_notificacion("Cambios guardados.", 1);
                            en_edicion = 0; recargar = 1;
                        } else { foco_edit = (foco_edit + 1) % 8; }
                    }
                    else if (c_e == KEY_BACKSPACE || c_e == 127 || c_e == '\b') {
                        if (foco_edit < 5) { int l = strlen(campos[foco_edit]); if(l>0) campos[foco_edit][l-1]='\0'; }
                    }
                    else if (c_e >= 32 && c_e <= 126) {
                        if (foco_edit < 5) { int l = strlen(campos[foco_edit]); if(l<55) { campos[foco_edit][l]=c_e; campos[foco_edit][l+1]='\0'; } }
                    }
                }
            }
        }
    }
}

// --- PUENTE DINAMICO: LISTA DE PACIENTES PENDIENTES ---
void menu_registrar_resultados(int semid, MemoriaCompartida *memoria) {
    int recargar = 1, ejecutando = 1;
    char pend_usr[20][50] = {0}, pend_prod[20][50] = {0};
    int total_pend = 0, foco = 0;

    while (ejecutando) {
        if (recargar) {
            // 1. Pedirle al servidor la lista de analisis pendientes
            if (down(semid, MUTEX) == -1) mostrar_error_servidor();
            strncpy(memoria->mensaje, "ADMIN_PENDIENTES_REQ", 512);
            if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
            if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
            
            char resp[512]; strncpy(resp, memoria->mensaje, 512);
            if (up(semid, MUTEX) == -1) mostrar_error_servidor();

            total_pend = 0;
            if (strcmp(resp, "VACIO") != 0) {
                char *token = strtok(resp, ";");
                while(token && total_pend < 20) {
                    // Desempaquetamos "usuario,producto"
                    sscanf(token, "%[^,],%[^\n]", pend_usr[total_pend], pend_prod[total_pend]);
                    total_pend++;
                    token = strtok(NULL, ";");
                }
            }
            recargar = 0;
            if (foco > total_pend) foco = 0;
        }

        clear();
        int alto = total_pend + 12; if (alto < 16) alto = 16;
        int ancho = 78;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        if (sx < 0) sx = 0;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 25, "=== BANDEJA DE PENDIENTES ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        if (total_pend == 0) {
            mvprintw(sy + 6, sx + 25, "No hay analisis pendientes.");
        } else {
            mvprintw(sy + 4, sx + 4, "Selecciona un paciente para registrar sus resultados:");
            mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);
            
            // Dibujar la lista scrolleable
            for(int i = 0; i < total_pend; i++) {
                if (foco == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                mvprintw(sy + 6 + i, sx + 4, "%s Paciente: %-15.15s | Analisis: %-25.25s", 
                         (foco == i) ? " >" : "  ", pend_usr[i], pend_prod[i]);
                if (foco == i) attroff(COLOR_PAIR(2) | A_BOLD); else attroff(COLOR_PAIR(1));
            }
        }
        attroff(COLOR_PAIR(1));

        // Boton de regresar posicionado dinámicamente al final
        if (foco == total_pend) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + alto - 3, sx + 32, " [ Regresar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco = (foco - 1 + total_pend + 1) % (total_pend + 1);
        else if (ch == KEY_DOWN) foco = (foco + 1) % (total_pend + 1);
        else if (ch == '\n' || ch == '\r') {
            if (foco == total_pend) {
                ejecutando = 0; // Rompe el ciclo y vuelve al menú Admin
            } else if (total_pend > 0) {
                // 2. Extraer del catálogo la información completa del análisis seleccionado
                if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
                if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor();
                
                char resp_cat[512]; strncpy(resp_cat, memoria->mensaje, 512);
                if (up(semid, MUTEX) == -1) mostrar_error_servidor();

                char c_cat[50]="", c_sub[50]="", c_nom[50]="", c_pre[20]="", c_comp[150]="";
                int encontrado = 0;
                char *token = strtok(resp_cat, ";");
                while(token) {
                    if(strlen(token) > 5) {
                        sscanf(token, "%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", c_cat, c_sub, c_nom, c_pre, c_comp);
                        if (strcmp(c_nom, pend_prod[foco]) == 0) { encontrado = 1; break; }
                    }
                    token = strtok(NULL, ";");
                }

                if (encontrado) {
                    // 3. Abrir la ventana dinámica de captura con los componentes listos
                    capturar_resultados_analisis(pend_usr[foco], c_cat, c_nom, c_comp, semid, memoria);
                    
                    // Activamos la bandera para que recargue la lista, ya que ese item desapareció (paso a COMPLETADO)
                    recargar = 1; 
                } else {
                    mostrar_notificacion("Error: Ese analisis fue borrado del catalogo base.", 0);
                }
            }
        }
    }
}


// --- MENU PRINCIPAL DEL ADMIN ACTUALIZADO ---
void abrir_panel_admin(int semid, MemoriaCompartida *memoria) {
    int opcion = 0, ejecutando = 1;
    // Agregamos la nueva opcion
    const char *opciones[5] = {
        "   Gestionar Usuarios               ", 
        "   Editar/Eliminar Analisis         ", 
        "   Registrar Resultados Medicos     ", // <--- NUEVA
        "   Ver Total de Ventas              ",
        "   Salir del Panel                  "
    };

    while(ejecutando) {
        clear();
        int alto = 22, ancho = 55; // Alto ajustado
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        if (sx < 0) sx = 0;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 13, "=== PANEL ADMINISTRADOR ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);

        for (int i = 0; i < 5; i++) {
            if (i == opcion) { attron(COLOR_PAIR(2) | A_BOLD); mvprintw(sy + 7 + (i * 2), sx + 8, "%s", opciones[i]); attroff(COLOR_PAIR(2) | A_BOLD); } 
            else { attron(COLOR_PAIR(1)); mvprintw(sy + 7 + (i * 2), sx + 8, "%s", opciones[i]); attroff(COLOR_PAIR(1)); }
        }
        refresh();

        int ch = getch();
        switch(ch) {
            case KEY_UP: opcion = (opcion - 1 + 5) % 5; break;
            case KEY_DOWN: opcion = (opcion + 1) % 5; break;
            case '\n': case '\r':
                if (opcion == 0) { gestionar_usuarios(semid, memoria); } 
                else if (opcion == 1) { gestionar_analisis(semid, memoria); } 
                else if (opcion == 2) { menu_registrar_resultados(semid, memoria); } // <--- LLAMA A LA FUNCION
                else if (opcion == 3) {
                    if (down(semid, MUTEX) == -1) mostrar_error_servidor();
                    strncpy(memoria->mensaje, "ADMIN_VENTAS_REQ", 512);
                    if (up(semid, CLIENTE_LISTO) == -1) mostrar_error_servidor(); 
                    if (down(semid, SERVIDOR_LISTO) == -1) mostrar_error_servidor(); 
                    char respuesta[512]; strncpy(respuesta, memoria->mensaje, 512);
                    if (up(semid, MUTEX) == -1) mostrar_error_servidor();
                    mostrar_notificacion(respuesta, 1);
                } 
                else if (opcion == 4) { ejecutando = 0; }
                break;
        }
    }
}
