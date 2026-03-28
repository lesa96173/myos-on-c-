MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384 ; Выделяем 16 КБ для стека
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Устанавливаем указатель стека
    mov esp, stack_top
    ; Переходим в наше C++ ядро
    call kernel_main
    ; Если ядро по какой-то причине завершит работу, останавливаем процессор
    cli
.hang:
    hlt
    jmp .hang