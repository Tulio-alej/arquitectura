.data
    msg_ingreso:  .asciiz "Ingrese un numero a revisar: "
    msg_resultado: .asciiz "Numero de paridad: "
    msg_par: .asciiz "\nEl numero es par"
    msg_imp: .asciiz "\nEl numero es impar"

.text
.globl main

main:
    li $v0, 4          # C: printf(prompt)
    la $a0, msg_ingreso
    syscall
    
    li $v0, 5          # C: scanf("%d", &v0)
    syscall
    move $a0, $v0      # C: n = v0

    jal paridad        # C: v0 = paridad(n)

    move $t0, $v0      # C: resultado = v0
    
    # Mostrar el valor 
    li $v0, 4          # C: printf(res_msg)
    la $a0, msg_resultado
    syscall
    
    li $v0, 1          # C: printf("%d", resultado)
    move $a0, $t0
    syscall

    beq $t0, $zero, es_par  # C: if (resultado == 0) go to es_par
    
    li $v0, 4               # C: printf(msg_imp)
    la $a0, msg_imp
    syscall
    j salir

es_par:
    li $v0, 4               # C: printf(msg_par)
    la $a0, msg_par
    syscall

salir:
    li $v0, 10         # C: return 0
    syscall

# Funcion Recursiva
paridad:
    subu $sp, $sp, 8   # C: stack -= 8
    sw $ra, 4($sp)     # C: stack[1] = return_address
    sw $a0, 0($sp)     # C: stack[0] = n

    beq $a0, $zero, caso_base # C: if (n == 0) return 0

    sub $a0, $a0, 1    # C: n = n - 1
    jal paridad        # C: v0 = paridad(n - 1)
    
    li $t1, 1          # C: t1 = 1
    sub $v0, $t1, $v0  # C: v0 = 1 - v0
    j fin

caso_base:
    li $v0, 0          # C: v0 = 0

fin:
    lw $ra, 4($sp)     # C: return_address = stack[1]
    addu $sp, $sp, 8   # C: stack += 8
    jr $ra             # C: return v0
