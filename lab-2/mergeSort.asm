.data
arreglo:     .word 5, 3, 1, 2, 4, 7, 9, 8, 6, 12, 24, 53, 10
tam:         .word 13
espacio:     .asciiz " "
salto_linea: .asciiz "\n"
msg_antes:   .asciiz "Arreglo original:\n"
msg_despues: .asciiz "Arreglo ordenado:\n"

.text
main:
    li $v0, 4
    la $a0, msg_antes
    syscall
    
    la $a0, arreglo
    lw $a1, tam
    jal mostrar_arreglo
    
    li $v0, 4
    la $a0, salto_linea
    syscall
    
    la $a0, arreglo          # mergeSort(arr, 0, tam-1)
    li $a1, 0
    lw $a2, tam
    addi $a2, $a2, -1
    jal mergesort
    
    li $v0, 4
    la $a0, msg_despues
    syscall
    
    la $a0, arreglo
    lw $a1, tam
    jal mostrar_arreglo
    
    li $v0, 4
    la $a0, salto_linea
    syscall
    
    li $v0, 10
    syscall

mostrar_arreglo:
    move $t0, $a0
    sll $t1, $a1, 2
    add $t1, $t0, $t1
    
mostrar_loop:
    li $v0, 1
    lw $a0, 0($t0)
    syscall

    li $v0, 4
    la $a0, espacio
    syscall
    
    addi $t0, $t0, 4
    blt $t0, $t1, mostrar_loop
    
    jr $ra

mergesort:
    bge $a1, $a2, mergesort_return   # if(left >= right) return
    
    addi $sp, $sp, -16
    sw $ra, 0($sp)
    sw $a1, 4($sp)
    sw $a2, 8($sp)
    sw $s0, 12($sp)
    
    add $s0, $a1, $a2                # int mid = (left + right) / 2
    srl $s0, $s0, 1
    
    move $a2, $s0                     # mergeSort(arr, left, mid)
    jal mergesort
    
    lw $a1, 4($sp)                    # mergeSort(arr, mid+1, right)
    lw $a2, 8($sp)
    addi $a1, $s0, 1
    jal mergesort
    
    lw $a1, 4($sp)                    # merge(arr, left, mid, right)
    lw $a2, 8($sp)
    move $a3, $s0
    jal merge
    
    lw $ra, 0($sp)
    lw $s0, 12($sp)
    addi $sp, $sp, 16
    
mergesort_return:
    jr $ra

merge:
    sub $t0, $a2, $a1                 # int size = right - left + 1
    addi $t0, $t0, 1
    sll $t0, $t0, 2
    
    sub $sp, $sp, $t0                 # int *temp = alloca(size * 4)
    move $t6, $sp
    
    move $t0, $a1                      # int i = left
    addi $t1, $a3, 1                   # int j = mid + 1
    move $t2, $a1                      # int k = left
    
    sll $t3, $a1, 2
    add $t3, $a0, $t3
    sll $t4, $a3, 2
    add $t4, $a0, $t4
    sll $t5, $a2, 2
    add $t5, $a0, $t5
    
merge_loop:
    bgt $t0, $a3, copy_right
    bgt $t1, $a2, copy_left
    
    lw $t7, 0($t3)
    sll $t8, $t1, 2
    add $t8, $a0, $t8
    lw $t9, 0($t8)
    
    ble $t7, $t9, take_i
    
    sw $t9, 0($t6)                     # *temp++ = arr[j++]
    addi $t1, $t1, 1
    j continue_merge
    
take_i:
    sw $t7, 0($t6)                     # *temp++ = arr[i++]
    addi $t0, $t0, 1
    addi $t3, $t3, 4
    
continue_merge:
    addi $t6, $t6, 4
    addi $t2, $t2, 1
    j merge_loop

copy_right:
    bgt $t1, $a2, copy_done
    
    sll $t7, $t1, 2
    add $t7, $a0, $t7
    lw $t8, 0($t7)
    sw $t8, 0($t6)                     # *temp++ = arr[j++]
    
    addi $t1, $t1, 1
    addi $t6, $t6, 4
    addi $t2, $t2, 1
    j copy_right

copy_left:
    bgt $t0, $a3, copy_done
    
    lw $t7, 0($t3)
    sw $t7, 0($t6)                     # *temp++ = arr[i++]
    
    addi $t0, $t0, 1
    addi $t3, $t3, 4
    addi $t6, $t6, 4
    addi $t2, $t2, 1
    j copy_left

copy_done:
    sub $t6, $t6, $sp
    srl $t6, $t6, 2
    
    move $t0, $sp                       # int *src = temp
    sll $t1, $a1, 2
    add $t2, $a0, $t1                   # int *dst = &arr[left]
    
copy_back:
    beqz $t6, merge_done
    
    lw $t3, 0($t0)
    sw $t3, 0($t2)                      # *dst++ = *src++
    
    addi $t0, $t0, 4
    addi $t2, $t2, 4
    addi $t6, $t6, -1
    j copy_back

merge_done:
    sub $t0, $a2, $a1
    addi $t0, $t0, 1
    sll $t0, $t0, 2
    add $sp, $sp, $t0
    
    jr $ra