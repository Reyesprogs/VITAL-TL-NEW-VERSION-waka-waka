#include "admin.h"
#include "ui.h"
#include "validaciones.h"
#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern void mostrar_errores_formulario(char errores[7][100], int n_errores);

void aplicar_xor(const char *origen, char *destino) {
    int i;
    for(i = 0; origen[i] != '\0'; i++) destino[i] = origen[i] ^ 0x0B;
    destino[i] = '\0';
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

        if (down(semid, MUTEX) == -1) return;
        snprintf(memoria->mensaje, 512, "PERFIL_REQ | Usr: %s", target_usr);
        up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO);
        char respuesta[512]; strncpy(respuesta, memoria->mensaje, 512);
        up(semid, MUTEX);

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
                    if (down(semid, MUTEX) != -1) {
                        snprintf(memoria->mensaje, 512, "ADMIN_USR_DEL|%s", target_usr);
                        up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                        mostrar_notificacion("Usuario eliminado.", 1);
                    }
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
                        if (down(semid, MUTEX) != -1) {
                            snprintf(memoria->mensaje, 512, "ADMIN_USR_UPD|%s|%s|%s|%s|%s|%s|%s|%s", 
                                     target_usr, campos[0], campos[1], campos[2], campos[3], campos[4], campos[5], nueva_pass);
                            up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                            mostrar_notificacion("Datos guardados con exito.", 1);
                        }
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
    char nombres[20][50] = {0}; 
    char precios[20][20] = {0}; 
    int total = 0;
    int foco_lista = 0;

    while(1) {
        if (recargar) {
            if (down(semid, MUTEX) == -1) return;
            strncpy(memoria->mensaje, "CATALOGO_REQ", 512);
            up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO);
            char resp[512]; strncpy(resp, memoria->mensaje, 512);
            up(semid, MUTEX);

            total = 0; char *token = strtok(resp, ",");
            while(token && total < 20) {
                if(strlen(token) > 2) {
                    sscanf(token, "%[^|]|%s", nombres[total], precios[total]);
                    total++;
                }
                token = strtok(NULL, ",");
            }
            recargar = 0;
            if (total == 0) { mostrar_notificacion("Catalogo vacio.", 0); return; }
        }

        clear();
        int alto = total + 10, ancho = 60;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 16, "=== CATALOGO CLINICO ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        
        attron(COLOR_PAIR(1));
        mvprintw(sy + 4, sx + 4, "Selecciona un analisis para EDITAR o ELIMINAR:");
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4);
        attroff(COLOR_PAIR(1));

        for(int i=0; i<total; i++) {
            if (foco_lista == i) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(sy + 6 + i, sx + 6, " > %-30s $%s ", nombres[i], precios[i]);
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(sy + 6 + i, sx + 6, "   %-30s $%s ", nombres[i], precios[i]);
                attroff(COLOR_PAIR(1));
            }
        }

        if (foco_lista == total) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 7 + total, sx + 24, " [ Regresar ] ");
        attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco_lista = (foco_lista - 1 + total + 1) % (total + 1);
        else if (ch == KEY_DOWN) foco_lista = (foco_lista + 1) % (total + 1);
        else if (ch == '\n' || ch == '\r') {
            if (foco_lista == total) return; 
            else {
                char old_nom[50], old_prc[20];
                strcpy(old_nom, nombres[foco_lista]); strcpy(old_prc, precios[foco_lista]);
                
                int foco_edit = 0, en_edicion = 1;
                char campos[2][50]; strcpy(campos[0], old_nom); strcpy(campos[1], old_prc);
                const char *titulos[] = {"Nombre:", "Precio:"};

                while(en_edicion) {
                    clear();
                    int e_alto = 20, e_ancho = 55;
                    int ey = (LINES - e_alto) / 2, ex = (COLS - e_ancho) / 2;
                    dibujar_tarjeta(ey, ex, e_alto, e_ancho);

                    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(ey + 1, ex + 12, "=== EDITAR ANALISIS ==="); attroff(COLOR_PAIR(3) | A_BOLD);

                    for (int i = 0; i < 2; i++) {
                        attron(COLOR_PAIR(1)); mvprintw(ey + 4 + (i * 2), ex + 5, "%-10s", titulos[i]);
                        if (foco_edit == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4)); 
                        mvprintw(ey + 4 + (i * 2), ex + 15, "                              "); 
                        mvprintw(ey + 4 + (i * 2), ex + 16, "%s", campos[i]);
                        if (foco_edit == i && strlen(campos[i]) < 28) { attron(A_BLINK); mvprintw(ey + 4 + (i * 2), ex + 16 + strlen(campos[i]), "_"); attroff(A_BLINK); }
                        attroff(COLOR_PAIR(2) | COLOR_PAIR(4));
                    }

                    if (foco_edit == 2) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(ey + 11, ex + 18, " [ Guardar Cambios ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);

                    if (foco_edit == 3) attron(COLOR_PAIR(5) | A_BOLD); else attron(COLOR_PAIR(1)); 
                    mvprintw(ey + 13, ex + 20, " [ Eliminar ] "); attroff(COLOR_PAIR(5) | COLOR_PAIR(1) | A_BOLD);

                    if (foco_edit == 4) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(ey + 15, ex + 20, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
                    refresh();

                    int c_e = getch();
                    if (c_e == KEY_UP) foco_edit = (foco_edit - 1 + 5) % 5;
                    else if (c_e == KEY_DOWN) foco_edit = (foco_edit + 1) % 5;
                    else if (c_e == '\n' || c_e == '\r') {
                        if (foco_edit == 4) { en_edicion = 0; }
                        else if (foco_edit == 3) { 
                            if (down(semid, MUTEX) != -1) {
                                snprintf(memoria->mensaje, 512, "ADMIN_CAT_DEL|%s", old_nom);
                                up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                                mostrar_notificacion("Analisis eliminado.", 1);
                            }
                            en_edicion = 0; recargar = 1; foco_lista = 0;
                        }
                        else if (foco_edit == 2) { 
                            if (strlen(campos[0]) == 0 || strlen(campos[1]) == 0) mostrar_notificacion("Campos vacios.", 0);
                            else {
                                if (down(semid, MUTEX) != -1) {
                                    snprintf(memoria->mensaje, 512, "ADMIN_CAT_UPD|%s|%s|%s", old_nom, campos[0], campos[1]);
                                    up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                                    mostrar_notificacion("Cambios guardados.", 1);
                                }
                                en_edicion = 0; recargar = 1;
                            }
                        } else { foco_edit = (foco_edit + 1) % 5; }
                    }
                    else if (c_e == KEY_BACKSPACE || c_e == 127 || c_e == '\b') {
                        if (foco_edit < 2) { int l = strlen(campos[foco_edit]); if(l>0) campos[foco_edit][l-1]='\0'; }
                    }
                    else if (c_e >= 32 && c_e <= 126) {
                        if (foco_edit < 2) { int l = strlen(campos[foco_edit]); if(l<28) { campos[foco_edit][l]=c_e; campos[foco_edit][l+1]='\0'; } }
                    }
                }
            }
        }
    }
}


