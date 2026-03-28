// Файл: libc/stdio.cpp
#include "stdio.h"
#include "../include/common.h"

// Нам нужна функция перевода числа в строку (itoa). 
// Я написал мощную версию, которая понимает даже отрицательные числа!
void itoa(int num, char* str) {
    if (num == 0) { str[0] = '0'; str[1] = '\0'; return; }
    int temp = num, len = 0;
    
    if (num < 0) { str[0] = '-'; len++; temp = -num; num = -num; }
    
    while(temp > 0) { len++; temp /= 10; }
    str[len] = '\0';
    
    while(num > 0) { str[--len] = (num % 10) + '0'; num /= 10; }
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format); // Начинаем перебирать аргументы после format

    for (int i = 0; format[i] != '\0'; i++) {
        // Если нашли знак процента и это не конец строки
        if (format[i] == '%' && format[i+1] != '\0') {
            i++; // Прыгаем на следующий символ (s, d, c)
            
            if (format[i] == 's') {
                // Если %s - вытаскиваем строку и печатаем
                char* s = va_arg(args, char*);
                print_string(s);
            } 
            else if (format[i] == 'd') {
                // Если %d - вытаскиваем число, конвертируем в строку и печатаем
                int n = va_arg(args, int);
                char buf[32];
                itoa(n, buf);
                print_string(buf);
            } 
            else if (format[i] == 'c') {
                // Если %c - печатаем один символ
                char c = (char)va_arg(args, int);
                print_char(c);
            } 
            else if (format[i] == '%') {
                // Если написали %% - печатаем просто знак процента
                print_char('%');
            }
        } else {
            // Если это обычная буква - просто печатаем её
            print_char(format[i]);
        }
    }
    va_end(args);
}