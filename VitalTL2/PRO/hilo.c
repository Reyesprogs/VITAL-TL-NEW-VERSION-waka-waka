#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "datos.h"
#include "hilo.h"
#include "semaforo.h"
#include "chamcont.h" 


void* simular(void* arg) {
    Analisis *datos = (Analisis*)arg;

    while(1) {
       
        bloquear(&datos->candado);

     
        if (datos->peticion_activa == 1) {
            
		if (datos->tipo_peticion == 1) { 
		                datos->respuesta_servidor = 0; 
		
		             
		                if (strcmp(datos->req_usuario, "admin") == 0 && strcmp(datos->req_pass, "admin123") == 0) {
		                    datos->respuesta_servidor = 2; 
		                    datos->peticion_activa = 0;
		                } 
	
		                else {
		                    FILE *fp = fopen("usuarios.txt", "r");
		                    if (fp != NULL) {
		                        char linea[256], passA[50];
		                        char t_nom[50]="", t_ape[50]="", t_edad[10]="", t_sangre[10]="", t_corr[50]="", t_usr[50]="";
		
		                        char passE[50];
		                        strcpy(passE, datos->req_pass);
		                        encriptar(passE); 
		
		                        while (fgets(linea, sizeof(linea), fp)) {
		                         
		                            if (strncmp(linea, "Nombre: ", 8) == 0) sscanf(linea, "Nombre: %[^\n]", t_nom);
		                            else if (strncmp(linea, "Apellido: ", 10) == 0) sscanf(linea, "Apellido: %[^\n]", t_ape);
		                            else if (strncmp(linea, "Edad: ", 6) == 0) sscanf(linea, "Edad: %[^\n]", t_edad);
		                            else if (strncmp(linea, "Sangre: ", 8) == 0) sscanf(linea, "Sangre: %[^\n]", t_sangre);
		                            else if (strncmp(linea, "Correo: ", 8) == 0) sscanf(linea, "Correo: %[^\n]", t_corr);
		                            else if (strncmp(linea, "Usuario: ", 9) == 0) {
		                                sscanf(linea, "Usuario: %[^\n]", t_usr);
		                     
		                                if (strcmp(t_usr, datos->req_usuario) == 0) {
		                                    if (fgets(linea, sizeof(linea), fp)) {
		                                        sscanf(linea, "Password: %[^\n]", passA);
		                                        
		                                      
		                                        if (strcmp(passA, passE) == 0) {
		                                        
		                                            strcpy(datos->req_nombre, t_nom);
		                                            strcpy(datos->req_apellido, t_ape);
		                                            strcpy(datos->req_edad, t_edad);
		                                            strcpy(datos->req_sangre, t_sangre);
		                                            strcpy(datos->req_correo, t_corr);
		                                            datos->respuesta_servidor = 1;
		                                            break; 
		                                        }
		                                    }
		                                }
		                            }
		                        }
		                        fclose(fp); 
		                    }
		                    
		             
		                    datos->peticion_activa = 0; 
		                }
		            }

		}
            
        desbloquear(&datos->candado);
        
        usleep(10000); 
    }
    return NULL;
}


void iniciar_hilo(pthread_t *hilo, Analisis *datos) {
    pthread_create(hilo, NULL, simular, (void*)datos);
}

void* procesar_cliente_privado(void* arg) {
    InfoClienteHilo *info = (InfoClienteHilo*)arg;
    ConexionCliente *cliente = info->datos;
    
    printf("[SERVIDOR] Hilo iniciado para cliente PID=%d\n", info->pid_cliente);
    fflush(stdout);
    
    
    while(1) {
        bloquear(&cliente->candado);
        
        if (cliente->peticion_activa == 1) {
            
            printf("[Accion] PID Cliente: %d |%s\n", 
                   cliente->pid_cliente, cliente->log_mensaje);
            fflush(stdout);
            
            if (cliente->tipo_peticion == 1) { 
                cliente->respuesta_servidor = 0;
                
                
                if (strcmp(cliente->req_usuario, "admin") == 0 && strcmp(cliente->req_pass, "admin") == 0) {
                    cliente->respuesta_servidor = 2;
                    printf("[SERVIDOR] [PID %d] Acceso concedido: Modo Administrador.\n", info->pid_cliente);
                }
                
                else if (validar_credenciales(cliente->req_usuario, cliente->req_pass)) {
                    cliente->respuesta_servidor = 1;
                    printf("[SERVIDOR] [PID %d] Acceso concedido: Paciente %s.\n", 
                           info->pid_cliente, cliente->req_usuario);
                }
                else {
                    cliente->respuesta_servidor = 0;
                    printf("[SERVIDOR] [PID %d] Acceso DENEGADO para: %s.\n", 
                           info->pid_cliente, cliente->req_usuario);
                }
            }
            else if (cliente->tipo_peticion == 2) { 
                cliente->respuesta_servidor = 3;
                printf("[SERVIDOR] [PID %d] Solicitud de desconexion recibida.\n", info->pid_cliente);
                
            }
            
            fflush(stdout);
            cliente->peticion_activa = 0;
            cliente->heartbeat_servidor++;
        }
        
        desbloquear(&cliente->candado);
        usleep(10000);
    }
    
    free(info);
    return NULL;
}

void iniciar_hilo_cliente(pthread_t *hilo, InfoClienteHilo *info_cliente) {
    pthread_create(hilo, NULL, procesar_cliente_privado, (void*)info_cliente);
}