// --- DIBUJAR GRAFICA DE BARRAS (VERTICAL TIPO ALSAMIXER) ---
void mostrar_grafica_admin(const char *titulo, char nombres[4][20], char valores[4][15], int num_campos) {
    clear();
    int alto = 20, ancho = 65;
    int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
    dibujar_tarjeta(sy, sx, alto, ancho);
    
    attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy+2, sx+16, "=== GRAFICA DE RESULTADOS ==="); attroff(COLOR_PAIR(3)|A_BOLD);
    attron(COLOR_PAIR(1) | A_BOLD); mvprintw(sy+4, sx+5, "%s", titulo); attroff(COLOR_PAIR(1) | A_BOLD);
    mvhline(sy+5, sx+4, ACS_HLINE, ancho-8);
    
    int max_altura = 8;
    int base_y = sy + 14; 
    
    for(int i=0; i<num_campos; i++) {
        int val = atoi(valores[i]);
        int barras = (val * max_altura) / 200; // Escala base a 200 puntos
        if(barras > max_altura) barras = max_altura;
        if(barras <= 0 && val > 0) barras = 1;
        
        int pos_x = sx + 8 + (i * 12); // Separación horizontal
        
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(base_y - barras - 1, pos_x, "%3d", val);
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(2) | A_REVERSE);
        for(int b=0; b<barras; b++) {
            mvprintw(base_y - b, pos_x, "   "); 
        }
        attroff(COLOR_PAIR(2) | A_REVERSE);

        attron(COLOR_PAIR(1));
        char short_lbl[6];
        strncpy(short_lbl, nombres[i], 5); short_lbl[5] = '\0';
        mvprintw(base_y + 1, pos_x - 1, "%5s", short_lbl);
        attroff(COLOR_PAIR(1));
    }
    
    attron(COLOR_PAIR(1)|A_BOLD); mvprintw(sy+17, sx+25, " [ Continuar ] "); attroff(COLOR_PAIR(1)|A_BOLD);
    refresh();
    while(getch() != '\n' && getch() != '\r');
}

