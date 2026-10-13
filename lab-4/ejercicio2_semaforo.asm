# Lab 4 - Ejercicio 2: semaforo con pulsador (tecla 's')
# Uso en MARS: Tools > Keyboard and Display MMIO Simulator > "Connect to MIPS".
# La tecla genera interrupcion; los temporizadores se miden con syscall 30 (ms).

        .eqv RCTRL  0xffff0000
        .eqv RDATA  0xffff0004

        .data
pulsado: .word 0
salir:   .word 0
msgE:    .asciiz "Tecla 'e' pulsada: fin de la ejecucion\n"
msgV:    .asciiz "Semaforo en verde, esperando pulsador\n"
msgP:    .asciiz "Pulsador activado: en 20 segundos, el semaforo cambiara a amarillo\n"
msgA:    .asciiz "Semaforo en amarillo, en 10 segundos, semaforo en rojo\n"
msgR:    .asciiz "Semaforo en rojo, en 30 segundos, semaforo en verde\n"

        .kdata
sv_t0:   .word 0
sv_t1:   .word 0

        .text
        .globl main
main:
        li   $t0, RCTRL
        li   $t1, 2
        sw   $t1, 0($t0)         # habilita interrupcion del teclado
        mfc0 $t0, $12
        ori  $t0, $t0, 0x0101    # IE + IM8
        mtc0 $t0, $12

ciclo:
        sw   $zero, pulsado
        la   $a0, msgV
        li   $v0, 4
        syscall
esperaPulsador:
        lw   $t0, salir
        bnez $t0, terminar
        lw   $t0, pulsado
        beqz $t0, esperaPulsador

        la   $a0, msgP
        li   $v0, 4
        syscall
        li   $s1, 20000
        jal  temporizador

        la   $a0, msgA
        li   $v0, 4
        syscall
        li   $s1, 10000
        jal  temporizador

        la   $a0, msgR
        li   $v0, 4
        syscall
        li   $s1, 30000
        jal  temporizador

        j    ciclo

terminar:
        la   $a0, msgE
        li   $v0, 4
        syscall
        li   $v0, 10
        syscall

# temporizador: espera $s1 milisegundos (syscall 30 pisa $a0 y $a1,
# por eso la duracion se guarda en $s1). Si se pulsa 'e', termina el programa.
temporizador:
        li   $v0, 30
        syscall
        move $t8, $a0
tloop:
        lw   $t0, salir
        bnez $t0, terminar
        li   $v0, 30
        syscall
        subu $t9, $a0, $t8
        bltu $t9, $s1, tloop
        jr   $ra

# =====================================================================
        .ktext 0x80000180
        move $k1, $at
        sw   $t0, sv_t0
        sw   $t1, sv_t1
        mfc0 $k0, $13
        srl  $t0, $k0, 2
        andi $t0, $t0, 0x1f
        bnez $t0, fin
        andi $t0, $k0, 0x100
        beqz $t0, fin
        li   $t0, RDATA
        lw   $t1, 0($t0)         # lee caracter
        andi $t1, $t1, 0xff
        beq  $t1, 'e', pedirSalir
        bne  $t1, 's', fin
        li   $t1, 1
        sw   $t1, pulsado        # marca pulsador (el ciclo principal lo consume)
        j    fin
pedirSalir:
        li   $t1, 1
        sw   $t1, salir          # el programa principal termina
fin:
        lw   $t1, sv_t1
        lw   $t0, sv_t0
        move $at, $k1
        mtc0 $zero, $13
        mfc0 $k0, $12
        ori  $k0, $k0, 1
        mtc0 $k0, $12
        eret
