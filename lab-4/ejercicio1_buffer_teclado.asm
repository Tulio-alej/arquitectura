# Lab 4 - Ejercicio 1: buffer circular de teclado con filtro A-Z
# Uso en MARS: Tools > Keyboard and Display MMIO Simulator > "Connect to MIPS",
# luego ensamblar y ejecutar. Escribir en el campo de teclado del simulador.
# El tiempo (20 s) se mide con syscall 30 (tiempo del sistema en ms).

        .eqv RCTRL  0xffff0000
        .eqv RDATA  0xffff0004
        .eqv BUFSZ  16
        .eqv MS20   20000

        .data
buffer:  .space 16
head:    .word 0            # posicion de escritura
tail:    .word 0            # posicion de lectura
count:   .word 0            # elementos validos
msgIni:  .asciiz "\n--- Escriba (solo A-Z se guardan), 20 segundos ---\n"
msgBuf:  .asciiz "\nContenido del buffer: "
nl:      .asciiz "\n"
chbuf:   .space 2

        .kdata
sv_at:   .word 0
sv_t0:   .word 0
sv_t1:   .word 0
sv_t2:   .word 0

        .text
        .globl main
main:
        # habilitar interrupcion del teclado en el dispositivo (bit 1)
        li   $t0, RCTRL
        li   $t1, 2
        sw   $t1, 0($t0)
        # habilitar interrupciones en el procesador: Status IE (bit0) + IM bit 8
        mfc0 $t0, $12
        ori  $t0, $t0, 0x0101
        mtc0 $t0, $12

ciclo:
        la   $a0, msgIni
        li   $v0, 4
        syscall

        li   $v0, 30            # tiempo inicial en $a0 (ms, parte baja)
        syscall
        move $s0, $a0

espera:
        li   $v0, 30
        syscall
        subu $t0, $a0, $s0
        blt  $t0, MS20, espera  # las interrupciones llegan durante esta espera

        # --- vaciar e imprimir el buffer (seccion critica: se deshabilita IE) ---
        mfc0 $t0, $12
        andi $t0, $t0, 0xfffe
        mtc0 $t0, $12

        la   $a0, msgBuf
        li   $v0, 4
        syscall

vaciar:
        lw   $t0, count
        beqz $t0, finVaciar
        lw   $t1, tail
        lb   $a0, buffer($t1)
        li   $v0, 11
        syscall
        addi $t1, $t1, 1
        li   $t2, BUFSZ
        rem  $t1, $t1, $t2
        sw   $t1, tail
        addi $t0, $t0, -1
        sw   $t0, count
        j    vaciar
finVaciar:
        la   $a0, nl
        li   $v0, 4
        syscall

        # reactivar interrupciones y repetir
        mfc0 $t0, $12
        ori  $t0, $t0, 1
        mtc0 $t0, $12
        j    ciclo

# =====================================================================
# Rutina de servicio de excepciones / interrupciones
# =====================================================================
        .ktext 0x80000180
        move $k1, $at            # guardar $at (MARS ignora .set noat)
        sw   $k1, sv_at
        sw   $t0, sv_t0
        sw   $t1, sv_t1
        sw   $t2, sv_t2

        mfc0 $k0, $13            # Cause
        srl  $t0, $k0, 2
        andi $t0, $t0, 0x1f      # codigo de excepcion
        bnez $t0, fin            # no es interrupcion: se ignora
        andi $t0, $k0, 0x100     # bit 8: interrupcion de teclado
        beqz $t0, fin

        li   $t0, RDATA
        lw   $t1, 0($t0)         # leer caracter (limpia Ready)
        andi $t1, $t1, 0xff
        blt  $t1, 'a', mayus     # minuscula a-z -> convertir a mayuscula
        bgt  $t1, 'z', mayus
        addi $t1, $t1, -32
mayus:
        blt  $t1, 'A', fin       # filtro A-Z
        bgt  $t1, 'Z', fin

        # insertar en buffer circular (si esta lleno, sobreescribe el mas antiguo)
        lw   $t0, head
        sb   $t1, buffer($t0)
        addi $t0, $t0, 1
        li   $t2, BUFSZ
        rem  $t0, $t0, $t2
        sw   $t0, head
        lw   $t1, count
        beq  $t1, BUFSZ, lleno
        addi $t1, $t1, 1
        sw   $t1, count
        j    fin
lleno:
        lw   $t1, tail          # descarta el mas antiguo
        addi $t1, $t1, 1
        li   $t2, BUFSZ
        rem  $t1, $t1, $t2
        sw   $t1, tail

fin:
        lw   $t2, sv_t2
        lw   $t1, sv_t1
        lw   $t0, sv_t0
        lw   $k1, sv_at
        move $at, $k1
        mtc0 $zero, $13          # limpiar Cause
        mfc0 $k0, $12
        ori  $k0, $k0, 1         # reactivar IE (EXL se limpia con eret)
        mtc0 $k0, $12
        eret
