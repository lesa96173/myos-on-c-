ENTRY(_start)

SECTIONS {
    /* Начинаем собирать ядро с адреса 1 МБ */
    . = 1M;

    /* Сначала идет заголовок Multiboot и код */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
    }

    /* Данные только для чтения */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* Инициализированные данные */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* Неинициализированные данные и стек */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}