#include <ncurses.h>
#include <string.h>
#include "perfil.h"

void mostrar_perfil(Paciente *p) {
    clear();
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(2, 5, "--- EXPEDIENTE CLINICO CENTRALIZADO ---");
    attroff(COLOR_PAIR(4) | A_BOLD);

    char linea[256];
    char t_nom[50] = "N/A", t_ape[50] = "N/A", t_edad[10] = "N/A", t_sangre[10] = "N/A", t_correo[50] = "N/A";
    
    FILE *fp = fopen("usuarios.txt", "r");
    if (fp != NULL) {
        char r_nom[50] = "", r_ape[50] = "", r_edad[10] = "", r_sangre[10] = "", r_corr[50] = "", r_usr[50] = "";
        while (fgets(linea, sizeof(linea), fp)) {
            if (strncmp(linea, "Nombre: ", 8) == 0) sscanf(linea, "Nombre: %[^\n]", r_nom);
            else if (strncmp(linea, "Apellido: ", 10) == 0) sscanf(linea, "Apellido: %[^\n]", r_ape);
            else if (strncmp(linea, "Edad: ", 6) == 0) sscanf(linea, "Edad: %[^\n]", r_edad);
            else if (strncmp(linea, "Sangre: ", 8) == 0) sscanf(linea, "Sangre: %[^\n]", r_sangre);
            else if (strncmp(linea, "Correo: ", 8) == 0) sscanf(linea, "Correo: %[^\n]", r_corr);
            else if (strncmp(linea, "Usuario: ", 9) == 0) sscanf(linea, "Usuario: %[^\n]", r_usr);
            else if (strncmp(linea, "------------------------", 24) == 0) {
                if (strcmp(r_usr, p->usuario) == 0) {
                    strcpy(t_nom, r_nom);
                    strcpy(t_ape, r_ape);
                    strcpy(t_edad, r_edad);
                    strcpy(t_sangre, r_sangre);
                    strcpy(t_correo, r_corr);
                    
                    strcpy(p->nombre, r_nom);
                    strcpy(p->apellido, r_ape);
                    strcpy(p->edad, r_edad);
                    strcpy(p->sangre, r_sangre);
                    strcpy(p->correo, r_corr);
                    break;
                }
                r_nom[0] = '\0'; r_ape[0] = '\0'; r_edad[0] = '\0';
                r_sangre[0] = '\0'; r_corr[0] = '\0'; r_usr[0] = '\0';
            }
        }
        fclose(fp);
    }

    mvprintw(5, 5, "--------------------------------------------------");
    mvprintw(6, 5, "| Nombre(s):   %-33s |", t_nom);
    mvprintw(7, 5, "| Apellido(s): %-33s |", t_ape);
    mvprintw(8, 5, "| Edad:        %-33s |", t_edad);
    mvprintw(9, 5, "| Gr. Sanguineo:%-33s |", t_sangre);
    mvprintw(10, 5, "| Correo:      %-33s |", t_correo);
    mvprintw(11, 5, "| Identificador:%-33s |", p->usuario);
    mvprintw(12, 5, "--------------------------------------------------");
    
    mvprintw(15, 5, "Origen del dato: local/usuarios.txt estricto.");
    mvprintw(18, 5, "Presiona 'q' para regresar al panel principal.");
    refresh();
    int ch;
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');
}