# Lab 3 - Ejercicio 1: sensor de luminosidad (E/S mapeada en memoria simulada)
# Los registros del dispositivo se ubican en la memoria de datos.
# En MARS no hay hardware real, asi que main simula el comportamiento del
# dispositivo escribiendo LuzEstado / LuzDatos antes de cada llamada.

        .data
LuzControl: .word 0          # escribir 1 -> inicializa
LuzEstado:  .word 0          # 0 no listo, 1 lectura disponible, -1 error
LuzDatos:   .word 0          # lectura 0..1023

msgInit:    .asciiz "Sensor inicializado.\n"
msgLect:    .asciiz "Luminosidad leida: "
msgEstado:  .asciiz "  Codigo de estado: "
msgErr:     .asciiz "Error de hardware en el sensor.\n"
nl:         .asciiz "\n"

        .text
        .globl main
main:
        # --- Simulacion: el sensor queda listo tras la inicializacion ---
        la   $t0, LuzEstado
        li   $t1, 1
        sw   $t1, 0($t0)

        jal  InicializarSensorLuz
        la   $a0, msgInit
        li   $v0, 4
        syscall

        # --- Lectura correcta: el dispositivo entrega 742 ---
        la   $t0, LuzDatos
        li   $t1, 742
        sw   $t1, 0($t0)
        jal  LeerLuminosidad
        move $s0, $v0            # dato
        move $s1, $v1            # estado
        jal  imprimirResultado

        # --- Simulacion de error de hardware ---
        la   $t0, LuzEstado
        li   $t1, -1
        sw   $t1, 0($t0)
        jal  LeerLuminosidad
        move $s0, $v0
        move $s1, $v1
        jal  imprimirResultado

        li   $v0, 10
        syscall

# ---------------------------------------------------------------
# InicializarSensorLuz: escribe 1 en LuzControl y espera LuzEstado == 1
# ---------------------------------------------------------------
InicializarSensorLuz:
        la   $t0, LuzControl
        li   $t1, 1
        sw   $t1, 0($t0)         # inicializa el sensor
        la   $t0, LuzEstado
esperaInit:
        lw   $t1, 0($t0)
        li   $t2, 1
        bne  $t1, $t2, esperaInit    # espera activa hasta que este listo
        jr   $ra

# ---------------------------------------------------------------
# LeerLuminosidad
#   Salida: $v0 = lectura (0 si error), $v1 = 0 correcta, -1 error
# ---------------------------------------------------------------
LeerLuminosidad:
        la   $t0, LuzEstado
        lw   $t1, 0($t0)
        li   $t2, -1
        beq  $t1, $t2, lecturaError
        li   $t2, 1
        bne  $t1, $t2, LeerLuminosidad   # 0 -> aun no listo, sigue esperando
        la   $t0, LuzDatos
        lw   $v0, 0($t0)
        li   $v1, 0
        jr   $ra
lecturaError:
        li   $v0, 0
        li   $v1, -1
        jr   $ra

# ---------------------------------------------------------------
# imprimirResultado: usa $s0 (dato) y $s1 (estado)
# ---------------------------------------------------------------
imprimirResultado:
        bnez $s1, imprimirError
        la   $a0, msgLect
        li   $v0, 4
        syscall
        move $a0, $s0
        li   $v0, 1
        syscall
        la   $a0, msgEstado
        li   $v0, 4
        syscall
        move $a0, $s1
        li   $v0, 1
        syscall
        la   $a0, nl
        li   $v0, 4
        syscall
        jr   $ra
imprimirError:
        la   $a0, msgErr
        li   $v0, 4
        syscall
        jr   $ra
