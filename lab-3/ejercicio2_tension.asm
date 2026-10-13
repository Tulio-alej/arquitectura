# Lab 3 - Ejercicio 2: modulo de tension arterial (E/S mapeada en memoria simulada)
# main simula el dispositivo dejando el estado en 1 y los resultados cargados.

        .data
TensionControl: .word 0
TensionEstado:  .word 0
TensionSistol:  .word 0
TensionDiastol: .word 0

msgSis: .asciiz "Tension sistolica:  "
msgDia: .asciiz "Tension diastolica: "
nl:     .asciiz "\n"

        .text
        .globl main
main:
        # --- Simulacion del dispositivo: medicion terminada ---
        la   $t0, TensionSistol
        li   $t1, 120
        sw   $t1, 0($t0)
        la   $t0, TensionDiastol
        li   $t1, 80
        sw   $t1, 0($t0)
        la   $t0, TensionEstado
        li   $t1, 1
        sw   $t1, 0($t0)

        jal  controlador_tension
        move $s0, $v0
        move $s1, $v1

        la   $a0, msgSis
        li   $v0, 4
        syscall
        move $a0, $s0
        li   $v0, 1
        syscall
        la   $a0, nl
        li   $v0, 4
        syscall

        la   $a0, msgDia
        li   $v0, 4
        syscall
        move $a0, $s1
        li   $v0, 1
        syscall
        la   $a0, nl
        li   $v0, 4
        syscall

        li   $v0, 10
        syscall

# ---------------------------------------------------------------
# controlador_tension
#   Salida: $v0 = sistolica, $v1 = diastolica
# ---------------------------------------------------------------
controlador_tension:
        la   $t0, TensionControl
        li   $t1, 1
        sw   $t1, 0($t0)             # inicia la medicion
        la   $t0, TensionEstado
espera:
        lw   $t1, 0($t0)
        beq  $t1, $zero, espera      # 0 -> midiendo
        la   $t0, TensionSistol
        lw   $v0, 0($t0)
        la   $t0, TensionDiastol
        lw   $v1, 0($t0)
        jr   $ra