// --- MODULO: INGRESAR RESULTADOS A PACIENTES ---
void agregar_resultados(int semid, MemoriaCompartida *memoria) {
    int recargar = 1;
    char usuarios[20][50] = {0}; char productos[20][50] = {0}; int total = 0;
    int foco_lista = 0;

    while(1) {
        if (recargar) {
            if (down(semid, MUTEX) == -1) return;
            strncpy(memoria->mensaje, "ADMIN_PEND_REQ", 512);
            up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO);
            char resp[512]; strncpy(resp, memoria->mensaje, 512);
            up(semid, MUTEX);

            total = 0; char *token = strtok(resp, ",");
            while(token && total < 20) {
                if(strlen(token) > 2) {
                    sscanf(token, "%[^|]|%s", usuarios[total], productos[total]);
                    total++;
                }
                token = strtok(NULL, ",");
            }
            recargar = 0;
            if (total == 0) { mostrar_notificacion("No hay analisis pendientes.", 0); return; }
        }

        clear();
        int alto = total + 12, ancho = 65;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
        dibujar_tarjeta(sy, sx, alto, ancho);

        attron(COLOR_PAIR(3) | A_BOLD); mvprintw(sy + 2, sx + 20, "=== BANDEJA DE PENDIENTES ==="); attroff(COLOR_PAIR(3) | A_BOLD);
        attron(COLOR_PAIR(1)); mvprintw(sy + 4, sx + 4, "Selecciona un paciente para ingresar resultados:");
        mvhline(sy + 5, sx + 2, ACS_HLINE, ancho - 4); attroff(COLOR_PAIR(1));

        for(int i=0; i<total; i++) {
            if (foco_lista == i) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
            mvprintw(sy + 6 + i, sx + 6, "%s | %-35s ", usuarios[i], productos[i]);
            if (foco_lista == i) attroff(COLOR_PAIR(2) | A_BOLD); else attroff(COLOR_PAIR(1));
        }

        if (foco_lista == total) attron(COLOR_PAIR(2) | A_BOLD); else attron(COLOR_PAIR(1));
        mvprintw(sy + 8 + total, sx + 25, " [ Regresar ] "); attroff(COLOR_PAIR(2) | COLOR_PAIR(1) | A_BOLD);
        refresh();

        int ch = getch();
        if (ch == KEY_UP) foco_lista = (foco_lista - 1 + total + 1) % (total + 1);
        else if (ch == KEY_DOWN) foco_lista = (foco_lista + 1) % (total + 1);
        else if (ch == '\n' || ch == '\r') {
            if (foco_lista == total) return; 
            else {
                int es_sino = 0;
                int num_campos = 4;
                char titulos[4][20];
                char campos_res[4][15] = {"0", "0", "0", "0"}; 
                char target_prod[50]; strcpy(target_prod, productos[foco_lista]);
                
                char tmp[50]; for(int i=0; target_prod[i]; i++) tmp[i] = tolower(target_prod[i]);
                
                if (strstr(tmp, "sifilis") || strstr(tmp, "vih") || strstr(tmp, "vdrl")) {
                    es_sino = 1; num_campos = 1;
                    strcpy(titulos[0], "Resultado:"); 
                } else if (strstr(tmp, "orina")) {
                    strcpy(titulos[0], "PH (Acidez):"); strcpy(titulos[1], "Densidad:");
                    strcpy(titulos[2], "Proteinas:"); strcpy(titulos[3], "Leucocitos:");
                } else { 
                    strcpy(titulos[0], "Glucosa:"); strcpy(titulos[1], "Colesterol:");
                    strcpy(titulos[2], "Trigliseridos:"); strcpy(titulos[3], "Acido Urico:");
                }

                int foco_form = 0, editando = 1;
                int val_sino = 0; // Para el boton toggle (0=Negativo, 1=Positivo)

                while(editando) {
                    clear();
                    int h = (num_campos * 2) + 12, w = 55;
                    int dy = (LINES - h) / 2, dx = (COLS - w) / 2;
                    dibujar_tarjeta(dy, dx, h, w);
                    
                    attron(COLOR_PAIR(3)|A_BOLD); mvprintw(dy+2, dx+16, "=== INGRESAR DATOS ==="); attroff(COLOR_PAIR(3)|A_BOLD);
                    attron(COLOR_PAIR(1)|A_BOLD); mvprintw(dy+4, dx+5, "Paciente: %s", usuarios[foco_lista]); attroff(COLOR_PAIR(1)|A_BOLD);
                    
                    for(int i=0; i<num_campos; i++) {
                        attron(COLOR_PAIR(1)); mvprintw(dy+7+(i*2), dx+5, "%-15s", titulos[i]); attroff(COLOR_PAIR(1));
                        
                        if (es_sino) {
                            if(foco_form == i) attron(COLOR_PAIR(2)|A_REVERSE); else attron(COLOR_PAIR(4));
                            mvprintw(dy+7+(i*2), dx+21, "               ");
                            mvprintw(dy+7+(i*2), dx+23, "< %s >", val_sino == 0 ? "NEGATIVO" : "POSITIVO");
                            attroff(COLOR_PAIR(2)|A_REVERSE|COLOR_PAIR(4));
                        } else {
                            if(foco_form == i) attron(COLOR_PAIR(2)); else attron(COLOR_PAIR(4));
                            mvprintw(dy+7+(i*2), dx+21, "               ");
                            mvprintw(dy+7+(i*2), dx+22, "%s", campos_res[i]);
                            if(foco_form == i && strlen(campos_res[i]) < 12) { attron(A_BLINK); mvprintw(dy+7+(i*2), dx+22+strlen(campos_res[i]), "_"); attroff(A_BLINK); }
                            attroff(COLOR_PAIR(2)|COLOR_PAIR(4));
                        }
                    }

                    if(foco_form == num_campos) attron(COLOR_PAIR(2)|A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(dy+h-6, dx+18, " [ Guardar Datos ] "); attroff(COLOR_PAIR(2)|A_BOLD|COLOR_PAIR(1));

                    if(foco_form == num_campos+1) attron(COLOR_PAIR(5)|A_BOLD); else attron(COLOR_PAIR(1));
                    mvprintw(dy+h-4, dx+20, " [ Cancelar ] "); attroff(COLOR_PAIR(5)|A_BOLD|COLOR_PAIR(1));
                    refresh();

                    int c3 = getch();
                    if(c3 == KEY_UP) foco_form = (foco_form - 1 + num_campos + 2) % (num_campos + 2);
                    else if (c3 == KEY_DOWN) foco_form = (foco_form + 1) % (num_campos + 2);
                    else if (c3 == KEY_LEFT || c3 == KEY_RIGHT) {
                        if (es_sino && foco_form == 0) val_sino = !val_sino; 
                    }
                    else if (c3 == '\n' || c3 == '\r') {
                        if (es_sino && foco_form == 0) {
                            val_sino = !val_sino; 
                        }
                        else if(foco_form == num_campos+1) { editando = 0; } 
                        else if(foco_form == num_campos) { 
                            if (es_sino) strcpy(campos_res[0], val_sino == 0 ? "NEGATIVO" : "POSITIVO");
                            
                            char meta_data[200] = "";
                            for(int i=0; i<num_campos; i++) {
                                char temp_dt[50]; snprintf(temp_dt, 50, "%s:%s|", titulos[i], campos_res[i]);
                                strcat(meta_data, temp_dt);
                            }

                            if(down(semid, MUTEX) != -1) {
                                snprintf(memoria->mensaje, 512, "ADMIN_RES_SAVE|%s|%s|%s", usuarios[foco_lista], productos[foco_lista], meta_data);
                                up(semid, CLIENTE_LISTO); down(semid, SERVIDOR_LISTO); up(semid, MUTEX);
                                mostrar_notificacion("Resultados adjuntados exitosamente.", 1);
                            }

                            if(!es_sino) {
                                mostrar_grafica_admin(productos[foco_lista], titulos, campos_res, num_campos);
                            }

                            editando = 0; recargar = 1; foco_lista = 0;
                        } else { foco_form = (foco_form + 1) % (num_campos + 2); }
                    }
                    else if (c3 == KEY_BACKSPACE || c3 == 127 || c3 == '\b') {
                        if(!es_sino && foco_form < num_campos) { int l=strlen(campos_res[foco_form]); if(l>0) campos_res[foco_form][l-1]='\0'; }
                    }
                    else if (c3 >= 32 && c3 <= 126) {
                        if(!es_sino && foco_form < num_campos) { 
                            int l=strlen(campos_res[foco_form]); 
                            if(l<12) {
                                if(isdigit(c3)) { campos_res[foco_form][l]=c3; campos_res[foco_form][l+1]='\0'; }
                            } 
                        }
                    }
                }
            }
        }
    }
}

