.data
arreglo: .word 5, 3, 1, 2, 4
tam: .word 5
espacio: .asciiz " "
salto_linea: .asciiz "\n"
msg_antes: .asciiz "Arreglo original:\n"
msg_despues: .asciiz "Arreglo ordenado:\n"

.text
main:
    li $v0, 4
    la $a0, msg_antes
    syscall
    
    la $a0, arreglo
    lw $a1, tam
    jal mostrar_arreglo
    
    la $a0, arreglo
    li $a1, 0
    lw $a2, tam
    addi $a2, $a2, -1
    jal quicksort
    
    li $v0, 4
    la $a0, msg_despues
    syscall
    
    la $a0, arreglo
    lw $a1, tam
    jal mostrar_arreglo
    
    li $v0, 10
    syscall

mostrar_arreglo:
    move $t0, $a0               # int *ptr = arr
    li $t1, 0                   # int i = 0
    
mostrar_loop:
    bge $t1, $a1, mostrar_fin   # if(i >= size) break
    
    lw $a0, 0($t0)              # printf("%d", *ptr)
    li $v0, 1
    syscall
    
    li $v0, 4
    la $a0, espacio
    syscall
    
    addi $t0, $t0, 4            # ptr++
    addi $t1, $t1, 1            # i++
    j mostrar_loop
    
mostrar_fin:
    li $v0, 4
    la $a0, salto_linea
    syscall
    jr $ra

quicksort:
    bge $a1, $a2, quicksort_return  # if(low >= high) return
    
    addi $sp, $sp, -16
    sw $ra, 0($sp)
    sw $a1, 4($sp)              # int low_saved = low
    sw $a2, 8($sp)              # int high_saved = high
    sw $s0, 12($sp)
    
    jal partition               # int pivot = partition(arr, low, high)
    move $s0, $v0               # pivot_index = pivot
    
    lw $a1, 4($sp)              # low = low_saved
    move $a2, $s0               
    addi $a2, $a2, -1           # high = pivot_index - 1
    jal quicksort               # quicksort(arr, low, pivot-1)
    
    addi $a1, $s0, 1            # low = pivot_index + 1
    lw $a2, 8($sp)              # high = high_saved
    jal quicksort               # quicksort(arr, pivot+1, high)
    
    lw $ra, 0($sp)
    lw $s0, 12($sp)
    addi $sp, $sp, 16
    
quicksort_return:
    jr $ra

partition:
    sll $t0, $a2, 2
    add $t0, $a0, $t0           # int *pivot_addr = arr + high
    lw $t1, 0($t0)              # int pivot = *pivot_addr
    
    move $t2, $a1               
    addi $t2, $t2, -1           # int i = low - 1
    move $t3, $a1               # int j = low
    
partition_loop:
    bge $t3, $a2, partition_done # while(j < high)
    
    sll $t4, $t3, 2
    add $t4, $a0, $t4           # int *j_addr = arr + j
    lw $t5, 0($t4)              # int val_j = *j_addr
    
    bgt $t5, $t1, partition_continue # if(val_j > pivot) continue
    
    addi $t2, $t2, 1            # i++
    
    sll $t6, $t2, 2
    add $t6, $a0, $t6           # int *i_addr = arr + i
    lw $t7, 0($t6)              # int val_i = *i_addr
    
    sw $t5, 0($t6)              # *i_addr = val_j
    sw $t7, 0($t4)              # *j_addr = val_i
    
partition_continue:
    addi $t3, $t3, 1            # j++
    j partition_loop

partition_done:
    addi $t2, $t2, 1            # int pivot_pos = i + 1
    
    sll $t4, $t2, 2
    add $t4, $a0, $t4           # int *pos_addr = arr + pivot_pos
    lw $t5, 0($t4)              # int val_at_pos = *pos_addr
    
    sw $t1, 0($t4)              # *pos_addr = pivot
    sw $t5, 0($t0)              # *pivot_addr = val_at_pos
    
    move $v0, $t2               # return pivot_pos
    jr $ra