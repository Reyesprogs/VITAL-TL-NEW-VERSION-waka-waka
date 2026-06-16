# 🚀 Guía Rápida: Sistema de Comunicación Privada

## ¿Qué cambió?

### Antes ❌
```
┌─────────────────┐     GLOBAL        ┌─────────────────┐
│   CLIENTE 1     │    MEMORIA        │     SERVIDOR    │
├─────────────────┤    COMPARTIDA     ├─────────────────┤
│ PID: 1234       │──────────────────>│ Procesa TODAS   │
│                 │<──────────────────│ las peticiones  │
└─────────────────┘    (conflictos)    │ en un hilo      │
                                       └─────────────────┘
┌─────────────────┐
│   CLIENTE 2     │    ❌ Solo 1 cliente
├─────────────────┤    ❌ Conflictos de datos
│ PID: 5678       │    ❌ No escalable
│                 │
└─────────────────┘
```

### Ahora ✅
```
┌──────────────────┐              ┌────────────────────────────┐
│    CLIENTE 1     │              │       SERVIDOR             │
├──────────────────┤              ├────────────────────────────┤
│  PID: 1234       │              │                            │
│  FIFO: Conexión  │──FIFO──────> │ ┌─ Hilo Aceptador ──────┐ │
│  (PID)           │ PID=1234     │ │  Recibe conexión       │ │
│                  │              │ │  Crea mem. privada     │ │
│  Mem Privada:    │ <──ATTACH─── │ │  Inicia hilo privado   │ │
│  shmid=1001      │              │ │                        │ │
│  Semáforo OK     │              │ └────────────────────────┘ │
│                  │              │                            │
│  petición ───────┼──SHM────────>│ ┌─ Hilo Cliente 1 ──────┐ │
│  <─ respuesta ───┼──SHM────────<│ │  Procesa Client 1     │ │
└──────────────────┘              │ │  (mem. privada 1)     │ │
                                  │ └────────────────────────┘ │
┌──────────────────┐              │                            │
│    CLIENTE 2     │              │ ┌─ Hilo Cliente 2 ──────┐ │
├──────────────────┤              │ │  Procesa Client 2     │ │
│  PID: 5678       │              │ │  (mem. privada 2)     │ │
│  FIFO: Conexión  │──FIFO──────> │ │                       │ │
│  (PID)           │ PID=5678     │ └───────────────────────┘ │
│                  │              │                            │
│  Mem Privada:    │ <──ATTACH─── │ (... más clientes ...)   │
│  shmid=1002      │              │                            │
│  Semáforo OK     │              └────────────────────────────┘
│                  │
│  petición ───────┼──SHM────────> Cada cliente tiene su
│  <─ respuesta ───┼──SHM────────< hilo dedicado en servidor
└──────────────────┘

✅ Múltiples clientes concurrentes
✅ Cada uno con su memoria privada
✅ Sin conflictos de sincronización
✅ Escalable hasta 10 clientes
```

---

## 📦 Cambios Técnicos

### 1. Estructura de Datos (datos.h)
```c
// Antes
typedef struct {
    // ... campos ...
} Analisis;  // Única, compartida por todos

// Ahora
typedef struct {
    // ... mismos campos ...
} ConexionCliente;  // Una por cliente

typedef struct {
    int heartbeat_global;
    int clientes_activos;
    sem_t candado_global;
} MaestroServidor;  // Control global
```

### 2. Conexión Inicial (main.c)
```c
// Cliente: Se conecta al servidor
ConexionCliente *datos = NULL;
int shmid = conectar_con_servidor(&datos);

// Flujo interno:
// 1. Abre /tmp/vital_conexion_fifo
// 2. Escribe su PID
// 3. Espera a que servidor cree memoria privada
// 4. Obtiene su memoria privada usando ftok(archivo, PID)
// 5. Se adjunta a ella
// 6. CONECTADO ✅
```

### 3. Servidor (servidor.c)
```c
// Servidor: Acepta múltiples clientes
main() {
    // Crear hilo aceptador
    pthread_create(&hilo_aceptador, NULL, 
                   hilo_aceptador_conexiones, NULL);
    
    // Loop principal (auditoría, estadísticas, etc.)
    while (servidor_activo) {
        printf("Clientes activos: %d\n", num_clientes);
        usleep(500000);
    }
}

// Cuando cliente conecta:
procesar_solicitud_conexion(pid_cliente) {
    // 1. Crear memoria privada para ese cliente
    // 2. Adjuntarla
    // 3. Crear hilo dedicado
    // 4. Registrar cliente
    // El hilo comienza a procesar peticiones del cliente
}
```

