#include "../include/common.h"

volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
int cursor_x = 0; int cursor_y = 0; 
const unsigned char color = 0x0A;

// Функция для управления аппаратным курсором видеокарты
void update_hw_cursor(int x, int y) {
    unsigned short pos = y * 80 + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) vga[i] = (unsigned short)' ' | (color << 8);
    cursor_x = 0; cursor_y = 0;
    update_hw_cursor(0, 0);
}

void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0; cursor_y++;
    } else {
        vga[cursor_y * 80 + cursor_x] = (unsigned short)c | (color << 8);
        cursor_x++;
    }
    if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }
    if (cursor_y >= 25) clear_screen();
    update_hw_cursor(cursor_x, cursor_y);
}

void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) print_char(str[i]);
}

void delete_last_char() {
    if (cursor_x > 0) cursor_x--;
    else if (cursor_y > 0) { cursor_y--; cursor_x = 79; }
    vga[cursor_y * 80 + cursor_x] = (unsigned short)' ' | (color << 8);
    update_hw_cursor(cursor_x, cursor_y);
}

void print_prompt() {
    print_string("[");
    print_string(fs[current_dir].name);
    print_string("]> ");
}

char scancode_to_char(unsigned char scancode) {
    const char kbd_us[128] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
      '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
        0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
       '*', 0, ' ', 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,0
    };
    if (scancode < 128) return kbd_us[scancode];
    return 0;
}