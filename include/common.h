#pragma once

// Порты
static inline unsigned char inb(unsigned short port) {
    unsigned char ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline unsigned short inw(unsigned short port) {
    unsigned short ret; asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static inline void outw(unsigned short port, unsigned short val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Строки
bool strings_equal(const char* s1, const char* s2);
void copy_string(char* dest, const char* src, int max_len);
bool starts_with(const char* str, const char* prefix);
int string_length(const char* str);

// Файловая система
struct FSNode {
    bool is_used;
    bool is_dir;
    int parent_id;
    char name[16];
    int size;          
    int first_sector;  
};

extern FSNode fs[]; 
extern unsigned short fat[]; 
extern int current_dir;
const int MAX_NODES = 20;

void init_fs();
int find_free_node();
void load_disk();
void save_disk();
char* read_file_to_ram(int node_id);
void write_ram_to_file(int node_id, const char* buffer, int length);

// Экран
extern volatile unsigned short* vga;
extern const unsigned char color;
void clear_screen();
void print_char(char c);
void print_string(const char* str);
void delete_last_char();
void print_prompt();
char scancode_to_char(unsigned char scancode);

// Память
void init_memory();
void* malloc(int size);
void free(void* ptr);