### 4. Hilo Privado por Cliente (hilo.c)
```c
void* procesar_cliente_privado(void* arg) {
    ConexionCliente *cliente = info->datos;
    
    while (1) {
        bloquear(&cliente->candado);
        
        if (cliente->peticion_activa) {
            // Procesar SOLO este cliente
            if (cliente->tipo_peticion == 1) {
                // Login, validación, etc.
                cliente->respuesta_servidor = resultado;
            }
            cliente->peticion_activa = 0;
            cliente->heartbeat_servidor++;
        }
        
        desbloquear(&cliente->candado);
        usleep(10000);
    }
}
```

### 5. Generación de Claves (memoria.c)
```c
// Clave única por cliente
key_t generar_clave_privada(pid_t pid_cliente) {
    int id_ftok = (pid_cliente % 255) + 100;
    return ftok("main.c", id_ftok);
    // Ejemplo: PID=1234 → id_ftok=234 → clave única
    //         PID=5678 → id_ftok=78  → clave diferente
}
```

---

## 🎯 Flujo de Operación Paso a Paso

### Paso 1: Iniciar Servidor
```bash
$ ./servidor
=== SERVIDOR VITAL-TL INICIADO ===
Esperando conexiones de clientes...

[SERVIDOR] FIFO de conexión creado en /tmp/vital_conexion_fifo
```

### Paso 2: Cliente Conecta
```bash
$ ./cliente
[CLIENTE] Intentando conectar al servidor (PID=1234)...
[CLIENTE] PID enviado al servidor. Esperando memoria privada...
[CLIENTE] Conectado exitosamente al servidor (Memoria privada adjuntada)
```

### Paso 3: Servidor Registra Conexión
```
[SERVIDOR] Solicitud de conexión recibida de cliente PID=1234
[SERVIDOR] Hilo iniciado para cliente PID=1234
[SERVIDOR] Cliente agregado (PID=1234, Índice=0)
[SERVIDOR] Cliente PID=1234 conectado exitosamente
[SERVIDOR] Clientes activos: 1
```

### Paso 4: Cliente Hace Petición
```bash
# Usuario intenta login
[CLIENTE] Usuario: juan
[CLIENTE] Contraseña: ***
[LOG PRIVADO] PID Cliente: 1234 | Accion: Cliente intenta login
```

### Paso 5: Servidor Procesa
```
[SERVIDOR] [PID 1234] Acceso concedido: Paciente juan.
```

### Paso 6: Múltiples Clientes Simultáneamente
```bash
Terminal 1: $ ./cliente  # PID 1234 - Login paciente
Terminal 2: $ ./cliente  # PID 5678 - Login admin
Terminal 3: $ ./cliente  # PID 9012 - Registro nuevo usuario

[SERVIDOR] Clientes activos: 3
- Hilo 1 procesa PID=1234
- Hilo 2 procesa PID=5678
- Hilo 3 procesa PID=9012
(sin conflictos de memoria)
```

---

## ⚙️ Compilación

```bash
$ cd /home/edwin-perea/Descargas/PRO_2/PRO_2/PRO
$ make clean
$ make

# Resultado
gcc ... servidor.o ... -lpthread  # Compilar servidor
gcc ... cliente.o ... -lncurses   # Compilar cliente

✅ servidor listo
✅ cliente listo
```

---

## 📊 Comparativa de Rendimiento

| Métrica | Antes | Ahora |
|---------|-------|-------|
| Clientes simultáneos | 1 | 10+ |
| Contención de semáforo | Alta | Baja (privada) |
| Sobrescritura de datos | Sí | No |
| Latencia/cliente | Depende de otros | Independiente |
| Complejidad código | Baja | Media |

---

## 🔍 Monitoreo

### Ver proceso del servidor
```bash
ps aux | grep servidor
# Muestra: 1 proceso principal + N hilos
```

### Ver clientes conectados
```bash
# Desde servidor (ver logs)
./servidor | grep "Cliente agregado"
```

### Ver memoria compartida
```bash
ipcs -m | grep vital
# Muestra: shmid para cada cliente (1001, 1002, ...)
```

### Ver FIFOs
```bash
ls -la /tmp/vital_conexion_fifo
# Muestra el named pipe
```

---

## 🧹 Limpieza

```bash
# Limpiar compilación
make clean

# Limpiar todo incluyendo FIFO
make clean-all

# Manual (si falla)
rm -f /tmp/vital_conexion_fifo
ipcs -m | grep "0x" | awk '{print $2}' | xargs -I {} ipcrm -m {}
```

---

## 💡 Tips

1. **FIFO no se elimina automáticamente** → usar `make clean-all`
2. **Cada cliente necesita su propio terminal** → no bloquea entrada
3. **Máximo 10 clientes** → configurable en `#define MAX_CLIENTES`
4. **Heartbeat = verificación de vida** → 2 segundos sin cambio = desconexión
5. **Semáforos privados** → evita deadlocks entre clientes

---

## 📞 Soporte

Para más detalles ver: [ARQUITECTURA_PRIVADA.md](ARQUITECTURA_PRIVADA.md)