// --- MENU PRINCIPAL DEL ADMIN ---
void abrir_panel_admin(int semid, MemoriaCompartida *memoria) {
    int opcion = 0, ejecutando = 1;
    const char *opciones[5] = {
        "   Gestionar Usuarios               ", 
        "   Editar/Eliminar Analisis         ", 
        "   Agregar Resultado a Paciente     ",
        "   Ver Total de Ventas              ",
        "   Salir del Panel                  "
    };

    while(ejecutando) {
        clear();
        int alto = 20, ancho = 55;
        int sy = (LINES - alto) / 2, sx = (COLS - ancho) / 2;
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
                else if (opcion == 2) { agregar_resultados(semid, memoria); } 
                else if (opcion == 3) {
                    if (down(semid, MUTEX) != -1) {
                        strncpy(memoria->mensaje, "ADMIN_VENTAS_REQ", 512);
                        if (up(semid, CLIENTE_LISTO) != -1 && down(semid, SERVIDOR_LISTO) != -1) {
                            char respuesta[512]; strncpy(respuesta, memoria->mensaje, 512);
                            up(semid, MUTEX); mostrar_notificacion(respuesta, 1);
                        }
                    }
                } 
                else if (opcion == 4) { ejecutando = 0; }
                break;
        }
    }
}
