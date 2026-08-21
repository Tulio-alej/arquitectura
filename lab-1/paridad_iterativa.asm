.data
    prompt:  .asciiz "Ingrese un numero a revisar: "
    res_msg: .asciiz "Numero de paridad: "
    msg_par: .asciiz "\nEl numero es par"
    msg_imp: .asciiz "\nEl numero es impar"

.text
.globl main

main:
    li $v0, 4          # C: printf(prompt)
    la $a0, prompt
    syscall
    
    li $v0, 5          # C: scanf("%d", &v0)
    syscall
    move $t0, $v0      # C: n = v0

    li $t1, 0          # C: resultado = 0

ciclo:
    beq $t0, $zero, mostrar_resultado # C: while (n != 0) {
    
    li $t2, 1          # C: t2 = 1
    sub $t1, $t2, $t1  # C: resultado = 1 - resultado
    
    sub $t0, $t0, 1    # C: n = n - 1
    j ciclo            # C: }

mostrar_resultado:
    li $v0, 4          # C: printf(res_msg)
    la $a0, res_msg
    syscall
    
    li $v0, 1          # C: printf("%d", resultado)
    move $a0, $t1
    syscall

    beq $t1, $zero, es_par  # C: if (resultado == 0) go to es_par
    
    li $v0, 4               # C: printf(msg_imp)
    la $a0, msg_imp
    syscall
    j salir                 # C: break

es_par:
    li $v0, 4               # C: printf(msg_par)
    la $a0, msg_par
    syscall

salir:
    li $v0, 10         # C: return 0
    syscall